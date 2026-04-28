@echo off
echo Cleaning UE generated folders...

if exist "Intermediate" (
    rmdir /s /q "Intermediate"
    echo Deleted: Intermediate
)
if exist "Binaries" (
    rmdir /s /q "Binaries"
    echo Deleted: Binaries
)
if exist "DerivedDataCache" (
    rmdir /s /q "DerivedDataCache"
    echo Deleted: DerivedDataCache
)

echo Done. Restart Unreal Editor.
pause
