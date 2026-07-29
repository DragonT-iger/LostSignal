"""타이틀 로고/문자용 간헐 RGB 글리치 UI 머티리얼을 생성한다."""

import unreal


MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
ASSET_TOOLS = unreal.AssetToolsHelpers.get_asset_tools()

PACKAGE_PATH = "/Game/LostSignal/Sandbox/YoonSeok/UI"
MATERIAL_NAME = "M_UI_TitleLogoGlitch"
MATERIAL_PATH = f"{PACKAGE_PATH}/{MATERIAL_NAME}"
INSTANCE_NAME = "MI_UI_TitleLogoGlitch"
INSTANCE_PATH = f"{PACKAGE_PATH}/{INSTANCE_NAME}"
PREVIEW_INSTANCE_NAME = "MI_UI_TitleLogoGlitch_Preview"
PREVIEW_INSTANCE_PATH = f"{PACKAGE_PATH}/{PREVIEW_INSTANCE_NAME}"

GLITCH_HLSL = r"""
float2 BaseUV = saturate(UV);

// 짧은 시간 단위 난수로 드문 글리치 버스트를 만든다.
float SafeBurstRate = max(BurstRate, 0.01);
float BurstTick = floor(TimeValue * SafeBurstRate);
float BurstNoise = frac(
    sin((BurstTick + RandomSeed * 71.0) * 12.9898) * 43758.5453
);
float AutoBurst = step(saturate(BurstThreshold), BurstNoise);
float Burst = max(AutoBurst, saturate(ManualBurst));
float Strength = saturate(GlitchStrength) * Burst;

// 가로 조각별로 서로 다른 방향과 크기의 이동을 준다.
float SafeSliceDensity = max(SliceDensity, 1.0);
float SliceIndex = floor(BaseUV.y * SafeSliceDensity);
float SliceNoise = frac(
    sin((SliceIndex + BurstTick * 23.0 + RandomSeed * 131.0) * 78.233)
    * 43758.5453
);
float SliceGate = step(1.0 - (0.10 + Strength * 0.22), SliceNoise);
float DirectionNoise = frac(
    sin((SliceIndex + BurstTick * 11.0 + RandomSeed * 29.0) * 41.137)
    * 15731.743
);
float Direction = step(0.5, DirectionNoise) * 2.0 - 1.0;
float SliceShift =
    SliceGate * Direction * HorizontalShift * (0.4 + Strength);

float MicroNoise = frac(
    sin((BurstTick + RandomSeed * 17.0) * 91.731) * 13758.991
);
float MicroShift =
    (MicroNoise * 2.0 - 1.0) * HorizontalShift * 0.08 * Strength;

float2 ShiftedUV = saturate(
    BaseUV + float2(SliceShift + MicroShift, 0.0)
);
float4 MainSample =
    Texture2DSample(Texture, TextureSampler, ShiftedUV);

// 버스트 중에만 적색/청색 채널을 반대 방향으로 벌린다.
float ChannelOffset = RGBSplit * Strength;
float2 RedUV = saturate(
    ShiftedUV + float2(ChannelOffset, 0.0)
);
float2 BlueUV = saturate(
    ShiftedUV - float2(ChannelOffset, 0.0)
);
float4 RedSample = Texture2DSample(Texture, TextureSampler, RedUV);
float4 BlueSample = Texture2DSample(Texture, TextureSampler, BlueUV);

float3 SplitColor = float3(
    RedSample.r,
    MainSample.g,
    BlueSample.b
);
float3 FinalColor = lerp(MainSample.rgb, SplitColor, Strength);

// 버스트 순간의 짧은 밝기 떨림.
float FlickerNoise = frac(
    sin((BurstTick + SliceIndex * 3.0 + RandomSeed * 43.0) * 53.121)
    * 43758.5453
);
float Flicker =
    lerp(1.0, 1.0 - saturate(FlickerStrength), step(0.58, FlickerNoise) * Burst);

float GhostAlpha = max(RedSample.a, BlueSample.a);
float FinalAlpha = max(
    MainSample.a,
    GhostAlpha * Strength * 0.85
);
float Opacity = saturate(FinalOpacity);

return float4(
    FinalColor * Flicker * Opacity,
    FinalAlpha * Opacity
);
"""

MATERIAL = None


def log(message):
    unreal.log(f"[TitleLogoGlitch] {message}")


def log_error(message):
    unreal.log_error(f"[TitleLogoGlitch] {message}")


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
        group="LS/Title Logo",
    )


def build_graph():
    texture = expression(
        unreal.MaterialExpressionTextureObjectParameter,
        -1300,
        -400,
        parameter_name="Texture",
        group="LS/Title Logo",
    )
    uv = expression(unreal.MaterialExpressionTextureCoordinate, -1300, -250)
    time = expression(unreal.MaterialExpressionTime, -1300, -100)

    parameters = {
        "GlitchStrength": scalar_parameter("GlitchStrength", 1.0, 50),
        "HorizontalShift": scalar_parameter("HorizontalShift", 0.012, 150),
        "RGBSplit": scalar_parameter("RGBSplit", 0.004, 250),
        "SliceDensity": scalar_parameter("SliceDensity", 72.0, 350),
        "BurstRate": scalar_parameter("BurstRate", 10.0, 500),
        "BurstThreshold": scalar_parameter("BurstThreshold", 0.985, 600),
        "FlickerStrength": scalar_parameter("FlickerStrength", 0.32, 700),
        "RandomSeed": scalar_parameter("RandomSeed", 2.0, 800),
        "FinalOpacity": scalar_parameter("FinalOpacity", 1.0, 900),
        "ManualBurst": scalar_parameter("ManualBurst", 0.0, 1050),
    }

    input_names = [
        "Texture",
        "UV",
        "TimeValue",
        "GlitchStrength",
        "HorizontalShift",
        "RGBSplit",
        "SliceDensity",
        "BurstRate",
        "BurstThreshold",
        "FlickerStrength",
        "RandomSeed",
        "FinalOpacity",
        "ManualBurst",
    ]
    custom = expression(
        unreal.MaterialExpressionCustom,
        -400,
        100,
        description="LS 타이틀 로고 간헐 RGB 글리치",
        code=GLITCH_HLSL,
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
        50,
        r=True,
        g=True,
        b=True,
        a=False,
    )
    alpha = expression(
        unreal.MaterialExpressionComponentMask,
        0,
        200,
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


def create_instance(name, path, manual_burst):
    if EAL.does_asset_exist(path):
        log(f"이미 존재하는 인스턴스: {path}")
        return

    instance = ASSET_TOOLS.create_asset(
        name,
        PACKAGE_PATH,
        unreal.MaterialInstanceConstant,
        unreal.MaterialInstanceConstantFactoryNew(),
    )
    if not instance:
        log_error(f"머티리얼 인스턴스 생성 실패: {path}")
        return

    MEL.set_material_instance_parent(instance, MATERIAL)
    MEL.set_material_instance_scalar_parameter_value(
        instance, "ManualBurst", manual_burst
    )
    EAL.save_asset(path, only_if_is_dirty=False)
    log(f"인스턴스 생성 완료: {path}")


def main():
    global MATERIAL

    if EAL.does_asset_exist(MATERIAL_PATH):
        MATERIAL = EAL.load_asset(MATERIAL_PATH)
        log(f"기존 머티리얼 사용: {MATERIAL_PATH}")
    else:
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

    create_instance(INSTANCE_NAME, INSTANCE_PATH, 0.0)
    create_instance(PREVIEW_INSTANCE_NAME, PREVIEW_INSTANCE_PATH, 1.0)


if __name__ == "__main__":
    main()
