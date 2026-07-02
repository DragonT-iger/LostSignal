$ErrorActionPreference = 'Stop'

# 에디터가 실행 중이면 종료 시 UE가 EditorKeyBindings.ini를 덮어써서 적용이 사라진다.
$runningEditors = Get-Process -Name 'UnrealEditor', 'UnrealEditor-Win64-DebugGame' -ErrorAction SilentlyContinue
if ($runningEditors) {
    Write-Host '[ABORT] 언리얼 에디터가 실행 중입니다.' -ForegroundColor Yellow
    Write-Host '        모든 언리얼 에디터 창을 닫은 뒤 다시 실행하세요.'
    exit 2
}

$targetJson = '{"BindingContext":"PlayWorld","CommandName":"StopPlaySession","ChordIndex":0,"Control":false,"Alt":false,"Shift":true,"Command":false,"Key":"Escape"}'
# UE ini parser (ParseLineExtended) treats { } " \ / | as special characters.
# The value must be escaped exactly like UE itself saves it (see FRemoteConfig::ReplaceIniCharWithSpecialChar),
# otherwise the chord line is mangled at parse time and silently ignored.
$targetValue = $targetJson.Replace('{', '~OpenBracket~').Replace('}', '~CloseBracket~').Replace('"', '~Quote~').Replace('\', '~Backslash~').Replace('/', '~Forwardslash~').Replace('|', '~Bar~')
$targetLine = 'UserDefinedChords=' + $targetValue

$candidateFiles = New-Object System.Collections.Generic.List[string]
foreach ($root in @($env:LOCALAPPDATA, $env:APPDATA)) {
    if (-not $root) {
        continue
    }

    foreach ($dirName in @('UnrealEngine', 'Unreal Engine')) {
        $dir = Join-Path $root $dirName
        if (Test-Path $dir) {
            Get-ChildItem -Path $dir -Recurse -Filter 'EditorKeyBindings.ini' -ErrorAction SilentlyContinue |
                ForEach-Object { $candidateFiles.Add($_.FullName) }
        }
    }
}

$defaultFile = Join-Path $env:LOCALAPPDATA 'UnrealEngine\5.7\Saved\Config\WindowsEditor\EditorKeyBindings.ini'
$candidateFiles.Add($defaultFile)
$files = $candidateFiles | Where-Object { $_ -eq $defaultFile -or $_ -match '\\5\.7\\' } | Select-Object -Unique

$stamp = Get-Date -Format 'yyyyMMdd_HHmmss'

function Set-StopPieChord {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    $dir = [System.IO.Path]::GetDirectoryName($Path)
    New-Item -ItemType Directory -Force -Path $dir | Out-Null

    if (Test-Path $Path) {
        Copy-Item -LiteralPath $Path -Destination ($Path + '.bak_' + $stamp) -Force
        $lines = [System.Collections.Generic.List[string]](Get-Content -LiteralPath $Path -Encoding UTF8)
    } else {
        $lines = [System.Collections.Generic.List[string]]::new()
    }

    $sectionIndex = -1
    for ($i = 0; $i -lt $lines.Count; $i++) {
        if ($lines[$i].Trim() -eq '[UserDefinedChords]') {
            $sectionIndex = $i
            break
        }
    }

    if ($sectionIndex -lt 0) {
        if ($lines.Count -eq 0) {
            # UE writes this header on its own saved config files; match it so the file is combined the same way.
            $lines.Add(';METADATA=(Diff=true, UseCommands=true)')
        } elseif ($lines[$lines.Count - 1].Trim() -ne '') {
            $lines.Add('')
        }

        $lines.Add('[UserDefinedChords]')
        $lines.Add($targetLine)
    } else {
        $nextSection = $lines.Count
        for ($i = $sectionIndex + 1; $i -lt $lines.Count; $i++) {
            if ($lines[$i].TrimStart().StartsWith('[')) {
                $nextSection = $i
                break
            }
        }

        for ($i = $nextSection - 1; $i -gt $sectionIndex; $i--) {
            if ($lines[$i] -match 'StopPlaySession' -and $lines[$i] -match 'PlayWorld') {
                $lines.RemoveAt($i)
            }
        }

        $lines.Insert($sectionIndex + 1, $targetLine)
    }

    [System.IO.File]::WriteAllLines($Path, [string[]]$lines, [System.Text.UTF8Encoding]::new($false))
    Write-Host ('[APPLY] ' + $Path)
    Write-Host ('        백업: ' + $Path + '.bak_' + $stamp)
}

foreach ($file in $files) {
    Set-StopPieChord $file
}

Write-Host ''
Write-Host '[DONE] PIE 정지 키가 ESC 에서 Shift+ESC 로 영구 변경되었습니다.' -ForegroundColor Green
Write-Host '       이 설정은 UE 5.7 에디터 전체(모든 프로젝트)에 적용됩니다.'
Write-Host '       되돌리기: 에디터 개인설정 > 키보드 단축키 > "Stop Play Session" 을 ESC 로 재설정'
