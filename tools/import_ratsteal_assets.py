"""
Unreal Editor Python script for RatSteal source asset import.

Run inside the Unreal Editor Python environment:
    py "C:/Users/user/Desktop/LostSignal/tools/import_ratsteal_assets.py"

This imports copied source assets under:
    Content/LostSignal/MiniGame/RatSteal/SourceAssets

into UE assets under:
    /Game/LostSignal/MiniGame/RatSteal/Imported
"""

from __future__ import annotations

from pathlib import Path
from dataclasses import dataclass

import unreal


SOURCE_ROOT = Path(unreal.SystemLibrary.get_project_directory()) / "Content" / "LostSignal" / "MiniGame" / "RatSteal" / "SourceAssets"
DEST_ROOT = "/Game/LostSignal/MiniGame/RatSteal/Imported"

# Set true when intentionally refreshing existing imported assets.
REIMPORT_EXISTING = False

IMPORT_EXTENSIONS = {
    ".png": "Textures",
    ".mp3": "Audio",
    ".ttf": "Fonts",
}


@dataclass(frozen=True)
class ImportCandidate:
    source_file: Path
    destination_path: str
    asset_name: str


def _normalize_asset_path(path: str) -> str:
    return path.replace("\\", "/").rstrip("/")


def _asset_path_for(candidate: ImportCandidate) -> str:
    return f"{candidate.destination_path}/{candidate.asset_name}"


def _destination_for(source_file: Path) -> str | None:
    extension = source_file.suffix.lower()
    category = IMPORT_EXTENSIONS.get(extension)
    if category is None:
        return None

    relative_parent = source_file.parent.relative_to(SOURCE_ROOT)
    if relative_parent == Path("."):
        return f"{DEST_ROOT}/{category}"

    relative_path = "/".join(relative_parent.parts)
    return _normalize_asset_path(f"{DEST_ROOT}/{category}/{relative_path}")


def _collect_candidates() -> list[ImportCandidate]:
    if not SOURCE_ROOT.exists():
        raise RuntimeError(f"RatSteal source asset root not found: {SOURCE_ROOT}")

    candidates: list[ImportCandidate] = []
    for source_file in sorted(SOURCE_ROOT.rglob("*")):
        if not source_file.is_file():
            continue

        destination_path = _destination_for(source_file)
        if destination_path is None:
            continue

        candidates.append(
            ImportCandidate(
                source_file=source_file,
                destination_path=destination_path,
                asset_name=source_file.stem,
            )
        )

    return candidates


def _make_import_task(candidate: ImportCandidate) -> unreal.AssetImportTask:
    task = unreal.AssetImportTask()
    task.filename = str(candidate.source_file)
    task.destination_path = candidate.destination_path
    task.destination_name = candidate.asset_name
    task.automated = True
    task.save = True
    task.replace_existing = REIMPORT_EXISTING
    task.replace_existing_settings = False
    return task


def import_ratsteal_assets() -> None:
    candidates = _collect_candidates()
    skipped: list[ImportCandidate] = []
    import_candidates: list[ImportCandidate] = []
    tasks: list[unreal.AssetImportTask] = []

    for candidate in candidates:
        asset_path = _asset_path_for(candidate)
        if not REIMPORT_EXISTING and unreal.EditorAssetLibrary.does_asset_exist(asset_path):
            skipped.append(candidate)
            continue

        import_candidates.append(candidate)
        tasks.append(_make_import_task(candidate))

    unreal.log(
        f"[RatStealAssetImport] Source={SOURCE_ROOT} candidates={len(candidates)} "
        f"tasks={len(tasks)} skipped={len(skipped)} reimport_existing={REIMPORT_EXISTING}"
    )

    if tasks:
        asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
        asset_tools.import_asset_tasks(tasks)

    succeeded = 0
    failed: list[ImportCandidate] = []
    for task, candidate in zip(tasks, import_candidates):
        asset_path = _asset_path_for(candidate)
        if task.imported_object_paths or unreal.EditorAssetLibrary.does_asset_exist(asset_path):
            succeeded += 1
        else:
            failed.append(candidate)

    for candidate in failed:
        unreal.log_error(f"[RatStealAssetImport] Failed: {candidate.source_file} -> {_asset_path_for(candidate)}")

    unreal.log(
        f"[RatStealAssetImport] Completed: imported={succeeded} skipped={len(skipped)} "
        f"failed={len(failed)} total={len(candidates)}"
    )


if __name__ == "__main__":
    import_ratsteal_assets()
