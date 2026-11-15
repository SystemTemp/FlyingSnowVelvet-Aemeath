<#
.SYNOPSIS
    用本机 Visual Studio 2022 生成工具链构建并运行 AemeathPet 的核心测试。

.DESCRIPTION
    不需要启动 Developer PowerShell：脚本自行定位 vcvars64.bat / CMake / Ninja，
    并把 vcvars 导出的环境注入当前进程（避免经过 cmd 传参，仓库路径含中文也安全）。

.PARAMETER Configuration
    Debug 或 Release，默认 Release。

.PARAMETER Target
    可选 CMake target 名，默认构建全部。

.PARAMETER RunTests
    构建后运行测试可执行文件。

.PARAMETER Clean
    删除构建目录后重新配置。
#>
[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')] [string]$Configuration = 'Release',
    [string]$Target,
    [switch]$RunTests,
    [switch]$Clean
)

$ErrorActionPreference = 'Stop'
$repo = (Split-Path -Parent $PSScriptRoot).TrimEnd('\')
Push-Location $repo
try {
    # ---- 定位工具链 ---------------------------------------------------------
    $vswhere = 'C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe'
    $vsPath = $null
    if (Test-Path $vswhere) {
        $candidates = & $vswhere -latest -products * -property installationPath 2>$null
        if ($candidates) {
            $vsPath = ($candidates | Where-Object { $_ -and $_.Trim() } | Select-Object -First 1)
        }
    }
    if (-not $vsPath) {
        # vswhere 不可用时的回退：直接扫描已知安装目录，找带 vcvars64.bat 的那个。
        $roots = @(
            'C:\Program Files\Microsoft Visual Studio\2022',
            'C:\Program Files (x86)\Microsoft Visual Studio\2022'
        )
        foreach ($root in $roots) {
            if (-not (Test-Path $root)) { continue }
            foreach ($edition in (Get-ChildItem $root -Directory -ErrorAction SilentlyContinue)) {
                if (Test-Path (Join-Path $edition.FullName 'VC\Auxiliary\Build\vcvars64.bat')) {
                    $vsPath = $edition.FullName
                    break
                }
            }
            if ($vsPath) { break }
        }
    }
    if (-not $vsPath) { throw '找不到 Visual Studio 2022（vcvars64.bat）安装。' }
    $vsPath = $vsPath.Trim()

    $vcvars = Join-Path $vsPath 'VC\Auxiliary\Build\vcvars64.bat'
    if (-not (Test-Path $vcvars)) { throw "找不到 vcvars64.bat：$vcvars" }

    $cmake = Join-Path $vsPath 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
    if (-not (Test-Path $cmake)) { $cmake = (Get-Command cmake -ErrorAction SilentlyContinue).Source }
    if (-not $cmake) { throw '找不到 CMake。' }

    $ninja = Join-Path $vsPath 'Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe'
    if (-not (Test-Path $ninja)) { $ninja = (Get-Command ninja -ErrorAction SilentlyContinue).Source }
    if (-not $ninja) { throw '找不到 Ninja。' }

    Write-Host "VS    : $vsPath"
    Write-Host "CMake : $cmake"
    Write-Host "Ninja : $ninja"

    # ---- 注入 vcvars 环境 ---------------------------------------------------
    if (-not $env:VSCMD_ARG_TGT_ARCH -or $env:VSCMD_ARG_TGT_ARCH -ne 'x64') {
        $dump = & cmd /c "call `"$vcvars`" >nul 2>&1 && set"
        foreach ($line in $dump) {
            if ($line -match '^([^=]+)=(.*)$') {
                $name = $Matches[1]
                $value = $Matches[2]
                try { Set-Item -Path "env:$name" -Value $value -ErrorAction Stop } catch { }
            }
        }
        Write-Host "已注入 vcvars64 环境（cl: $((Get-Command cl -ErrorAction SilentlyContinue).Source)）"
    }

    # ---- 配置与构建 ---------------------------------------------------------
    $buildDir = Join-Path $repo 'build\msvc'
    if ($Clean -and (Test-Path $buildDir)) {
        Write-Host "清理 $buildDir"
        Remove-Item $buildDir -Recurse -Force
    }

    if (-not (Test-Path (Join-Path $buildDir 'CMakeCache.txt'))) {
        Write-Host '---- configure ----'
        $env:PATH = "$(Split-Path -Parent $ninja);$env:PATH"
        $cfgArgs = @(
            '-S', $repo, '-B', $buildDir, '-G', 'Ninja',
            "-DCMAKE_BUILD_TYPE=$Configuration",
            '-DCMAKE_C_COMPILER=cl', '-DCMAKE_CXX_COMPILER=cl'
        )
        & $cmake @cfgArgs
        if ($LASTEXITCODE -ne 0) { throw "CMake 配置失败，退出码 $LASTEXITCODE" }
    }

    Write-Host '---- build ----'
    $buildArgs = @('--build', $buildDir)
    if ($Target) { $buildArgs += @('--target', $Target) }
    & $cmake @buildArgs
    if ($LASTEXITCODE -ne 0) { throw "构建失败，退出码 $LASTEXITCODE" }

    Write-Host "构建成功：$buildDir"

    if ($RunTests) {
        $testExe = Join-Path $buildDir 'bin\AemeathPet.Tests.exe'
        if (-not (Test-Path $testExe)) { throw "找不到测试程序：$testExe" }
        Write-Host '---- test ----'
        & $testExe
        if ($LASTEXITCODE -ne 0) { throw "测试失败，退出码 $LASTEXITCODE" }
    }
}
finally {
    Pop-Location
}

