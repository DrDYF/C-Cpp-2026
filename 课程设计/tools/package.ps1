param([string]$CompilerRoot = 'D:/mingw64')
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$buildDir = Join-Path $projectRoot 'build-release'
$packageDir = Join-Path $projectRoot 'dist/Gomoku'
$cmakeExe = Join-Path $CompilerRoot 'bin/cmake.exe'
& $cmakeExe -S $projectRoot -B $buildDir -G 'MinGW Makefiles' -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF "-DCMAKE_C_COMPILER=$CompilerRoot/bin/gcc.exe" "-DCMAKE_CXX_COMPILER=$CompilerRoot/bin/g++.exe" "-DCMAKE_MAKE_PROGRAM=$CompilerRoot/bin/mingw32-make.exe"
if ($LASTEXITCODE -ne 0) { throw 'Release configuration failed' }
& $cmakeExe --build $buildDir --parallel 8
if ($LASTEXITCODE -ne 0) { throw 'Release build failed' }
New-Item -ItemType Directory -Force $packageDir | Out-Null
Copy-Item (Join-Path $buildDir 'gobang.exe') $packageDir -Force
Copy-Item (Join-Path $buildDir 'assets') $packageDir -Recurse -Force
Copy-Item (Join-Path $projectRoot 'third_party/raylib/LICENSE') (Join-Path $packageDir 'raylib-LICENSE.txt') -Force
Copy-Item (Join-Path $projectRoot 'README.md') $packageDir -Force
# Copy compiler runtime imports recursively; Windows system DLLs remain system-provided.
$queue = [System.Collections.Generic.Queue[string]]::new()
$queue.Enqueue((Join-Path $packageDir 'gobang.exe'))
$seen = @{}
while ($queue.Count -gt 0) {
    $binary = $queue.Dequeue()
    $imports = & (Join-Path $CompilerRoot 'bin/objdump.exe') -p $binary
    if ($LASTEXITCODE -ne 0) { throw "Cannot inspect imports: $binary" }
    foreach ($line in $imports) {
        if ($line -match 'DLL Name:\s*(\S+)') {
            $dll = $Matches[1]
            $source = Join-Path $CompilerRoot "bin/$dll"
            if ((Test-Path $source) -and -not $seen.ContainsKey($dll)) {
                $seen[$dll] = $true
                $dest = Join-Path $packageDir $dll
                Copy-Item $source $dest -Force
                $queue.Enqueue($dest)
            }
        }
    }
}
$archive = Join-Path $projectRoot 'dist/Gomoku-Windows-x64.zip'
Compress-Archive -Path (Join-Path $packageDir '*') -DestinationPath $archive -Force
Write-Output "Package: $archive"
