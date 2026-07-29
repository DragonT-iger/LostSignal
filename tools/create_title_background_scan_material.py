"""타이틀 배경용 느린 스캔 라인/간헐 깜빡임 UI 머티리얼을 생성한다."""

import unreal


MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
ASSET_TOOLS = unreal.AssetToolsHelpers.get_asset_tools()

PACKAGE_PATH = "/Game/LostSignal/Sandbox/YoonSeok/UI"
MATERIAL_NAME = "M_UI_TitleBackgroundScan"
MATERIAL_PATH = f"{PACKAGE_PATH}/{MATERIAL_NAME}"

SCAN_HLSL = r"""
float2 BaseUV = saturate(UV);
float4 Source = Texture2DSample(Texture, TextureSampler, BaseUV);

// 화면 위에서 아래로 천천히 순환하는 부드러운 한 줄.
float SafeScanSpeed = max(ScanSpeed, 0.0001);
float SafeScanWidth = max(ScanWidth, 0.0001);
float ScanY = frac(TimeValue * SafeScanSpeed);
float ScanDistance = abs(BaseUV.y - ScanY);
ScanDistance = min(ScanDistance, 1.0 - ScanDistance);
float ScanMask = 1.0 - smoothstep(SafeScanWidth, SafeScanWidth * 2.0, ScanDistance);
float ScanGain = 1.0 + ScanMask * max(ScanIntensity, 0.0);

// 짧은 시간 단위의 난수를 사용해 드물게 한 틱 동안만 어두워진다.
float SafeFlickerRate = max(FlickerRate, 0.01);
float FlickerTick = floor(TimeValue * SafeFlickerRate);
float FlickerNoise = frac(
    sin((FlickerTick + RandomSeed * 37.0) * 12.9898) * 43758.5453
);
float FlickerBurst = step(saturate(FlickerThreshold), FlickerNoise);
float FlickerGain = lerp(
    1.0,
    saturate(1.0 - FlickerStrength),
    FlickerBurst
);

float3 FinalColor = Source.rgb * ScanGain * FlickerGain;
return float4(FinalColor, Source.a);
"""

MATERIAL = None


def log(message):
    unreal.log(f"[TitleBackgroundScan] {message}")


def log_error(message):
    unreal.log_error(f"[TitleBackgroundScan] {message}")


def expression(expression_class, x, y, **properties):
    node = MEL.create_material_expression(MATERIAL, expression_class, x, y)
    for name, value in properties.items():
        node.set_editor_property(name, value)
    return node


def custom_input(name):
    value = unreal.CustomInput()
    value.set_editor_property("input_name", name)
    return value


def connect(source, output_name, destination, input_name):
    output_names = [output_name] if isinstance(output_name, str) else output_name
    input_names = [input_name] if isinstance(input_name, str) else input_name
    for candidate_output in output_names:
        for candidate_input in input_names:
            if MEL.connect_material_expressions(
                source, candidate_output, destination, candidate_input
            ):
                return
    log_error(
        f"연결 실패: {source.get_name()}({output_names}) -> "
        f"{destination.get_name()}({input_names})"
    )


def scalar_parameter(name, default_value, y):
    return expression(
        unreal.MaterialExpressionScalarParameter,
        -1000,
        y,
        parameter_name=name,
        default_value=default_value,
        group="LS/Title Background",
    )


def build_graph():
    texture = expression(
        unreal.MaterialExpressionTextureObjectParameter,
        -1300,
        -350,
        parameter_name="Texture",
        group="LS/Title Background",
    )
    uv = expression(unreal.MaterialExpressionTextureCoordinate, -1300, -200)
    time = expression(unreal.MaterialExpressionTime, -1300, -50)

    parameters = {
        "ScanSpeed": scalar_parameter("ScanSpeed", 0.08, 100),
        "ScanWidth": scalar_parameter("ScanWidth", 0.015, 200),
        "ScanIntensity": scalar_parameter("ScanIntensity", 0.18, 300),
        "FlickerRate": scalar_parameter("FlickerRate", 10.0, 450),
        "FlickerThreshold": scalar_parameter("FlickerThreshold", 0.985, 550),
        "FlickerStrength": scalar_parameter("FlickerStrength", 0.32, 650),
        "RandomSeed": scalar_parameter("RandomSeed", 1.0, 750),
    }

    input_names = [
        "Texture",
        "UV",
        "TimeValue",
        "ScanSpeed",
        "ScanWidth",
        "ScanIntensity",
        "FlickerRate",
        "FlickerThreshold",
        "FlickerStrength",
        "RandomSeed",
    ]
    custom = expression(
        unreal.MaterialExpressionCustom,
        -400,
        0,
        description="LS 타이틀 배경 스캔/깜빡임",
        code=SCAN_HLSL,
        output_type=unreal.CustomMaterialOutputType.CMOT_FLOAT4,
        inputs=[custom_input(name) for name in input_names],
    )

    connect(texture, "", custom, "Texture")
    connect(uv, ["", "UV"], custom, "UV")
    connect(time, ["", "Time"], custom, "TimeValue")
    for name, parameter in parameters.items():
        connect(parameter, "", custom, name)

    rgb = expression(
        unreal.MaterialExpressionComponentMask,
        0,
        -50,
        r=True,
        g=True,
        b=True,
        a=False,
    )
    alpha = expression(
        unreal.MaterialExpressionComponentMask,
        0,
        100,
        r=False,
        g=False,
        b=False,
        a=True,
    )
    connect(custom, "", rgb, ["Input", ""])
    connect(custom, "", alpha, ["Input", ""])

    if not MEL.connect_material_property(
        rgb, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR
    ):
        log_error("Final Color 연결 실패")
    if not MEL.connect_material_property(alpha, "", unreal.MaterialProperty.MP_OPACITY):
        log_error("Opacity 연결 실패")


def main():
    global MATERIAL

    if EAL.does_asset_exist(MATERIAL_PATH):
        log_error(f"이미 존재하는 자산이라 덮어쓰지 않았습니다: {MATERIAL_PATH}")
        return

    MATERIAL = ASSET_TOOLS.create_asset(
        MATERIAL_NAME,
        PACKAGE_PATH,
        unreal.Material,
        unreal.MaterialFactoryNew(),
    )
    if not MATERIAL:
        log_error("머티리얼 생성 실패")
        return

    MATERIAL.set_editor_property("material_domain", unreal.MaterialDomain.MD_UI)
    MATERIAL.set_editor_property(
        "blend_mode", unreal.BlendMode.BLEND_ALPHA_COMPOSITE
    )

    build_graph()
    MEL.recompile_material(MATERIAL)
    EAL.save_asset(MATERIAL_PATH, only_if_is_dirty=False)
    log(f"생성 완료: {MATERIAL_PATH}")


if __name__ == "__main__":
    main()
