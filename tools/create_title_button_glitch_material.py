"""타이틀 버튼용 Hover 지속/평상시 간헐 무지개 글리치 UI 머티리얼을 생성한다."""

import unreal


MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
ASSET_TOOLS = unreal.AssetToolsHelpers.get_asset_tools()

PACKAGE_PATH = "/Game/LostSignal/Sandbox/YoonSeok/Code"
MATERIAL_NAME = "M_UI_TitleButtonGlitch"
INSTANCE_NAME = "MI_UI_TitleButtonGlitch"
PREVIEW_INSTANCE_NAME = "MI_UI_TitleButtonGlitch_Preview"
MATERIAL_PATH = f"{PACKAGE_PATH}/{MATERIAL_NAME}"
INSTANCE_PATH = f"{PACKAGE_PATH}/{INSTANCE_NAME}"
PREVIEW_INSTANCE_PATH = f"{PACKAGE_PATH}/{PREVIEW_INSTANCE_NAME}"

GLITCH_HLSL = r"""
float2 BaseUV = saturate(UV);

// 평상시에는 드물게 한 틱만 켜지고, Hover/Focus 중에는 계속 켜진다.
float SafeBurstRate = max(IdleBurstRate, 0.01);
float BurstTick = floor(TimeValue * SafeBurstRate);
float BurstNoise = frac(
    sin((BurstTick + RandomSeed * 71.0) * 12.9898) * 43758.5453
);
float IdleBurst = step(saturate(IdleBurstThreshold), BurstNoise);
float Active = max(
    max(IdleBurst, saturate(HoverAmount)),
    saturate(ManualBurst)
);
float Strength = saturate(GlitchStrength) * Active;

// 가로 조각별로 방향이 다른 수평 이동을 만든다.
float SafeSliceDensity = max(SliceDensity, 1.0);
float SliceIndex = floor(BaseUV.y * SafeSliceDensity);
float SliceNoise = frac(
    sin((SliceIndex + BurstTick * 23.0 + RandomSeed * 131.0) * 78.233)
    * 43758.5453
);
float SliceGate = step(1.0 - (0.12 + Strength * 0.28), SliceNoise);
float DirectionNoise = frac(
    sin((SliceIndex + BurstTick * 11.0 + RandomSeed * 29.0) * 41.137)
    * 15731.743
);
float Direction = step(0.5, DirectionNoise) * 2.0 - 1.0;
float SliceShift =
    SliceGate * Direction * HorizontalShift * (0.45 + Strength) * Active;

float Motion =
    sin(TimeValue * 17.0 + SliceIndex * 1.73 + RandomSeed) * 0.5 + 0.5;
float HoverMotion = lerp(1.0, 0.6 + Motion * 0.8, saturate(HoverAmount));
float2 ShiftedUV = saturate(
    BaseUV + float2(SliceShift * HoverMotion, 0.0)
);
float4 MainSample =
    Texture2DSample(Texture, TextureSampler, ShiftedUV);

// 좌우 복제본에 시간에 따라 변하는 무지개 팔레트를 곱한다.
float GhostOffset = RGBSplit * Strength;
float4 GhostLeft = Texture2DSample(
    Texture,
    TextureSampler,
    saturate(ShiftedUV - float2(GhostOffset, 0.0))
);
float4 GhostRight = Texture2DSample(
    Texture,
    TextureSampler,
    saturate(ShiftedUV + float2(GhostOffset, 0.0))
);

float HuePhase = TimeValue * RainbowSpeed + RandomSeed * 0.13;
float3 RainbowLeft = 0.5 + 0.5 * cos(
    6.2831853 * (HuePhase + float3(0.0, 0.3333333, 0.6666667))
);
float3 RainbowRight = 0.5 + 0.5 * cos(
    6.2831853 * (HuePhase + 0.5 + float3(0.0, 0.3333333, 0.6666667))
);

float3 GhostColor =
    GhostLeft.rgb * RainbowLeft +
    GhostRight.rgb * RainbowRight;
float3 GlitchedColor = saturate(
    MainSample.rgb + GhostColor * RainbowAmount * Strength
);

float FlickerNoise = frac(
    sin((BurstTick + SliceIndex * 3.0 + RandomSeed * 43.0) * 53.121)
    * 43758.5453
);
float Flicker = lerp(
    1.0,
    1.0 - saturate(FlickerStrength),
    step(0.62, FlickerNoise) * Active
);

float GhostAlpha = max(GhostLeft.a, GhostRight.a);
float FinalAlpha = max(
    MainSample.a,
    GhostAlpha * Strength * saturate(RainbowAmount)
);
float Opacity = saturate(FinalOpacity);

return float4(
    GlitchedColor * Flicker * Opacity,
    FinalAlpha * Opacity
);
"""

MATERIAL = None


def log(message):
    unreal.log(f"[TitleButtonGlitch] {message}")


def log_error(message):
    unreal.log_error(f"[TitleButtonGlitch] {message}")


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
        group="LS/Title Button",
    )


def build_graph():
    texture = expression(
        unreal.MaterialExpressionTextureObjectParameter,
        -1300,
        -450,
        parameter_name="Texture",
        group="LS/Title Button",
    )
    uv = expression(unreal.MaterialExpressionTextureCoordinate, -1300, -300)
    time = expression(unreal.MaterialExpressionTime, -1300, -150)

    parameters = {
        "HoverAmount": scalar_parameter("HoverAmount", 0.0, 0),
        "GlitchStrength": scalar_parameter("GlitchStrength", 1.0, 100),
        "HorizontalShift": scalar_parameter("HorizontalShift", 0.018, 200),
        "RGBSplit": scalar_parameter("RGBSplit", 0.007, 300),
        "SliceDensity": scalar_parameter("SliceDensity", 56.0, 400),
        "RainbowAmount": scalar_parameter("RainbowAmount", 0.72, 550),
        "RainbowSpeed": scalar_parameter("RainbowSpeed", 0.45, 650),
        "IdleBurstRate": scalar_parameter("IdleBurstRate", 8.0, 800),
        "IdleBurstThreshold": scalar_parameter("IdleBurstThreshold", 0.995, 900),
        "FlickerStrength": scalar_parameter("FlickerStrength", 0.22, 1000),
        "RandomSeed": scalar_parameter("RandomSeed", 3.0, 1100),
        "FinalOpacity": scalar_parameter("FinalOpacity", 1.0, 1200),
        "ManualBurst": scalar_parameter("ManualBurst", 0.0, 1350),
    }

    input_names = [
        "Texture",
        "UV",
        "TimeValue",
        *parameters.keys(),
    ]
    custom = expression(
        unreal.MaterialExpressionCustom,
        -400,
        150,
        description="LS 타이틀 버튼 무지개 글리치",
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
        100,
        r=True,
        g=True,
        b=True,
        a=False,
    )
    alpha = expression(
        unreal.MaterialExpressionComponentMask,
        0,
        250,
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
        custom_nodes = [
            node
            for node in MEL.get_all_expressions_in_material(MATERIAL)
            if isinstance(node, unreal.MaterialExpressionCustom)
        ]
        if not custom_nodes:
            log_error("Custom 노드를 찾지 못했습니다.")
            return
        custom_nodes[0].set_editor_property("code", GLITCH_HLSL)
        MEL.recompile_material(MATERIAL)
        EAL.save_asset(MATERIAL_PATH, only_if_is_dirty=False)

        instance = EAL.load_asset(INSTANCE_PATH)
        if instance:
            MEL.set_material_instance_scalar_parameter_value(
                instance, "IdleBurstRate", 8.0
            )
            MEL.set_material_instance_scalar_parameter_value(
                instance, "IdleBurstThreshold", 0.995
            )
            MEL.set_material_instance_scalar_parameter_value(
                instance, "ManualBurst", 0.0
            )
            EAL.save_asset(INSTANCE_PATH, only_if_is_dirty=False)

        log(f"기존 머티리얼 수정 완료: {MATERIAL_PATH}")
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
    create_instance(INSTANCE_NAME, INSTANCE_PATH, 0.0)
    create_instance(PREVIEW_INSTANCE_NAME, PREVIEW_INSTANCE_PATH, 1.0)


if __name__ == "__main__":
    main()
