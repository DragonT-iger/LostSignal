"""기존 타이틀 버튼을 Sandbox 작업용으로 복제한다.

WidgetBlueprint의 WidgetTree 편집은 UE Python에서 보호되어 있으므로
RetainerBox 추가와 C++ 부모 변경은 위젯 디자이너에서 수행한다.
"""

import unreal


EAL = unreal.EditorAssetLibrary

SOURCE_PATH = "/Game/LostSignal/UI/Title/WBP_TitleButton"
PACKAGE_PATH = "/Game/LostSignal/Sandbox/YoonSeok/Code"
WIDGET_NAME = "WBP_TitleGlitchButton_yoonseok"
WIDGET_PATH = f"{PACKAGE_PATH}/{WIDGET_NAME}"


def log(message):
    unreal.log(f"[TitleGlitchButtonWidget] {message}")


def log_error(message):
    unreal.log_error(f"[TitleGlitchButtonWidget] {message}")


def main():
    if EAL.does_asset_exist(WIDGET_PATH):
        log_error(f"이미 존재하는 자산이라 덮어쓰지 않았습니다: {WIDGET_PATH}")
        return

    if not EAL.duplicate_asset(SOURCE_PATH, WIDGET_PATH):
        log_error(f"기존 버튼 복제 실패: {SOURCE_PATH}")
        return

    EAL.save_asset(WIDGET_PATH, only_if_is_dirty=False)
    log(f"복제 완료: {WIDGET_PATH}")
    log("위젯 디자이너에서 GlitchRetainer 추가 후 부모를 LSTitleGlitchButtonWidget으로 변경하세요.")


if __name__ == "__main__":
    main()
