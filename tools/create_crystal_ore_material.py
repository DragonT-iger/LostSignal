"""
크리스탈 광석 마스터 머티리얼(M_CrystalOre) 생성 스크립트 (언리얼 에디터 Python 전용).

구조: Opaque + BumpOffset 패럴랙스 가짜 내부 레이어 4장
  - 크랙(-4.0) / 스트림(-5.0 + Panner) / 스펙클(+2.0) / 커버처(+0.2)
  - 프레넬 페이드, 로컬 Z 그라디언트(팁 발광), 오브젝트 위치 기반 펄스로 Emissive 변조

에디터 실행:   py "<프로젝트>/tools/create_crystal_ore_material.py"
헤드리스 실행: UnrealEditor-Cmd.exe <uproject> -ExecutePythonScript="<this file>"

생성 후 머티리얼 인스턴스에서 텍스처 3장을 교체한다 (전부 sRGB 꺼진 리니어 권장):
  NormalTexture    — 베이크 노멀
  CurvatureTexture — 베이크 커버처
  PackedNoise      — R: 크랙 / G: 스트림 / B: 스펙클
"""

import unreal

MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
ASSET_TOOLS = unreal.AssetToolsHelpers.get_asset_tools()

PKG_PATH = "/Game/LostSignal/Materials"
ASSET_NAME = "M_CrystalOre"
FULL_PATH = f"{PKG_PATH}/{ASSET_NAME}"

TEX_FLAT_NORMAL = "/Engine/EngineMaterials/FlatNormal"
TEX_NOISE_CANDIDATES = [
    "/Engine/EngineMaterials/Good64x64TilingNoiseHighFreq",
    "/Engine/EngineResources/DefaultTexture",
]
FN_FLATTEN_NORMAL = "/Engine/Functions/Engine_MaterialFunctions01/Texturing/FlattenNormal"
FN_BBOX_UVW_CANDIDATES = [
    "/Engine/Functions/Engine_MaterialFunctions01/Coordinates/BoundingBoxBased_0-1_UVW",
    "/Engine/Functions/Engine_MaterialFunctions02/Utility/BoundingBoxBased_0-1_UVW",
    "/Engine/Functions/Engine_MaterialFunctions01/Texturing/BoundingBoxBased_0-1_UVW",
]

MAT = None


def log(msg):
    unreal.log(f"[CrystalOre] {msg}")


def log_warn(msg):
    unreal.log_warning(f"[CrystalOre] {msg}")


def log_error(msg):
    unreal.log_error(f"[CrystalOre] {msg}")


def first_existing(paths):
    for p in paths:
        if EAL.does_asset_exist(p):
            return p
    return None


def expr(cls, x, y, **props):
    node = MEL.create_material_expression(MAT, cls, x, y)
    for key, value in props.items():
        node.set_editor_property(key, value)
    return node


def connect(src, out_name, dst, in_names):
    # 엔진 버전에 따라 입력 핀 이름이 다를 수 있어 후보를 순서대로 시도한다
    if isinstance(in_names, str):
        in_names = [in_names]
    for name in in_names:
        if MEL.connect_material_expressions(src, out_name, dst, name):
            return
    log_error(f"핀 연결 실패: {src.get_name()}({out_name}) -> {dst.get_name()}{in_names}")


def to_property(src, out_name, prop):
    if not MEL.connect_material_property(src, out_name, prop):
        log_error(f"머티리얼 출력 연결 실패: {prop}")


def scalar(name, default, group, x, y):
    return expr(unreal.MaterialExpressionScalarParameter, x, y,
                parameter_name=name, default_value=default, group=group)


def vector(name, rgb, group, x, y):
    return expr(unreal.MaterialExpressionVectorParameter, x, y,
                parameter_name=name, group=group,
                default_value=unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))


def texture_param(name, tex_path, group, x, y):
    # sampler_type은 지정한 텍스처에 맞춰 자동 설정되도록 texture만 할당한다
    node = expr(unreal.MaterialExpressionTextureSampleParameter2D, x, y,
                parameter_name=name, group=group)
    if tex_path:
        node.set_editor_property("texture", unreal.load_asset(tex_path))
    return node


def mul(a, a_out, b, b_out, x, y):
    node = expr(unreal.MaterialExpressionMultiply, x, y)
    connect(a, a_out, node, "A")
    connect(b, b_out, node, "B")
    return node


def add(a, a_out, b, b_out, x, y):
    node = expr(unreal.MaterialExpressionAdd, x, y)
    connect(a, a_out, node, "A")
    connect(b, b_out, node, "B")
    return node


def lerp(a, a_out, b, b_out, alpha, alpha_out, x, y):
    node = expr(unreal.MaterialExpressionLinearInterpolate, x, y)
    connect(a, a_out, node, "A")
    connect(b, b_out, node, "B")
    connect(alpha, alpha_out, node, "Alpha")
    return node


def power(base, base_out, exp_node, x, y):
    node = expr(unreal.MaterialExpressionPower, x, y)
    connect(base, base_out, node, "Base")
    connect(exp_node, "", node, ["Exponent", "Exp"])
    return node


def one_minus(src, out_name, x, y):
    node = expr(unreal.MaterialExpressionOneMinus, x, y)
    connect(src, out_name, node, ["Input", ""])
    return node


def saturate(src, out_name, x, y):
    node = expr(unreal.MaterialExpressionSaturate, x, y)
    connect(src, out_name, node, ["Input", ""])
    return node


def const(value, x, y):
    return expr(unreal.MaterialExpressionConstant, x, y, r=value)


def bump_offset(coord, height_param, ratio_param, x, y):
    node = expr(unreal.MaterialExpressionBumpOffset, x, y)
    connect(coord, "", node, "Coordinate")
    connect(height_param, "", node, "Height")
    connect(ratio_param, "", node, "HeightRatioInput")
    return node


def build_crack_layer(tc, noise_path):
    y, grp = 0, "01 Crack"
    off = scalar("Cracks_BumpOffset", -4.0, grp, -2450, y + 80)
    ratio = scalar("Cracks_BumpRatio", 0.04, grp, -2450, y + 160)
    bo = bump_offset(tc, off, ratio, -2150, y)
    ts = texture_param("PackedNoise", noise_path, grp, -1900, y)
    connect(bo, "", ts, ["UVs", "Coordinates"])
    contrast = scalar("Fade_Contrast_Cracks", 1.5, grp, -1900, y + 240)
    pw = power(ts, "R", contrast, -1650, y)
    inten = scalar("Crack_Intensity", 2.0, grp, -1650, y + 160)
    col = vector("Crack_Color", (0.5, 0.15, 1.0), grp, -1450, y + 160)
    out = mul(mul(pw, "", inten, "", -1450, y), "", col, "", -1250, y)
    return out, ts


def build_stream_layer(tc, noise_path):
    y, grp = 350, "02 Stream"
    pan = expr(unreal.MaterialExpressionPanner, -2650, y, speed_x=0.01, speed_y=0.005)
    connect(tc, "", pan, "Coordinate")
    off = scalar("Stream_BumpOffset", -5.0, grp, -2450, y + 80)
    ratio = scalar("Stream_BumpRatio", 0.04, grp, -2450, y + 160)
    bo = bump_offset(pan, off, ratio, -2150, y)
    ts = texture_param("PackedNoise", noise_path, grp, -1900, y)
    connect(bo, "", ts, ["UVs", "Coordinates"])
    inten = scalar("Stream_Intensity", 1.3, grp, -1650, y + 160)
    col = vector("Stream_Color", (1.0, 0.25, 1.6), grp, -1450, y + 160)
    return mul(mul(ts, "G", inten, "", -1450, y), "", col, "", -1250, y)


def build_speckle_layer(noise_path):
    y, grp = 700, "03 Speckle"
    tc = expr(unreal.MaterialExpressionTextureCoordinate, -2650, y, u_tiling=4.0, v_tiling=4.0)
    off = scalar("Speckle_BumpOffset", 2.0, grp, -2450, y + 80)
    ratio = scalar("Speckle_BumpRatio", 0.04, grp, -2450, y + 160)
    bo = bump_offset(tc, off, ratio, -2150, y)
    ts = texture_param("PackedNoise", noise_path, grp, -1900, y)
    connect(bo, "", ts, ["UVs", "Coordinates"])
    contrast = scalar("Fade_Contrast_Speckles", 3.0, grp, -1900, y + 240)
    pw = power(ts, "B", contrast, -1650, y)
    col = vector("Speckle_Color", (1.5, 1.5, 2.0), grp, -1450, y + 160)
    return mul(pw, "", col, "", -1250, y)


def build_curvature_layer(tc, noise_path):
    y, grp = 1050, "04 Curvature"
    off = scalar("Curvature_BumpOffset", 0.2, grp, -2450, y + 80)
    ratio = scalar("Curvature_BumpRatio", 0.04, grp, -2450, y + 160)
    bo = bump_offset(tc, off, ratio, -2150, y)
    ts = texture_param("CurvatureTexture", noise_path, grp, -1900, y)
    connect(bo, "", ts, ["UVs", "Coordinates"])
    inten = scalar("Curvature_Intensity", 0.6, grp, -1650, y + 160)
    col = vector("Curvature_Color", (1.0, 0.4, 1.8), grp, -1450, y + 160)
    return mul(mul(ts, "R", inten, "", -1450, y), "", col, "", -1250, y)


def build_fresnel():
    y, grp = 1400, "05 Fresnel"
    exp = scalar("Fresnel_Exponent", 2.0, grp, -2450, y)
    base = scalar("BaseReflectFraction", 0.2, grp, -2450, y + 80)
    fres = expr(unreal.MaterialExpressionFresnel, -2150, y)
    connect(exp, "", fres, "ExponentIn")
    connect(base, "", fres, "BaseReflectFractionIn")
    # 스치는 각도에서 내부 레이어를 감쇠시키는 마스크
    fade_amt = scalar("Interior_Fade", 0.5, grp, -2150, y + 160)
    fade = one_minus(mul(fres, "", fade_amt, "", -1900, y), "", -1650, y)
    # 가장자리 림 발광 (페이드 없이 그대로 더해짐)
    rim_int = scalar("Rim_Intensity", 0.3, grp, -1900, y + 240)
    rim_col = vector("Rim_Color", (0.9, 0.6, 1.4), grp, -1650, y + 240)
    rim = mul(mul(fres, "", rim_int, "", -1650, y + 160), "", rim_col, "", -1450, y + 160)
    return fade, rim


def build_gradient():
    y, grp = 1750, "06 Gradient"
    fn_path = first_existing(FN_BBOX_UVW_CANDIDATES)
    if not fn_path:
        log_warn("BoundingBoxBased_0-1_UVW 함수를 찾지 못해 그라디언트를 상수 1로 대체함")
        return const(1.0, -1450, y)
    call = expr(unreal.MaterialExpressionMaterialFunctionCall, -2450, y)
    call.set_editor_property("material_function", unreal.load_asset(fn_path))
    mask = expr(unreal.MaterialExpressionComponentMask, -2250, y, r=False, g=False, b=True, a=False)
    connect(call, "", mask, ["Input", ""])
    grad_pow = scalar("Local_Gradient_Power", 2.0, grp, -2250, y + 120)
    sat = saturate(power(mask, "", grad_pow, -2050, y), "", -1850, y)
    top = scalar("Top_Gradient_Multiplier", 3.0, grp, -1850, y + 120)
    return lerp(const(1.0, -1850, y + 240), "", top, "", sat, "", -1650, y)


def build_pulse():
    y, grp = 2100, "07 Pulse"
    time = expr(unreal.MaterialExpressionTime, -2650, y)
    speed = scalar("Pulse_Speed", 0.4, grp, -2650, y + 90)
    t = mul(time, "", speed, "", -2450, y)
    # 오브젝트 월드 위치로 위상을 어긋내 인스턴스마다 다른 타이밍으로 맥동
    objpos = expr(unreal.MaterialExpressionObjectPositionWS, -2650, y + 180)
    mask = expr(unreal.MaterialExpressionComponentMask, -2450, y + 180, r=True, g=False, b=False, a=False)
    connect(objpos, "", mask, ["Input", ""])
    pulsation = scalar("ObjectPosition_Pulsation", 0.025, grp, -2450, y + 280)
    phase = mul(mask, "", pulsation, "", -2250, y + 180)
    sine = expr(unreal.MaterialExpressionSine, -2050, y)
    connect(add(t, "", phase, "", -2250, y), "", sine, ["Input", ""])
    amt = scalar("Pulse_Amount", 0.15, grp, -2050, y + 120)
    wobble = mul(sine, "", amt, "", -1850, y)
    return add(const(1.0, -1850, y + 120), "", wobble, "", -1650, y)


def build_emissive(crack, stream, speckle, curv, fade, rim, grad, pulse):
    y, grp = 500, "08 Emissive"
    interior = add(add(crack, "", stream, "", -1050, y), "", speckle, "", -900, y)
    faded = mul(interior, "", fade, "", -750, y)
    surface = add(curv, "", rim, "", -900, y + 160)
    base = add(faded, "", surface, "", -650, y)
    e_pow = scalar("Emissive_Power", 1.1, grp, -650, y + 160)
    pw = power(base, "", e_pow, -500, y)
    e_int = scalar("Emissive_Intensity", 20.0, grp, -500, y + 160)
    boosted = mul(pw, "", e_int, "", -380, y)
    final = mul(mul(boosted, "", grad, "", -280, y), "", pulse, "", -180, y)
    to_property(final, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)


def build_base_color(crack_ts):
    y, grp = -450, "09 BaseColor"
    c_main = vector("Crystal_Color", (0.10, 0.03, 0.20), grp, -1050, y)
    c_deep = vector("Crystal_Color_Deep", (0.02, 0.01, 0.06), grp, -1050, y + 200)
    mix = lerp(c_main, "", c_deep, "", crack_ts, "R", -800, y)
    desat = expr(unreal.MaterialExpressionDesaturation, -600, y)
    connect(mix, "", desat, ["Input", ""])
    desat_amt = scalar("Desaturation_Amount", 0.0, grp, -800, y + 200)
    connect(desat_amt, "", desat, "Fraction")
    to_property(desat, "", unreal.MaterialProperty.MP_BASE_COLOR)


def build_surface(crack_ts):
    y, grp = -900, "10 Surface"
    n_ts = texture_param("NormalTexture", TEX_FLAT_NORMAL, grp, -1050, y)
    if EAL.does_asset_exist(FN_FLATTEN_NORMAL):
        flatten = expr(unreal.MaterialExpressionMaterialFunctionCall, -700, y)
        flatten.set_editor_property("material_function", unreal.load_asset(FN_FLATTEN_NORMAL))
        connect(n_ts, "", flatten, "Normal")
        flatness = scalar("Normal_Flatness", 0.0, grp, -1050, y + 240)
        connect(flatness, "", flatten, "Flatness")
        to_property(flatten, "", unreal.MaterialProperty.MP_NORMAL)
    else:
        to_property(n_ts, "", unreal.MaterialProperty.MP_NORMAL)
    # 크랙이 지나가는 자리는 표면을 살짝 거칠게
    r_base = scalar("Roughness_Base", 0.15, grp, -700, y + 360)
    r_crack = scalar("Roughness_Crack", 0.35, grp, -700, y + 440)
    rough = saturate(
        add(r_base, "", mul(crack_ts, "R", r_crack, "", -500, y + 400), "", -380, y + 380),
        "", -280, y + 380)
    to_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    spec = scalar("Specular_Amount", 0.6, grp, -280, y + 500)
    to_property(spec, "", unreal.MaterialProperty.MP_SPECULAR)


def main():
    global MAT
    if EAL.does_asset_exist(FULL_PATH):
        log_error(f"{FULL_PATH} 가 이미 존재함. 삭제하거나 ASSET_NAME을 바꾼 뒤 다시 실행.")
        return
    MAT = ASSET_TOOLS.create_asset(ASSET_NAME, PKG_PATH, unreal.Material, unreal.MaterialFactoryNew())
    if not MAT:
        log_error("머티리얼 에셋 생성 실패")
        return
    noise_path = first_existing(TEX_NOISE_CANDIDATES)
    if not noise_path:
        log_warn("기본 노이즈 텍스처를 찾지 못함 — 텍스처 파라미터가 비어 컴파일 에러가 날 수 있음")
    tc = expr(unreal.MaterialExpressionTextureCoordinate, -2900, 0)
    crack, crack_ts = build_crack_layer(tc, noise_path)
    stream = build_stream_layer(tc, noise_path)
    speckle = build_speckle_layer(noise_path)
    curv = build_curvature_layer(tc, noise_path)
    fade, rim = build_fresnel()
    grad = build_gradient()
    pulse = build_pulse()
    build_emissive(crack, stream, speckle, curv, fade, rim, grad, pulse)
    build_base_color(crack_ts)
    build_surface(crack_ts)
    MEL.recompile_material(MAT)
    EAL.save_asset(FULL_PATH, only_if_is_dirty=False)
    log(f"생성 완료: {FULL_PATH}")


if __name__ == "__main__":
    main()
