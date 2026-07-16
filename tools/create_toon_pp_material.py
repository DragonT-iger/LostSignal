"""
툰 셰이딩 포스트 프로세스 머티리얼(PP_ToonShading) 생성 스크립트 (언리얼 에디터 Python 전용).

원리: 디퍼드 GBuffer를 이용해 SceneColor / BaseColor 로 라이팅 항을 역산하고,
휘도를 계단화(quantize)한 뒤 다시 곱해 되돌린다. 에셋 머티리얼(Default Lit)은
손대지 않고 그대로 두면 되므로, 복잡한 외부 에셋 머티리얼도 툰화할 수 있다.
CustomStencil로 마스킹해 지정한 오브젝트(몬스터 등)에만 선별 적용한다.

에디터 실행:   py "<프로젝트>/tools/create_toon_pp_material.py"
헤드리스 실행: UnrealEditor-Cmd.exe <uproject> -ExecutePythonScript="<this file>"

생성 에셋:
  /Game/LostSignal/Materials/PostProcess/PP_ToonShading     (마스터)
  /Game/LostSignal/Materials/PostProcess/MI_PP_ToonShading  (인스턴스 — PPV에 등록용)

실행 후 수동 설정 (스크립트가 못 하는 부분):
  - 레벨의 PostProcessVolume(Infinite Extent) > Rendering Features > Post Process
    Materials 배열에 MI_PP_ToonShading 추가
  - Custom Depth-Stencil Pass는 이미 활성(DefaultEngine.ini r.CustomDepth=3),
    캐릭터 CustomDepth는 ALSCharacterBase 생성자에서 이미 일괄 처리(스텐실 1)

프로젝트 스텐실 값 할당 현황 (충돌 금지):
  1   — 모든 캐릭터 (LSCharacterBase 생성자, 플레이어+몬스터. M_PP_PlayerVision이 소비)
  10  — 비전 시스템 정적 메시 마커 (LSStencilMarkerComponent 기본값)
  251/252/253 — 상호작용 마커 (RaidEntrance/Extraction/ChipStation·LobbyStorage)

머티리얼 파라미터:
  Steps         — 음영 단계 수 (기본 3)
  StencilValue  — 기본 1 = 캐릭터만 툰화. 0이면 CustomStencil>0 전부(비전 벽 10,
                  마커 251~253까지 걸리므로 사용 금지), 그 외엔 일치 값만
  MinBrightness — 가장 어두운 단계의 하한 (완전 암부 뭉개짐 방지)
  SpecThreshold — 이 밝기를 넘는 라이팅을 스펙큘러로 간주 (디퓨즈 최대 밝기 근사.
                  주광 세기에 맞춰 튜닝 — 낮으면 밝은 면이 하이라이트로 오검출)
  SpecIntensity — 하이라이트 단계가 디퓨즈 위에 얹는 밝기. 0이면 하이라이트 끔
                  (임계값 초과분이 최상단 디퓨즈 단계로 눌림)

재실행 동작: PP_ToonShading이 이미 있으면 노드만 비우고 재빌드한다 (MI 부모 유지).
"""

import unreal

MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
ASSET_TOOLS = unreal.AssetToolsHelpers.get_asset_tools()

PKG_PATH = "/Game/LostSignal/Materials/PostProcess"
MAT_NAME = "PP_ToonShading"
MI_NAME = "MI_PP_ToonShading"
MAT_PATH = f"{PKG_PATH}/{MAT_NAME}"
MI_PATH = f"{PKG_PATH}/{MI_NAME}"

# 엔진 버전에 따라 Blendable Location enum 이름이 다르다 (5.4+ / 이전)
BLENDABLE_LOCATION_CANDIDATES = [
    "BL_SCENE_COLOR_AFTER_DOF",  # UE 5.4+ (구 Before Tonemapping에 해당)
    "BL_BEFORE_TONEMAPPING",     # UE 5.3 이하
]

# 라이팅 항 역산 → 휘도 계단화(+임계값 초과분은 평평한 스펙큘러 하이라이트) → 재조합
# → 스텐실 마스크로 원본과 lerp
# 스펙큘러는 SceneColor에 이미 합산돼 들어와 정확한 분리가 불가하므로,
# "디퓨즈로 설명되는 최대 밝기(SpecThreshold)를 넘는 구간 = 스펙큘러"로 근사한다.
# unlit/이미시브 픽셀은 GBuffer BaseColor가 거의 0이라 라이팅 역산이 무의미
# (역산값이 폭주해 SpecThreshold 클램프에 눌려 어두워짐) → 툰화에서 제외한다.
TOON_HLSL = """
float3 scene = SceneColor.rgb;
float3 albedo = max(BaseColor.rgb, 0.001);
float3 lighting = scene / albedo;
float intensity = dot(lighting, float3(0.2126, 0.7152, 0.0722));
float steps = max(Steps, 1.0);
float diffuse = min(intensity, SpecThreshold);
float band = floor(diffuse * steps + 0.5) / steps;
band = max(band, MinBrightness);
float highlight = step(SpecThreshold, intensity) * SpecIntensity;
float scale = (band + highlight) / max(intensity, 0.0001);
float3 toon = scene * scale;
float stencil = Stencil.r;
float mask = (StencilValue < 0.5)
    ? saturate(stencil)
    : (abs(stencil - StencilValue) < 0.5 ? 1.0 : 0.0);
float albedoLum = dot(BaseColor.rgb, float3(0.2126, 0.7152, 0.0722));
mask *= step(0.01, albedoLum);
return lerp(scene, toon, mask);
"""

MAT = None


def log(msg):
    unreal.log(f"[ToonPP] {msg}")


def log_warn(msg):
    unreal.log_warning(f"[ToonPP] {msg}")


def log_error(msg):
    unreal.log_error(f"[ToonPP] {msg}")


def expr(cls, x, y, **props):
    node = MEL.create_material_expression(MAT, cls, x, y)
    for key, value in props.items():
        node.set_editor_property(key, value)
    return node


def connect(src, out_name, dst, in_names):
    if isinstance(in_names, str):
        in_names = [in_names]
    for name in in_names:
        if MEL.connect_material_expressions(src, out_name, dst, name):
            return
    log_error(f"핀 연결 실패: {src.get_name()}({out_name}) -> {dst.get_name()}{in_names}")


def scalar(name, default, x, y):
    return expr(unreal.MaterialExpressionScalarParameter, x, y,
                parameter_name=name, default_value=default, group="LS Toon")


def scene_texture(tex_id, x, y):
    return expr(unreal.MaterialExpressionSceneTexture, x, y, scene_texture_id=tex_id)


def custom_input(name):
    ci = unreal.CustomInput()
    ci.set_editor_property("input_name", name)
    return ci


def set_blendable_location(mat):
    for name in BLENDABLE_LOCATION_CANDIDATES:
        loc = getattr(unreal.BlendableLocation, name, None)
        if loc is not None:
            mat.set_editor_property("blendable_location", loc)
            log(f"Blendable Location = {name}")
            return
    log_warn("Blendable Location enum을 찾지 못함 — 머티리얼에서 직접 "
             "'Scene Color After DOF'(구 Before Tonemapping)로 설정할 것")


def build_graph():
    scene = scene_texture(unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0, -900, 0)
    base = scene_texture(unreal.SceneTextureId.PPI_BASE_COLOR, -900, 200)
    stencil = scene_texture(unreal.SceneTextureId.PPI_CUSTOM_STENCIL, -900, 400)
    steps = scalar("Steps", 3.0, -900, 600)
    stencil_value = scalar("StencilValue", 1.0, -900, 700)
    min_brightness = scalar("MinBrightness", 0.05, -900, 800)
    spec_threshold = scalar("SpecThreshold", 1.2, -900, 900)
    spec_intensity = scalar("SpecIntensity", 0.4, -900, 1000)

    toon = expr(unreal.MaterialExpressionCustom, -450, 200,
                description="LS Toon Quantize",
                code=TOON_HLSL,
                output_type=unreal.CustomMaterialOutputType.CMOT_FLOAT3,
                inputs=[custom_input("SceneColor"),
                        custom_input("BaseColor"),
                        custom_input("Stencil"),
                        custom_input("Steps"),
                        custom_input("StencilValue"),
                        custom_input("MinBrightness"),
                        custom_input("SpecThreshold"),
                        custom_input("SpecIntensity")])

    connect(scene, "Color", toon, "SceneColor")
    connect(base, "Color", toon, "BaseColor")
    connect(stencil, "Color", toon, "Stencil")
    connect(steps, "", toon, "Steps")
    connect(stencil_value, "", toon, "StencilValue")
    connect(min_brightness, "", toon, "MinBrightness")
    connect(spec_threshold, "", toon, "SpecThreshold")
    connect(spec_intensity, "", toon, "SpecIntensity")

    if not MEL.connect_material_property(toon, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR):
        log_error("EmissiveColor 출력 연결 실패")


def create_instance():
    if EAL.does_asset_exist(MI_PATH):
        log(f"{MI_PATH} 가 이미 존재해 인스턴스 생성은 건너뜀")
        return
    mi = ASSET_TOOLS.create_asset(MI_NAME, PKG_PATH, unreal.MaterialInstanceConstant,
                                  unreal.MaterialInstanceConstantFactoryNew())
    if not mi:
        log_error("머티리얼 인스턴스 생성 실패")
        return
    MEL.set_material_instance_parent(mi, MAT)
    EAL.save_asset(MI_PATH, only_if_is_dirty=False)
    log(f"생성 완료: {MI_PATH}")


def main():
    global MAT
    if EAL.does_asset_exist(MAT_PATH):
        # 삭제 후 재생성하면 MI의 부모 참조가 깨지므로, 노드만 비우고 제자리에서 재빌드한다
        MAT = EAL.load_asset(MAT_PATH)
        MEL.delete_all_material_expressions(MAT)
        log(f"{MAT_PATH} 가 이미 존재 — 노드를 비우고 재빌드")
    else:
        MAT = ASSET_TOOLS.create_asset(MAT_NAME, PKG_PATH, unreal.Material,
                                       unreal.MaterialFactoryNew())
    if not MAT:
        log_error("머티리얼 에셋 생성/로드 실패")
        return
    MAT.set_editor_property("material_domain", unreal.MaterialDomain.MD_POST_PROCESS)
    set_blendable_location(MAT)
    build_graph()
    MEL.recompile_material(MAT)
    EAL.save_asset(MAT_PATH, only_if_is_dirty=False)
    log(f"생성 완료: {MAT_PATH}")
    create_instance()
    log("남은 수동 설정: Custom Depth-Stencil Pass 활성화 / 메시 bRenderCustomDepth / PPV 등록 (파일 상단 주석 참고)")


if __name__ == "__main__":
    main()
