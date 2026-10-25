param([Parameter(Mandatory=$true)][string]$ExpectedVersion)
$ErrorActionPreference = 'Stop'
if ($env:GITHUB_ACTIONS -ne 'true') { throw 'Run this qualification only in an ephemeral GitHub Windows runner.' }
Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
Add-Type -AssemblyName System.Windows.Forms
Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class PQCQualificationWindow {
 [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
}
'@
$repo = 'https://github.com/TechSavage1989/Print-Quote-Calculator'
$oldHash = 'df817ea7db6466c0f80a52760b242008a1277d55f5db7fcc4f7f50d64e104a37'
$root = Join-Path $env:RUNNER_TEMP ('pqc-upgrade-' + [guid]::NewGuid().ToString('N'))
$appFolder = Join-Path $root 'app'
$null = New-Item $appFolder -ItemType Directory -Force
$exe = Join-Path $appFolder 'PrintQuoteCalculator.exe'
$updateRoot = Join-Path $env:LOCALAPPDATA 'PrintQuoteCalculatorWebView2\Updates'
$old = $null; $new = $null

function RootElement($process) {
    $process.Refresh()
    if ($process.HasExited -or $process.MainWindowHandle -eq 0) { return $null }
    return [System.Windows.Automation.AutomationElement]::FromHandle($process.MainWindowHandle)
}
function FindControl($rootElement, [string]$type, [string]$pattern) {
    if (!$rootElement) { return $null }
    $controlType = [System.Windows.Automation.ControlType]::$type
    $condition = New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::ControlTypeProperty, $controlType)
    $elements = $rootElement.FindAll([System.Windows.Automation.TreeScope]::Descendants, $condition)
    foreach ($element in $elements) {
        try { if ($element.Current.Name -match $pattern -and !$element.Current.IsOffscreen) { return $element } } catch {}
    }
    return $null
}
function WaitControl($process, [string]$type, [string]$pattern, [int]$seconds=60) {
    $until = [DateTime]::UtcNow.AddSeconds($seconds)
    while ([DateTime]::UtcNow -lt $until) {
        try { $element = FindControl (RootElement $process) $type $pattern; if ($element) { return $element } } catch {}
        Start-Sleep -Milliseconds 500
    }
    throw "Control was not available: $type / $pattern"
}
function InvokeControl($element) {
    $invoke = $element.GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern)
    $invoke.Invoke()
}
function ReadValue($element) {
    return $element.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern).Current.Value
}
try {
    # Fresh runner, no user profile is deleted or rewritten. The actual old app creates its own settings.
    $manifest = Invoke-RestMethod "$repo/releases/latest/download/update.json"
    if ($manifest.version -ne $ExpectedVersion -or $manifest.app_id -ne 'PrintQuoteCalculator.WebView2') { throw 'Published feed version or identity mismatch.' }
    if ($manifest.url -ne "$repo/releases/download/v$ExpectedVersion/PrintQuoteCalculator.exe") { throw 'Untrusted release asset URL.' }
    Invoke-WebRequest "$repo/releases/download/v1.6.0/PrintQuoteCalculator.exe" -OutFile $exe -UseBasicParsing
    if ((Get-FileHash $exe -Algorithm SHA256).Hash.ToLowerInvariant() -ne $oldHash) { throw 'Previous released EXE digest mismatch.' }
    $old = Start-Process $exe -PassThru
    $project = WaitControl $old 'Edit' '^Project name'
    $null = [PQCQualificationWindow]::SetForegroundWindow($old.MainWindowHandle)
    $project.SetFocus()
    [System.Windows.Forms.SendKeys]::SendWait('^a')
    [System.Windows.Forms.SendKeys]::SendWait('Updater test ' + $ExpectedVersion)
    Start-Sleep -Milliseconds 500
    $beforeName = ReadValue $project
    if ($beforeName -ne ('Updater test ' + $ExpectedVersion)) { throw 'Could not enter a real project name in the old app.' }
    $theme = WaitControl $old 'Button' '(Switch to (dark|light) mode|^(Dark|Light) mode$)'
    InvokeControl $theme
    Start-Sleep -Milliseconds 500
    $beforeTheme = (WaitControl $old 'Button' '(Switch to (dark|light) mode|^(Dark|Light) mode$)').Current.Name
    $beforeBounds = (RootElement $old).Current.BoundingRectangle
    # This is the released 1.6.0 app's own Download action, using its own WinHTTP and validation code.
    InvokeControl (WaitControl $old 'Button' '^Download$' 90)
    $install = WaitControl $old 'Button' '^Install and restart$' 120
    InvokeControl $install
    if (!$old.WaitForExit(45000)) { throw 'The previous app did not exit after explicit installation.' }
    $until = [DateTime]::UtcNow.AddSeconds(75)
    while ([DateTime]::UtcNow -lt $until) {
        $new = Get-Process -Name PrintQuoteCalculator -ErrorAction SilentlyContinue | Where-Object { $_.Path -eq $exe -and $_.Id -ne $old.Id } | Select-Object -First 1
        if ($new) {
            try { if (FindControl (RootElement $new) 'Edit' '^Project name') { break } } catch {}
        }
        Start-Sleep -Milliseconds 500
    }
    if (!$new -or !(RootElement $new)) { throw 'The updated app did not show a new native window.' }
    $new.Refresh()
    if ($new.MainWindowTitle -notmatch [regex]::Escape($ExpectedVersion)) { throw 'Updated native title has the wrong version.' }
    if ((Get-FileHash $exe -Algorithm SHA256).Hash.ToLowerInvariant() -ne $manifest.sha256) { throw 'Installed EXE does not match the published digest.' }
    if ([Diagnostics.FileVersionInfo]::GetVersionInfo($exe).FileVersion -ne $ExpectedVersion) { throw 'Installed PE has the wrong file version.' }
    if ((ReadValue (WaitControl $new 'Edit' '^Project name')) -ne $beforeName) { throw 'Project input was not preserved.' }
    if ((WaitControl $new 'Button' '(Switch to (dark|light) mode|^(Dark|Light) mode$)').Current.Name -ne $beforeTheme) { throw 'Theme was not preserved.' }
    $bounds = (RootElement $new).Current.BoundingRectangle
    if ([Math]::Abs($bounds.X-$beforeBounds.X) -gt 8 -or [Math]::Abs($bounds.Y-$beforeBounds.Y) -gt 8 -or [Math]::Abs($bounds.Width-$beforeBounds.Width) -gt 8 -or [Math]::Abs($bounds.Height-$beforeBounds.Height) -gt 8) { throw 'Native window placement changed unexpectedly.' }
    InvokeControl (WaitControl $new 'Button' '^Settings$')
    $until = [DateTime]::UtcNow.AddSeconds(20)
    $about = $false
    while ([DateTime]::UtcNow -lt $until) {
        $r = RootElement $new
        $condition = New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::NameProperty, 'About Print Quote Calculator')
        if ($r.FindFirst([System.Windows.Automation.TreeScope]::Descendants,$condition)) { $about=$true; break }
        Start-Sleep -Milliseconds 500
    }
    if (!$about) { throw 'The new About section was not visible after upgrading.' }
    $backup = Join-Path $updateRoot 'rollback\previous.exe'
    if (!(Test-Path $backup) -or (Get-FileHash $backup -Algorithm SHA256).Hash.ToLowerInvariant() -ne $oldHash) { throw 'The one rollback backup does not match the previous release.' }
    $until = [DateTime]::UtcNow.AddSeconds(30)
    do {
        $transactions = @(Get-ChildItem $updateRoot -Directory | Where-Object { $_.Name -match '^[a-f0-9]{32}$' })
        if ($transactions.Count -eq 0) { break }
        Start-Sleep -Milliseconds 500
    } while ([DateTime]::UtcNow -lt $until)
    if ($transactions.Count -ne 0) { throw 'Update helper transaction was not cleaned after the healthy restart.' }
    if (@(Get-ChildItem $appFolder -File).Count -ne 1) { throw 'The portable app folder contains updater leftovers.' }
    Write-Output "PASS: actual published 1.6.0 -> $ExpectedVersion UI Download, digest/PE validation, explicit Install and restart, visible responsive new About UI, project/theme/window preservation, stable launch path, one rollback backup and helper cleanup."
} finally {
    foreach ($process in @($new,$old)) {
        if ($process) { try { $process.Refresh(); if (!$process.HasExited) { $null=$process.CloseMainWindow(); if (!$process.WaitForExit(10000)) { Stop-Process -Id $process.Id -Force } } } catch {} }
    }
}
