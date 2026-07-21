"""
스텐실 기반 아웃라인 포스트 프로세스 머티리얼(PP_Outline) 생성 스크립트 (언리얼 에디터 Python 전용).

원리: CustomStencil 버퍼를 현재 픽셀 + 상하좌우 이웃 4곳에서 샘플링하고,
"현재 픽셀은 대상(StencilTarget)이 아닌데 이웃 중 하나가 대상"인 곳 = 실루엣
바깥 경계로 판정해 아웃라인 색을 칠한다(디퓨즈/라이팅과 무관, 순수 엣지 검출).
대상 메시는 상황에 따라 SetRenderCustomDepth(true)+SetCustomDepthStencilValue()로
켜고 끈다(예: 루팅박스 상호작용 범위 진입 시).

에디터 실행:   py "<프로젝트>/tools/create_outline_pp_material.py"
헤드리스 실행: UnrealEditor-Cmd.exe <uproject> -ExecutePythonScript="<this file>"

생성 에셋:
  /Game/LostSignal/Materials/PostProcess/PP_Outline     (마스터)
  /Game/LostSignal/Materials/PostProcess/MI_PP_Outline  (인스턴스 — PPV에 등록용)

프로젝트 스텐실 값 할당 현황 (충돌 금지):
  1   — 모든 캐릭터 (LSCharacterBase 생성자. 툰 PP·비전 PP가 소비)
  10  — 비전 시스템 정적 메시 마커 (LSStencilMarkerComponent 기본값)
  20  — 루팅박스 근접 하이라이트 (이 아웃라인 PP 대상, StencilTarget 기본값)
  251/252/253 — 상호작용 마커 (RaidEntrance/Extraction/ChipStation·LobbyStorage)

머티리얼 파라미터:
  OutlineColor     — 아웃라인 색 (기본 청록 하이라이트)
  OutlineThickness — 두께(픽셀). 이웃 샘플 오프셋 거리
  StencilTarget    — 아웃라인을 그릴 CustomStencil 값 (기본 20 = 루팅박스)

실행 후 수동 설정:
  - 레벨 PostProcessVolume(Infinite Extent) > Post Process Materials 배열에 MI_PP_Outline 추가
  - Custom Depth-Stencil Pass는 이미 활성(DefaultEngine.ini r.CustomDepth=3)

재실행 동작: PP_Outline이 이미 있으면 노드만 비우고 재빌드한다 (MI 부모 유지).
"""

import unreal

MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
ASSET_TOOLS = unreal.AssetToolsHelpers.get_asset_tools()

PKG_PATH = "/Game/LostSignal/Materials/PostProcess"
MAT_NAME = "PP_Outline"
MI_NAME = "MI_PP_Outline"
MAT_PATH = f"{PKG_PATH}/{MAT_NAME}"
MI_PATH = f"{PKG_PATH}/{MI_NAME}"

BLENDABLE_LOCATION_CANDIDATES = [
    "BL_SCENE_COLOR_AFTER_DOF",  # UE 5.4+ (구 Before Tonemapping)
    "BL_BEFORE_TONEMAPPING",     # UE 5.3 이하
]

# 상하좌우 4탭. 대각선까지 필요하면 (1,1)(-1,-1)(1,-1)(-1,1)을 추가한다.
NEIGHBOR_DIRS = [(1.0, 0.0), (-1.0, 0.0), (0.0, 1.0), (0.0, -1.0)]

# 이웃 스텐실이 대상이고 현재 픽셀은 대상이 아니면 바깥 경계 → 아웃라인
EDGE_HLSL = """
float t = StencilTarget;
float c = (abs(Center.r - t) < 0.5) ? 1.0 : 0.0;
float n0 = (abs(N0.r - t) < 0.5) ? 1.0 : 0.0;
float n1 = (abs(N1.r - t) < 0.5) ? 1.0 : 0.0;
float n2 = (abs(N2.r - t) < 0.5) ? 1.0 : 0.0;
float n3 = (abs(N3.r - t) < 0.5) ? 1.0 : 0.0;
float neighbor = max(max(n0, n1), max(n2, n3));
return neighbor * (1.0 - c);
"""

MAT = None


def log(msg):
    unreal.log(f"[OutlinePP] {msg}")


def log_warn(msg):
    unreal.log_warning(f"[OutlinePP] {msg}")


def log_error(msg):
    unreal.log_error(f"[OutlinePP] {msg}")


def expr(cls, x, y, **props):
    node = MEL.create_material_expression(MAT, cls, x, y)
    for key, value in props.items():
        node.set_editor_property(key, value)
    return node


def connect(src, out_name, dst, in_names):
    if isinstance(in_names, str):
        in_names = [in_names]
    if isinstance(out_name, str):
        out_names = [out_name]
    else:
        out_names = out_name
    for o in out_names:
        for name in in_names:
            if MEL.connect_material_expressions(src, o, dst, name):
                return
    log_error(f"핀 연결 실패: {src.get_name()}({out_name}) -> {dst.get_name()}{in_names}")


def scalar(name, default, x, y):
    return expr(unreal.MaterialExpressionScalarParameter, x, y,
                parameter_name=name, default_value=default, group="LS Outline")


def scene_texture(tex_id, x, y):
    return expr(unreal.MaterialExpressionSceneTexture, x, y, scene_texture_id=tex_id)


def custom_input(name):
    ci = unreal.CustomInput()
    ci.set_editor_property("input_name", name)
    return ci


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


def set_blendable_location(mat):
    for name in BLENDABLE_LOCATION_CANDIDATES:
        loc = getattr(unreal.BlendableLocation, name, None)
        if loc is not None:
            mat.set_editor_property("blendable_location", loc)
            log(f"Blendable Location = {name}")
            return
    log_warn("Blendable Location enum을 찾지 못함 — 머티리얼에서 직접 "
             "'Scene Color After DOF'(구 Before Tonemapping)로 설정할 것")


def sample_stencil_at(viewport_uv, inv_size, thickness, direction, x, y):
    """ViewportUV를 direction*InvSize*Thickness 만큼 밀어 CustomStencil을 샘플한다."""
    dir_const = expr(unreal.MaterialExpressionConstant2Vector, x, y,
                     r=direction[0], g=direction[1])
    inv_thick = mul(inv_size, "InvSize", thickness, "", x, y + 90)
    offset = mul(dir_const, "", inv_thick, "", x + 180, y)
    uv = add(viewport_uv, ["ViewportUV", ""], offset, "", x + 340, y)
    tex = scene_texture(unreal.SceneTextureId.PPI_CUSTOM_STENCIL, x + 500, y)
    connect(uv, "", tex, ["UVs", ""])
    return tex


def build_graph():
    scene = scene_texture(unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0, -1500, -200)
    center = scene_texture(unreal.SceneTextureId.PPI_CUSTOM_STENCIL, -1500, 0)
    screen = expr(unreal.MaterialExpressionScreenPosition, -1900, 300)
    thickness = scalar("OutlineThickness", 2.0, -1900, 500)
    stencil_target = scalar("StencilTarget", 20.0, -1900, 600)
    outline_color = expr(unreal.MaterialExpressionVectorParameter, -1900, 700,
                         parameter_name="OutlineColor", group="LS Outline",
                         default_value=unreal.LinearColor(0.1, 1.0, 0.4, 1.0))

    neighbors = []
    for i, direction in enumerate(NEIGHBOR_DIRS):
        tex = sample_stencil_at(screen, center, thickness, direction, -1200, 250 + i * 260)
        neighbors.append(tex)

    edge = expr(unreal.MaterialExpressionCustom, -300, 200,
                description="LS Outline Edge",
                code=EDGE_HLSL,
                output_type=unreal.CustomMaterialOutputType.CMOT_FLOAT1,
                inputs=[custom_input("Center"), custom_input("N0"),
                        custom_input("N1"), custom_input("N2"),
                        custom_input("N3"), custom_input("StencilTarget")])
    connect(center, "Color", edge, "Center")
    for i, tex in enumerate(neighbors):
        connect(tex, "Color", edge, f"N{i}")
    connect(stencil_target, "", edge, "StencilTarget")

    final = expr(unreal.MaterialExpressionLinearInterpolate, 100, 0)
    connect(scene, "Color", final, "A")
    connect(outline_color, "", final, "B")
    connect(edge, "", final, "Alpha")

    if not MEL.connect_material_property(final, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR):
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
    log("남은 수동 설정: PPV(Infinite Extent) > Post Process Materials 에 MI_PP_Outline 추가")


if __name__ == "__main__":
    main()
