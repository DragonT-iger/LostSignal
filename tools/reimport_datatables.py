"""
Unreal Editor Python script for LostSignal DataTable CSV reimport.

Run inside the Unreal Editor Python environment:
    py "C:/Users/user/Desktop/LostSignal/tools/reimport_datatables.py"
"""

from __future__ import annotations

import os
from dataclasses import dataclass

import unreal


@dataclass(frozen=True)
class DataTableTarget:
    asset_name: str
    csv_name: str
    row_struct_path: str


CSV_DIR = os.path.join(unreal.SystemLibrary.get_project_directory(), "Content", "LostSignal", "Sandbox", "DT")
ASSET_DIR = "/Game/LostSignal/Data/DataTables"

TARGETS = [
    DataTableTarget("DT_Armor", "DT_Armor.csv", "/Script/LostSignal.LSArmorRow"),
    DataTableTarget("DT_CharacterStat", "DT_CharacterStat.csv", "/Script/LostSignal.LSCharacterStatRow"),
    DataTableTarget("DT_ChipRow", "DT_Chip.csv", "/Script/LostSignal.LSChipRow"),
    DataTableTarget("DT_ChipStat", "DT_ChipStat.csv", "/Script/LostSignal.LSChipStatRow"),
    DataTableTarget("DT_DropTable", "DT_DropTable.csv", "/Script/LostSignal.LSDropTableRow"),
    DataTableTarget("DT_GroupTable", "DT_GroupTable.csv", "/Script/LostSignal.LSGroupTableRow"),
    DataTableTarget("DT_Item", "DT_Item.csv", "/Script/LostSignal.LSItemRow"),
    DataTableTarget("DT_Protocol", "DT_Protocol.csv", "/Script/LostSignal.LSProtocolUnlockRow"),
    DataTableTarget("DT_RootingObject", "DT_RootingObject.csv", "/Script/LostSignal.LSRootingObjectRow"),
    DataTableTarget("DT_StoreStock", "DT_StoreStock.csv", "/Script/LostSignal.LSStoreStockRow"),
    DataTableTarget("DT_Weapon", "DT_Weapon.csv", "/Script/LostSignal.LSWeaponRow"),
]


def _load_row_struct(path: str) -> unreal.ScriptStruct:
    row_struct = unreal.load_object(None, path)
    if not row_struct:
        raise RuntimeError(f"RowStruct load failed: {path}")
    return row_struct


def _load_or_create_datatable(target: DataTableTarget, row_struct: unreal.ScriptStruct) -> unreal.DataTable:
    asset_path = f"{ASSET_DIR}/{target.asset_name}"
    datatable = unreal.EditorAssetLibrary.load_asset(asset_path)
    if datatable:
        return datatable

    factory = unreal.DataTableFactory()
    factory.struct = row_struct
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    created = asset_tools.create_asset(target.asset_name, ASSET_DIR, unreal.DataTable, factory)
    if not created:
        raise RuntimeError(f"DataTable create failed: {asset_path}")
    return created


def reimport_target(target: DataTableTarget) -> bool:
    csv_path = os.path.join(CSV_DIR, target.csv_name)
    if not os.path.exists(csv_path):
        unreal.log_error(f"[LSDataTableReimport] CSV not found: {csv_path}")
        return False

    row_struct = _load_row_struct(target.row_struct_path)
    datatable = _load_or_create_datatable(target, row_struct)
    # UE5.7 파이썬은 row_struct 속성 직접 대입을 지원하지 않는다. 신규 생성은 팩토리가 구조체를
    # 이미 지정했으므로, 기존 자산만 시도하고 실패(읽기 전용)하면 그대로 둔다.
    try:
        datatable.set_editor_property("row_struct", row_struct)
    except Exception:
        pass

    with open(csv_path, "r", encoding="utf-8-sig") as csv_file:
        csv_content = csv_file.read()

    problems = datatable.create_table_from_csv_string(csv_content)
    if problems:
        unreal.log_error(f"[LSDataTableReimport] {target.asset_name} import problems: {'; '.join(problems)}")
        return False

    datatable.mark_package_dirty()
    unreal.EditorAssetLibrary.save_loaded_asset(datatable)
    unreal.log(f"[LSDataTableReimport] Reimported {target.asset_name} from {csv_path}")
    return True


def main() -> None:
    success_count = 0
    for target in TARGETS:
        try:
            if reimport_target(target):
                success_count += 1
        except Exception as exc:
            unreal.log_error(f"[LSDataTableReimport] {target.asset_name} failed: {exc}")

    unreal.log(f"[LSDataTableReimport] Completed {success_count}/{len(TARGETS)} DataTables.")


if __name__ == "__main__":
    main()
