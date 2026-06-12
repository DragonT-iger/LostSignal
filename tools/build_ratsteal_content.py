"""
RatSteal content build script (run inside Unreal Editor Python).

Pipeline:
  1. Import source PNGs (delegates to import_ratsteal_assets.py)
  2. Create PaperSprites (sheet frames from aseprite JSON + single textures)
  3. Create PaperFlipbooks per frameTag
  4. Create Blueprints (player/farmer/crop/spawn manager/bush/indicator/game modes) and assign assets
  5. Create UMG widget blueprints (HUD/Result/Pause) with BindWidget-matching names
  6. Create maps MG_RatSteal / MG_RatSteal_Tutorial

Headless run:
  UnrealEditor-Cmd.exe <uproject> -ExecutePythonScript="<this file>"
"""

from __future__ import annotations

import json
import struct
import sys
from pathlib import Path

import unreal

TOOLS_DIR = Path(__file__).resolve().parent
PROJECT_DIR = Path(unreal.SystemLibrary.get_project_directory())
SOURCE_ROOT = PROJECT_DIR / "Content" / "LostSignal" / "MiniGame" / "RatSteal" / "SourceAssets"

ROOT = "/Game/LostSignal/MiniGame/RatSteal"
TEX_ROOT = f"{ROOT}/Imported/Textures"
SPR_ROOT = f"{ROOT}/Sprites"
FB_ROOT = f"{ROOT}/Flipbooks"
BP_ROOT = f"{ROOT}/Blueprints"
UI_ROOT = f"{ROOT}/UI"
MAP_ROOT = f"{ROOT}/Maps"

ASSET_TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
EAL = unreal.EditorAssetLibrary

FLIPBOOK_FPS = 20.0  # frame_run = round(duration_ms / 50)

CREATED = []
FAILED = []


def log(msg: str) -> None:
    unreal.log(f"[RatStealBuild] {msg}")


def log_error(msg: str) -> None:
    unreal.log_error(f"[RatStealBuild] {msg}")


def save(asset) -> None:
    path = asset.get_path_name().split(".")[0]
    EAL.save_asset(path, only_if_is_dirty=False)


def load_texture(rel: str) -> unreal.Texture2D | None:
    path = f"{TEX_ROOT}/{rel}"
    tex = unreal.load_asset(path)
    if not tex:
        log_error(f"texture not found: {path}")
    return tex


def load_sound(rel: str):
    path = f"{ROOT}/Imported/Audio/Sounds/{rel}"
    sound = unreal.load_asset(path)
    if not sound:
        log_error(f"sound not found: {path}")
    return sound


# ---------------------------------------------------------------- sprites

def png_dimensions(rel: str) -> tuple[int, int] | None:
    path = SOURCE_ROOT / f"{rel}.png"
    if not path.exists():
        log_error(f"source png not found: {path}")
        return None

    with open(path, "rb") as f:
        header = f.read(24)
    if len(header) < 24 or header[:8] != b"\x89PNG\r\n\x1a\n":
        log_error(f"invalid png header: {path}")
        return None

    width, height = struct.unpack(">II", header[16:24])
    return width, height


def create_sprite(name: str, texture: unreal.Texture2D,
                  uv=None, dim=None, folder: str = SPR_ROOT) -> unreal.PaperSprite | None:
    asset_path = f"{folder}/{name}"
    if EAL.does_asset_exist(asset_path):
        sprite = unreal.load_asset(asset_path)
    else:
        if not texture:
            return None

        factory = unreal.PaperSpriteFactory()
        sprite = ASSET_TOOLS.create_asset(name, folder, unreal.PaperSprite, factory)
        if not sprite:
            log_error(f"sprite create failed: {asset_path}")
            FAILED.append(asset_path)
            return None
        CREATED.append(asset_path)

    sprite.set_editor_property("source_texture", texture)
    if uv is not None:
        sprite.set_editor_property("source_uv", unreal.Vector2D(uv[0], uv[1]))
    if dim is not None:
        sprite.set_editor_property("source_dimension", unreal.Vector2D(dim[0], dim[1]))
    sprite.set_editor_property("pixels_per_unreal_unit", 1.0)
    save(sprite)
    return sprite


def load_aseprite_json(rel: str) -> dict:
    with open(SOURCE_ROOT / rel, "r", encoding="utf-8") as f:
        return json.load(f)


def build_sheet_flipbooks(json_rel: str, texture_rel: str, prefix: str) -> dict[str, unreal.PaperFlipbook]:
    """Create per-frame sprites + per-tag flipbooks from an aseprite sheet json."""
    data = load_aseprite_json(json_rel)
    texture = load_texture(texture_rel)
    if not texture:
        return {}

    frames = data["frames"]
    if isinstance(frames, dict):  # hash export fallback
        frames = list(frames.values())

    sprites = []
    durations = []
    for idx, fr in enumerate(frames):
        rect = fr["frame"]
        spr = create_sprite(
            f"SPR_{prefix}_{idx:02d}", texture,
            uv=(rect["x"], rect["y"]), dim=(rect["w"], rect["h"]),
            folder=f"{SPR_ROOT}/{prefix}")
        sprites.append(spr)
        durations.append(int(fr.get("duration", 100)))

    flipbooks = {}
    for tag in data["meta"].get("frameTags", []):
        name = f"FB_{prefix}_{tag['name']}"
        fb = create_flipbook(
            name,
            [(sprites[i], durations[i]) for i in range(int(tag["from"]), int(tag["to"]) + 1)])
        if fb:
            flipbooks[tag["name"]] = fb
    return flipbooks


def create_flipbook(name: str, keyframes) -> unreal.PaperFlipbook | None:
    asset_path = f"{FB_ROOT}/{name}"
    if EAL.does_asset_exist(asset_path):
        return unreal.load_asset(asset_path)

    factory = unreal.PaperFlipbookFactory()
    fb = ASSET_TOOLS.create_asset(name, FB_ROOT, unreal.PaperFlipbook, factory)
    if not fb:
        log_error(f"flipbook create failed: {asset_path}")
        FAILED.append(asset_path)
        return None

    kfs = []
    for sprite, duration_ms in keyframes:
        if not sprite:
            continue
        kf = unreal.PaperFlipbookKeyFrame()
        kf.set_editor_property("sprite", sprite)
        kf.set_editor_property("frame_run", max(1, round(duration_ms / (1000.0 / FLIPBOOK_FPS))))
        kfs.append(kf)

    fb.set_editor_property("frames_per_second", FLIPBOOK_FPS)
    fb.set_editor_property("key_frames", kfs)
    save(fb)
    CREATED.append(asset_path)
    return fb


# ---------------------------------------------------------------- blueprints

def create_blueprint(name: str, parent_class) -> tuple[unreal.Blueprint | None, object | None]:
    asset_path = f"{BP_ROOT}/{name}"
    if EAL.does_asset_exist(asset_path):
        bp = unreal.load_asset(asset_path)
    else:
        factory = unreal.BlueprintFactory()
        factory.set_editor_property("parent_class", parent_class)
        bp = ASSET_TOOLS.create_asset(name, BP_ROOT, None, factory)
        if bp:
            CREATED.append(asset_path)
    if not bp:
        log_error(f"blueprint create failed: {asset_path}")
        FAILED.append(asset_path)
        return None, None

    gen_class = unreal.load_object(None, f"{asset_path}.{name}_C")
    return bp, gen_class


def set_cdo(gen_class, props: dict, component_props: dict | None = None) -> None:
    cdo = unreal.get_default_object(gen_class)
    for key, value in props.items():
        try:
            cdo.set_editor_property(key, value)
        except Exception as exc:  # noqa: BLE001 - log and continue per property
            log_error(f"set_cdo {gen_class.get_name()}.{key} failed: {exc}")
    for comp_name, comp_values in (component_props or {}).items():
        try:
            comp = cdo.get_editor_property(comp_name)
            for key, value in comp_values.items():
                comp.set_editor_property(key, value)
        except Exception as exc:  # noqa: BLE001
            log_error(f"set_cdo {gen_class.get_name()}.{comp_name} failed: {exc}")


# ---------------------------------------------------------------- widgets

def create_widget_bp(name: str, parent_class):
    asset_path = f"{UI_ROOT}/{name}"
    if EAL.does_asset_exist(asset_path):
        return unreal.load_asset(asset_path)

    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property("parent_class", parent_class)
    wbp = ASSET_TOOLS.create_asset(name, UI_ROOT, None, factory)
    if wbp:
        CREATED.append(asset_path)
    return wbp


def widget_tree_of(wbp):
    return wbp.get_editor_property("widget_tree")


def make_canvas_root(wbp):
    tree = widget_tree_of(wbp)
    root = unreal.new_object(unreal.CanvasPanel, outer=tree, name="RootCanvas")
    tree.set_editor_property("root_widget", root)
    return tree, root


def add_text(tree, root, name: str, x: float, y: float, size: int, text: str = ""):
    block = unreal.new_object(unreal.TextBlock, outer=tree, name=name)
    block.set_editor_property("text", unreal.Text(text))
    font = block.get_editor_property("font")
    font.set_editor_property("size", size)
    block.set_editor_property("font", font)
    slot = root.add_child_to_canvas(block)
    slot.set_position(unreal.Vector2D(x, y))
    slot.set_auto_size(True)
    return block


def add_image(tree, root, name: str, x: float, y: float, w: float, h: float, texture=None):
    img = unreal.new_object(unreal.Image, outer=tree, name=name)
    if texture:
        brush = unreal.SlateBrush()
        brush.set_editor_property("resource_object", texture)
        brush.set_editor_property("image_size", unreal.DeprecateSlateVector2D(w, h))
        img.set_editor_property("brush", brush)
    slot = root.add_child_to_canvas(img)
    slot.set_position(unreal.Vector2D(x, y))
    slot.set_size(unreal.Vector2D(w, h))
    return img


def add_progress_bar(tree, root, name: str, x: float, y: float, w: float, h: float):
    bar = unreal.new_object(unreal.ProgressBar, outer=tree, name=name)
    bar.set_editor_property("percent", 1.0)
    slot = root.add_child_to_canvas(bar)
    slot.set_position(unreal.Vector2D(x, y))
    slot.set_size(unreal.Vector2D(w, h))
    return bar


def build_widgets() -> dict[str, object]:
    """Returns widget generated classes by key. Failures are logged and skipped."""
    # UI는 현재 C++ 폴백 레이아웃을 사용한다. WBP는 아트 패스에서 별도 제작/할당.
    return {
        "hud": unreal.LSRatHUDWidget,
        "result": unreal.LSRatResultWidget,
        "pause": unreal.LSRatPauseWidget,
    }


# ---------------------------------------------------------------- maps

def get_subsystems():
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    return les, eas, ues


def spawn_bp(eas, bp_asset, x: float, z: float, y: float = 0.0):
    return eas.spawn_actor_from_object(bp_asset, unreal.Vector(x, y, z))


def spawn_cls(eas, cls, x: float, z: float, y: float = 0.0):
    return eas.spawn_actor_from_class(cls, unreal.Vector(x, y, z))


def find_actor_by_label(eas, label: str):
    for actor in eas.get_all_level_actors():
        if actor.get_actor_label() == label:
            return actor
    return None


def place_actor(actor, x: float, z: float, y: float = 0.0):
    actor.set_actor_location(unreal.Vector(x, y, z), False, False)
    return actor


def ensure_bp_actor(eas, bp_asset, label: str, x: float, z: float, y: float = 0.0):
    actor = find_actor_by_label(eas, label)
    if not actor:
        actor = spawn_bp(eas, bp_asset, x, z, y)
        actor.set_actor_label(label)
    return place_actor(actor, x, z, y)


def replace_bp_actor(eas, bp_asset, label: str, x: float, z: float, y: float = 0.0):
    actor = find_actor_by_label(eas, label)
    if actor:
        eas.destroy_actor(actor)
    actor = spawn_bp(eas, bp_asset, x, z, y)
    actor.set_actor_label(label)
    return actor


def ensure_cls_actor(eas, cls, label: str, x: float, z: float, y: float = 0.0):
    actor = find_actor_by_label(eas, label)
    if not actor:
        actor = spawn_cls(eas, cls, x, z, y)
        actor.set_actor_label(label)
    return place_actor(actor, x, z, y)


def set_world_game_mode(ues, gm_class) -> None:
    world = ues.get_editor_world()
    settings = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.WorldSettings)
    if settings:
        settings[0].set_editor_property("default_game_mode", gm_class)
    else:
        log_error("WorldSettings not found — game mode override skipped")


def build_map(level_path: str, gm_class, bps: dict, sprites: dict, tutorial: bool) -> None:
    les, eas, ues = get_subsystems()

    if EAL.does_asset_exist(level_path):
        if not les.load_level(level_path):
            log_error(f"load_level failed: {level_path}")
            FAILED.append(level_path)
            return
        log(f"map exists, updating: {level_path}")
    elif not les.new_level(level_path):
        log_error(f"new_level failed: {level_path}")
        FAILED.append(level_path)
        return

    set_world_game_mode(ues, gm_class)

    # 배경 (원작 order -200000)
    bg_key = "background_tutorial" if tutorial else "background"
    if sprites.get(bg_key):
        bg = ensure_cls_actor(eas, unreal.PaperSpriteActor, "Background", 0.0, 0.0, y=500.0)
        comp = bg.get_editor_property("render_component")
        comp.set_editor_property("source_sprite", sprites[bg_key])
        comp.set_editor_property("translucency_sort_priority", -200000)

    replace_bp_actor(eas, bps["spawn_manager"], "RatSpawnManager", 0.0, 0.0)

    ensure_cls_actor(eas, unreal.PlayerStart, "PlayerStart", 0.0, 0.0)

    if tutorial:
        # 축소판: 제출존 1, 부쉬 1, 농부 1(외곽) — 32_Tutorial
        ensure_cls_actor(eas, unreal.LSRatSubmissionArea, "SubmissionArea_R", 3580.0, 0.0)
        ensure_bp_actor(eas, bps["bush"], "Bush_1", -100.0, 0.0)
        replace_bp_actor(eas, bps["farmer"], "Farmer_1", 2400.0, 1500.0)
    else:
        # 원작 MainScene: 제출존 x=±3580, 부쉬 (-100,0) + 동선용 추가
        ensure_cls_actor(eas, unreal.LSRatSubmissionArea, "SubmissionArea_R", 3580.0, 0.0)
        ensure_cls_actor(eas, unreal.LSRatSubmissionArea, "SubmissionArea_L", -3580.0, 0.0)
        for idx, (bx, bz) in enumerate([(-100.0, 0.0), (-1600.0, 900.0), (1600.0, -900.0),
                                        (-2600.0, -1600.0), (2600.0, 1600.0)]):
            ensure_bp_actor(eas, bps["bush"], f"Bush_{idx + 1}", bx, bz)
        for idx, (fx, fz) in enumerate([(1500.0, 700.0), (-1500.0, -700.0),
                                        (1500.0, -700.0), (-1500.0, 700.0)]):
            replace_bp_actor(eas, bps["farmer"], f"Farmer_{idx + 1}", fx, fz)

    les.save_current_level()
    CREATED.append(level_path)
    log(f"map created: {level_path}")


# ---------------------------------------------------------------- main

def main() -> None:
    # 1) 원본 PNG 임포트
    sys.path.insert(0, str(TOOLS_DIR))
    import import_ratsteal_assets  # noqa: PLC0415

    import_ratsteal_assets.import_ratsteal_assets()

    # 2) 시트 → 스프라이트 + 플립북
    mole_fbs = build_sheet_flipbooks("Player/mole_final.json", "Player/mole_final", "Mole")
    farmer_fbs = build_sheet_flipbooks("Farmer/farmer_final.json", "Farmer/farmer_final", "Farmer")
    sparkle_fbs = build_sheet_flipbooks("Crops/crop_sparkle/crop_sparkle.json", "Crops/crop_sparkle/crop_sparkle", "Sparkle")

    # 3) 단일 스프라이트 (작물 단계 Born=plant 공용 / 부쉬 / 지시자 / 배경)
    singles = {
        "plant": "Crops/plant",
        "eggplantS": "Crops/eggplantS", "eggplantM": "Crops/eggplantM", "eggplantL": "Crops/eggplantL",
        "potatoS": "Crops/potatoS", "potatoM": "Crops/potatoM", "potatoL": "Crops/potatoL",
        "pumpkinS": "Crops/pumpkinS", "pumpkinM": "Crops/pumpkinM", "pumpkinL": "Crops/pumpkinL",
        "eggplant_item": "Crops/eggplant_item",
        "potato_item": "Crops/potato_item",
        "pumpkin_item": "Crops/pumpkin_item",
        "bush": "Ground/bush_1",
        "redCircle": "Farmer/redCircle",
        "background": "Ground/background_test_1",
        "background_tutorial": "Ground/TutorialBackground",
    }
    sprites = {
        key: create_sprite(f"SPR_{key}", load_texture(rel), dim=png_dimensions(rel))
        for key, rel in singles.items()
    }

    # 4) 블루프린트 + 에셋 할당
    bps = {}

    bp, indicator_class = create_blueprint("BP_RatAttackIndicator", unreal.LSRatAttackIndicator)
    set_cdo(indicator_class, {"indicator_sprite": sprites["redCircle"]})
    save(bp)
    bps["indicator"] = bp

    bp, player_class = create_blueprint("BP_RatPlayer", unreal.LSRatPlayer)
    set_cdo(player_class, {
        "idle_flipbook": mole_fbs.get("idle"),
        "walk_flipbook": mole_fbs.get("walk"),
        "steal_flipbook": mole_fbs.get("steal"),
        "throw_sprites": {
            unreal.LSRatCropType.EGGPLANT: sprites["eggplant_item"],
            unreal.LSRatCropType.POTATO: sprites["potato_item"],
            unreal.LSRatCropType.PUMPKIN: sprites["pumpkin_item"],
        },
        "steal_sound": load_sound("SFX/1"),
        "throw_sound": load_sound("SFX/2"),
        "hit_sound": load_sound("SFX/3"),
        "submit_sound": load_sound("UI/1"),
    }, component_props={
        "camera": {"ortho_width": 950.0},
    })
    save(bp)
    bps["player"] = bp

    bp, farmer_class = create_blueprint("BP_RatFarmer", unreal.LSRatFarmer)
    set_cdo(farmer_class, {
        "idle_flipbook": farmer_fbs.get("idle"),
        "angry_idle_flipbook": farmer_fbs.get("angryidle"),
        "walk_flipbook": farmer_fbs.get("walk"),
        "angry_walk_flipbook": farmer_fbs.get("angrywalk"),
        "attack_flipbook": farmer_fbs.get("attack"),
        "indicator_class": indicator_class,
        "attack_sound": load_sound("SFX/4"),
    })
    save(bp)
    bps["farmer"] = bp

    bp, crop_class = create_blueprint("BP_RatCrop", unreal.LSRatCrop)
    set_cdo(crop_class, {}, component_props={
        "sparkle_effect": {"source_flipbook": sparkle_fbs.get("sparkle")},
    })
    save(bp)
    bps["crop"] = bp

    bp, _bush_class = create_blueprint("BP_RatBush", unreal.LSRatBush)
    set_cdo(_bush_class, {}, component_props={
        "sprite": {"source_sprite": sprites["bush"]},
    })
    save(bp)
    bps["bush"] = bp

    def visual_set(stage_keys):
        vs = unreal.LSRatCropVisualSet()
        vs.set_editor_property("stage_sprites", [sprites[k] for k in stage_keys])
        return vs

    bp, sm_class = create_blueprint("BP_RatSpawnManager", unreal.LSRatSpawnManager)
    set_cdo(sm_class, {
        "crop_class": crop_class,
        "crop_visuals": {
            unreal.LSRatCropType.EGGPLANT: visual_set(["plant", "eggplantS", "eggplantM", "eggplantL"]),
            unreal.LSRatCropType.POTATO: visual_set(["plant", "potatoS", "potatoM", "potatoL"]),
            unreal.LSRatCropType.PUMPKIN: visual_set(["plant", "pumpkinS", "pumpkinM", "pumpkinL"]),
        },
    })
    save(bp)
    bps["spawn_manager"] = bp

    # 5) 위젯
    widget_classes = build_widgets()

    # 6) 게임모드 BP (위젯/폰 클래스 연결)
    gm_props = {
        "default_pawn_class": player_class,
        "hud_widget_class": widget_classes.get("hud"),
        "result_widget_class": widget_classes.get("result"),
        "pause_widget_class": widget_classes.get("pause"),
    }
    gm_props = {k: v for k, v in gm_props.items() if v}

    bp, gm_class = create_blueprint("BP_RatGameMode", unreal.LSRatGameMode)
    set_cdo(gm_class, gm_props)
    save(bp)

    bp, tut_gm_class = create_blueprint("BP_RatTutorialGameMode", unreal.LSRatTutorialGameMode)
    set_cdo(tut_gm_class, gm_props)
    save(bp)

    # 7) 맵
    build_map(f"{MAP_ROOT}/MG_RatSteal", gm_class, bps, sprites, tutorial=False)
    build_map(f"{MAP_ROOT}/MG_RatSteal_Tutorial", tut_gm_class, bps, sprites, tutorial=True)

    log(f"done. created={len(CREATED)} failed={len(FAILED)}")
    for path in FAILED:
        log_error(f"FAILED: {path}")


if __name__ == "__main__":
    main()
