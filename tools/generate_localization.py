"""
로컬라이징 자동화 스크립트
엑셀 → ST CSV + .po 파일 자동 생성

사용법:
  python generate_localization.py

엑셀 형식 (Localization.xlsx, Sheet: CharacterName):
  Key          | Korean  | English
  char1_name   | 캐릭터1  | Character1
"""

import openpyxl
import os

# ── 경로 설정 ──────────────────────────────────────────────
BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

EXCEL_PATH   = os.path.join(BASE_DIR, "Content", "LostSignal", "Sandbox", "DT", "Localization.xlsx")
ST_CSV_PATH  = os.path.join(BASE_DIR, "Content", "LostSignal", "Sandbox", "DT", "ST_CharacterName.csv")
PO_PATH      = os.path.join(BASE_DIR, "Content", "Localization", "Game", "en", "Game.po")
ST_ASSET_PATH = "/Game/LostSignal/Data/StringTable/ST_CharacterName"

# ── 엑셀 읽기 ─────────────────────────────────────────────
def read_excel(path, sheet_name):
    wb = openpyxl.load_workbook(path)
    ws = wb[sheet_name]
    rows = list(ws.iter_rows(min_row=2, values_only=True))
    return [(row[0], row[1], row[2]) for row in rows if row[0]]

# ── ST CSV 생성 (한국어 소스) ──────────────────────────────
def write_st_csv(entries, path):
    with open(path, "w", encoding="utf-8-sig") as f:
        f.write("Key,SourceString\n")
        for key, korean, _ in entries:
            f.write(f"{key},{korean}\n")
    print(f"[ST CSV] {path}")

# ── .po 파일 생성 (영어 번역) ──────────────────────────────
def write_po(entries, path, st_asset_path):
    source_location = f"{st_asset_path}.{st_asset_path.split('/')[-1]}"

    with open(path, "r", encoding="utf-8") as f:
        content = f.read()

    # 기존 헤더 유지, msgctxt 블록만 교체
    header_end = content.find("msgctxt")
    header = content[:header_end] if header_end != -1 else content

    body = ""
    for key, korean, english in entries:
        body += f'#. Key:\t{key}\n'
        body += f'#. SourceLocation:\t{source_location}\n'
        body += f'#: {source_location}\n'
        body += f'msgctxt "{st_asset_path.split("/")[-1]},{key}"\n'
        body += f'msgid "{korean}"\n'
        body += f'msgstr "{english or ""}"\n\n'

    with open(path, "w", encoding="utf-8") as f:
        f.write(header + body)
    print(f"[.po]    {path}")

# ── 실행 ──────────────────────────────────────────────────
if __name__ == "__main__":
    entries = read_excel(EXCEL_PATH, "CharacterName")
    write_st_csv(entries, ST_CSV_PATH)
    write_po(entries, PO_PATH, ST_ASSET_PATH)
    print("\n완료! UE에서 ST Reimport → Import Text → Compile Text 하세요.")
