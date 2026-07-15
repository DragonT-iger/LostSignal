"""
BS_Unarmed 샘플 시퀀스의 싱크 마커 위치에 발소리 노티파이(LSAN_Footstep) 자동 삽입.

블렌드 스페이스의 모든 샘플 AnimSequence에서 싱크 마커(발 닿는 프레임)를 읽어,
같은 시간에 LSAN_Footstep 노티파이를 전용 트랙에 삽입한다. 사운드 에셋은 노티파이가
아니라 캐릭터(ALSCharacterBase::FootstepSound)가 소유하므로 여기서는 소켓 이름만 굽는다.
블렌드 스페이스의 Notify Trigger Mode가 기본값(Highest Weighted Animation)이면
가중치 1등 샘플의 노티파이만 발화하므로 중복 재생은 없다.

Run inside the Unreal Editor Cmd console:
    py "C:/Users/user/Desktop/LostSignal/tools/insert_footstep_notifies.py"

재실행: 이미 트랙이 있는 시퀀스는 건너뛴다. 다시 깔고 싶으면 FORCE_REBUILD = True.
"""

from __future__ import annotations

import unreal

BLEND_SPACE_PATH = "/Game/LostSignal/Characters/Test/BS_Unarmed"
TRACK_NAME = "LS_Footsteps"

# True면 기존 TRACK_NAME 트랙의 노티파이를 지우고 다시 삽입한다.
FORCE_REBUILD = True

# 시퀀스 이름에 이 키워드가 포함되면 bSpawnVFX=False로 굽는다(사운드만). 걷기는 먼지 이펙트가 과함.
VFX_EXCLUDE_KEYWORDS = ["walk"]

FOOTSTEP_CLASS_PATH = "/Script/LostSignal.LSAN_Footstep"


def _socket_for_marker(marker_name: str) -> str:
    """마커 이름에서 좌/우 발 소켓(본) 이름을 추론한다."""
    lowered = marker_name.lower()
    if lowered == "l" or "_l" in lowered or "left" in lowered:
        return "foot_l"
    if lowered == "r" or "_r" in lowered or "right" in lowered:
        return "foot_r"
    return ""


def _collect_sample_sequences(blend_space: unreal.BlendSpace) -> list[unreal.AnimSequence]:
    """샘플 목록에서 AnimSequence를 중복 없이 수집한다."""
    sequences: list[unreal.AnimSequence] = []
    seen: set[str] = set()
    for sample in blend_space.get_editor_property("sample_data"):
        animation = sample.get_editor_property("animation")
        if not animation:
            continue
        path = animation.get_path_name()
        if path in seen:
            continue
        seen.add(path)
        sequences.append(animation)
    return sequences


def _get_sync_markers(sequence: unreal.AnimSequence) -> list[tuple[str, float]]:
    """(마커 이름, 시간) 목록을 반환한다."""
    markers = unreal.AnimationLibrary.get_animation_sync_markers(sequence)
    result: list[tuple[str, float]] = []
    for marker in markers:
        name = str(marker.get_editor_property("marker_name"))
        time = float(marker.get_editor_property("time"))
        result.append((name, time))
    return result


def _prepare_track(sequence: unreal.AnimSequence) -> bool:
    """노티파이 트랙을 준비한다. False면 이미 처리된 시퀀스라 건너뛴다."""
    if unreal.AnimationLibrary.is_valid_anim_notify_track_name(sequence, TRACK_NAME):
        if not FORCE_REBUILD:
            return False
        unreal.AnimationLibrary.remove_animation_notify_events_by_track(sequence, TRACK_NAME)
        return True
    unreal.AnimationLibrary.add_animation_notify_track(
        sequence, TRACK_NAME, unreal.LinearColor(0.2, 0.8, 0.4, 1.0))
    return True


def _insert_footstep(sequence, time, socket_name, spawn_vfx, footstep_class):
    """한 발 접지 시점에 LSAN_Footstep 노티파이를 삽입한다."""
    notify = unreal.AnimationLibrary.add_animation_notify_event(
        sequence, TRACK_NAME, time, footstep_class)
    if not notify:
        return
    if socket_name:
        notify.set_editor_property("socket_name", socket_name)
    notify.set_editor_property("spawn_vfx", spawn_vfx)


def main() -> None:
    blend_space = unreal.EditorAssetLibrary.load_asset(BLEND_SPACE_PATH)
    if not blend_space:
        unreal.log_error(f"블렌드 스페이스를 찾을 수 없음: {BLEND_SPACE_PATH}")
        return

    footstep_class = unreal.load_class(None, FOOTSTEP_CLASS_PATH)
    if not footstep_class:
        unreal.log_error("노티파이 클래스 로드 실패 (LSAN_Footstep)")
        return

    inserted_total = 0
    for sequence in _collect_sample_sequences(blend_space):
        seq_name = sequence.get_name()
        markers = _get_sync_markers(sequence)
        if not markers:
            unreal.log(f"[건너뜀] {seq_name}: 싱크 마커 없음")
            continue
        if not _prepare_track(sequence):
            unreal.log(f"[건너뜀] {seq_name}: '{TRACK_NAME}' 트랙이 이미 있음 (재삽입은 FORCE_REBUILD=True)")
            continue

        spawn_vfx = not any(keyword in seq_name.lower() for keyword in VFX_EXCLUDE_KEYWORDS)
        for marker_name, time in markers:
            socket_name = _socket_for_marker(marker_name)
            if not socket_name:
                unreal.log_warning(f"{seq_name}: 마커 '{marker_name}' 좌/우 판별 실패 — 소켓 미지정으로 삽입")
            _insert_footstep(sequence, time, socket_name, spawn_vfx, footstep_class)
            inserted_total += 1
            unreal.log(f"  {seq_name} @ {time:.3f}s ({marker_name}) → LSAN_Footstep 삽입 (VFX {'on' if spawn_vfx else 'off'})")

        unreal.EditorAssetLibrary.save_loaded_asset(sequence)

    unreal.log(f"완료: 발 접지 {inserted_total}곳에 노티파이 삽입 (트랙 '{TRACK_NAME}')")


main()
