"""
크리스탈 광석 텍스처 임포트 + 머티리얼 인스턴스 구성 스크립트 (언리얼 에디터 Python 전용).

generate_crystal_textures.py 가 만든 PNG 3장을 임포트하고(sRGB/압축 설정 포함),
M_CrystalOre 를 부모로 한 MI_CrystalOre 를 만들어 텍스처 파라미터에 할당한다.

실행 순서:
  1. python tools/generate_crystal_textures.py        (에디터 밖)
  2. py tools/create_crystal_ore_material.py          (에디터 안, 최초 1회)
  3. py tools/import_crystal_textures.py              (에디터 안)
"""

from pathlib import Path

import unreal

MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
ASSET_TOOLS = unreal.AssetToolsHelpers.get_asset_tools()

PROJECT_DIR = Path(unreal.SystemLibrary.get_project_directory())
SRC_DIR = PROJECT_DIR / "Content/LostSignal/Textures/CrystalOre/SourceAssets"
TEX_DEST = "/Game/LostSignal/Textures/CrystalOre"
PARENT_MAT = "/Game/LostSignal/Materials/M_CrystalOre"
MI_PATH = "/Game/LostSignal/Materials/MI_CrystalOre"

# (파일명, 머티리얼 파라미터명, 압축 설정)
TEXTURES = [
    ("T_CrystalOre_Packed.png", "PackedNoise", unreal.TextureCompressionSettings.TC_MASKS),
    ("T_CrystalOre_Curvature.png", "CurvatureTexture", unreal.TextureCompressionSettings.TC_GRAYSCALE),
    ("T_CrystalOre_N.png", "NormalTexture", unreal.TextureCompressionSettings.TC_NORMALMAP),
]


def log(msg):
    unreal.log(f"[CrystalTexImport] {msg}")


def log_error(msg):
    unreal.log_error(f"[CrystalTexImport] {msg}")


def import_texture(file_name, compression):
    src = SRC_DIR / file_name
    if not src.exists():
        log_error(f"소스 PNG 없음: {src} — generate_crystal_textures.py 를 먼저 실행")
        return None
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(src))
    task.set_editor_property("destination_path", TEX_DEST)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", False)
    ASSET_TOOLS.import_asset_tasks([task])

    asset_path = f"{TEX_DEST}/{src.stem}"
    tex = unreal.load_asset(asset_path)
    if not tex:
        log_error(f"임포트 실패: {asset_path}")
        return None
    tex.set_editor_property("srgb", False)
    tex.set_editor_property("compression_settings", compression)
    if compression == unreal.TextureCompressionSettings.TC_NORMALMAP:
        tex.set_editor_property("flip_green_channel", False)  # DirectX 규약으로 생성됨
    EAL.save_asset(asset_path, only_if_is_dirty=False)
    log(f"임포트 완료: {asset_path}")
    return tex


def get_or_create_instance(parent):
    if EAL.does_asset_exist(MI_PATH):
        mi = unreal.load_asset(MI_PATH)
        log(f"기존 인스턴스 재사용: {MI_PATH}")
        return mi
    factory = unreal.MaterialInstanceConstantFactoryNew()
    factory.set_editor_property("initial_parent", parent)
    mi = ASSET_TOOLS.create_asset("MI_CrystalOre", "/Game/LostSignal/Materials",
                                  unreal.MaterialInstanceConstant, factory)
    log(f"인스턴스 생성: {MI_PATH}")
    return mi


def main():
    parent = unreal.load_asset(PARENT_MAT) if EAL.does_asset_exist(PARENT_MAT) else None
    if not parent:
        log_error(f"{PARENT_MAT} 없음 — create_crystal_ore_material.py 를 먼저 실행")
        return

    imported = {}
    for file_name, param_name, compression in TEXTURES:
        tex = import_texture(file_name, compression)
        if tex:
            imported[param_name] = tex

    if not imported:
        log_error("임포트된 텍스처가 없어 인스턴스 구성을 건너뜀")
        return

    mi = get_or_create_instance(parent)
    if not mi:
        log_error("머티리얼 인스턴스 생성 실패")
        return
    for param_name, tex in imported.items():
        if not MEL.set_material_instance_texture_parameter_value(mi, param_name, tex):
            log_error(f"텍스처 파라미터 할당 실패: {param_name}")
    MEL.update_material_instance(mi)
    EAL.save_asset(MI_PATH, only_if_is_dirty=False)
    log(f"완료: {MI_PATH} 에 텍스처 {len(imported)}장 할당됨")


if __name__ == "__main__":
    main()
