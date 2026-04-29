@echo off
set RD=rmdir
echo Cleaning UE generated folders...

if exist "Intermediate" (
    %RD% /s /q "Intermediate"
    if errorlevel 1 (
        echo [ERROR] Failed to delete Intermediate - Close Unreal Editor and Visual Studio, then try again.
        pause
        exit /b 1
    )
    echo Deleted: Intermediate
)
if exist "Binaries" (
    %RD% /s /q "Binaries"
    if errorlevel 1 (
        echo [ERROR] Failed to delete Binaries - Close Unreal Editor and Visual Studio, then try again.
        pause
        exit /b 1
    )
    echo Deleted: Binaries
)
if exist "DerivedDataCache" (
    %RD% /s /q "DerivedDataCache"
    if errorlevel 1 (
        echo [ERROR] Failed to delete DerivedDataCache - Close Unreal Editor and Visual Studio, then try again.
        pause
        exit /b 1
    )
    echo Deleted: DerivedDataCache
)

echo Done. Restart Unreal Editor.
pause
