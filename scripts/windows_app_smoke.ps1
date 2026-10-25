$ErrorActionPreference = 'Stop'
$token = [guid]::NewGuid().ToString('N')
$event = [System.Threading.EventWaitHandle]::new($false, [System.Threading.EventResetMode]::ManualReset, "Local\PrintQuoteUpdateHealth-$token")
$process = Start-Process -FilePath (Join-Path $PSScriptRoot '..\PrintQuoteCalculator.exe') -ArgumentList '--update-health', $token -PassThru
try {
    if (!$event.WaitOne(60000)) { throw 'Actual WebView2 calculator did not pass the visible-window/render/JS responsiveness health check.' }
    $process.Refresh()
    if ($process.HasExited -or $process.MainWindowHandle -eq 0) { throw 'The actual calculator has no visible native window.' }
    Write-Output 'PASS: actual WebView2 startup, visible native window, rendered calculator and live JS health response.'
} finally {
    if (!$process.HasExited) { $null = $process.CloseMainWindow(); if (!$process.WaitForExit(10000)) { Stop-Process -Id $process.Id -Force } }
    $event.Dispose()
}
