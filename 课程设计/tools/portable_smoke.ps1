$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$smoke = Join-Path $root 'build-portable-smoke'
$game = Join-Path $smoke 'game'
$empty = Join-Path $smoke 'empty-cwd'
New-Item -ItemType Directory -Force $game,$empty | Out-Null
Expand-Archive -LiteralPath (Join-Path $root 'dist/Gomoku-Windows-x64.zip') -DestinationPath $game -Force
$original = (Get-FileHash (Join-Path $root 'assets/fonts/Font.ttf')).Hash
$packaged = (Get-FileHash (Join-Path $game 'assets/fonts/Font.ttf')).Hash
if ($original -ne $packaged) { throw 'Packaged font differs from supplied original' }
$oldPath = $env:PATH
try {
    $env:PATH = "$env:SystemRoot\System32;$env:SystemRoot"
    for ($attempt = 1; $attempt -le 5; $attempt++) {
        $process = Start-Process -FilePath (Join-Path $game 'gobang.exe') -WorkingDirectory $empty -PassThru -RedirectStandardOutput (Join-Path $smoke "launch-$attempt.log") -RedirectStandardError (Join-Path $smoke "launch-$attempt.err")
        $null = $process.Handle
        try {
            Start-Sleep -Milliseconds 1200
            $process.Refresh()
            if ($process.HasExited) { throw "Portable game exited early: $($process.ExitCode)" }
            if ($process.MainWindowHandle -eq 0) { throw 'Portable game did not create a visible window' }
            Write-Output "Portable launch $attempt created a window with clean PATH and empty cwd"
            if (-not $process.CloseMainWindow()) { throw 'Cannot close owned smoke-test window' }
            if (-not $process.WaitForExit(5000)) { throw 'Owned game did not exit cleanly' }
            $process.WaitForExit()
            $process.Refresh()
            if ($process.ExitCode -ne 0) { throw "Portable game exit code: $($process.ExitCode)" }
        } finally {
            if (-not $process.HasExited) { $process.Kill(); $process.WaitForExit() }
            $process.Dispose()
        }
    }
    # The test binary is copied into the extracted test copy only, never into the release archive.
    Copy-Item (Join-Path $root 'build-verify/ui_tests.exe') $game -Force
    $tests = Start-Process -FilePath (Join-Path $game 'ui_tests.exe') -WorkingDirectory $empty -PassThru -RedirectStandardOutput (Join-Path $smoke 'ui-tests.log') -RedirectStandardError (Join-Path $smoke 'ui-tests.err')
    $null = $tests.Handle
    try {
        if (-not $tests.WaitForExit(30000)) { throw 'Portable UI test timed out' }
        $tests.WaitForExit()
        $tests.Refresh()
        if ($tests.ExitCode -ne 0) { throw "Portable UI tests failed: $($tests.ExitCode)" }
        Write-Output 'Portable font, AI threads, cancellation and UI regressions passed'
    } finally {
        if (-not $tests.HasExited) { $tests.Kill(); $tests.WaitForExit() }
        $tests.Dispose()
    }
} finally { $env:PATH = $oldPath }
