# Build rkk_esdoom_lib.dll (CLion MinGW) and rkk_esdoom_lib.so (CLion WSL cmake + Debian gcc).
$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot # Root of the project
$Native = Join-Path $Root "native" # Native folder

# Change these paths to your own MinGW, CMake, Ninja, and WSL paths. (this script maked for my own setup in windows, you can use it as a template, good luck ;D)
$Mingw = "Z:\JetBrains\Apps\CLion\bin\mingw\bin"
$Cmake = "Z:\JetBrains\Apps\CLion\bin\cmake\win\x64\bin\cmake.exe"
$Ninja = "Z:\JetBrains\Apps\CLion\bin\ninja\win\x64\ninja.exe"
$WslCmake = "/mnt/z/JetBrains/Apps/CLion/bin/cmake/wsl/x64/bin/cmake"
$WslNinja = "/mnt/z/JetBrains/Apps/CLion/bin/ninja/wsl/x64/ninja"

# Build for Windows
$env:PATH = "$Mingw;" + $env:PATH
& $Cmake -G Ninja -S $Native -B (Join-Path $Native "build-win") `
    -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_C_COMPILER="$($Mingw -replace '\\','/')/gcc.exe" `
    -DCMAKE_MAKE_PROGRAM="$Ninja"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& $Ninja -C (Join-Path $Native "build-win")
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

# Build for Linux
$linux = "cd /mnt/z/Repos/rkk-project-esdoom/native && $WslCmake -G Ninja -S . -B build-linux -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=gcc -DCMAKE_MAKE_PROGRAM=$WslNinja && $WslNinja -C build-linux"
wsl bash -lc $linux
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
