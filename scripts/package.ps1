<#
.SYNOPSIS
    构建并打包 AemeathPet 的 Windows x64 发行 ZIP。

.DESCRIPTION
    产物：
      dist/AemeathPet-<Version>-win64.zip
      dist/AemeathPet-<Version>-win64.zip.sha256

    ZIP 内是可直接运行的目录结构：
      AemeathPet.exe
      assets/...
      README-INSTALL.txt
      LICENSE-CODE（存在时）
#>
[CmdletBinding()]
param(
    [string]$Version = '0.1.0',
    [switch]$SkipBuild,
    [ValidateSet('Release', 'Debug')] [string]$Configuration = 'Release'
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
Push-Location $root
try {
    if (-not $SkipBuild) {
        Write-Host '==== 1/4 构建 ===='
        & pwsh -NoProfile -File (Join-Path $root 'scripts\build-msvc.ps1') -Configuration $Configuration
        if ($LASTEXITCODE -ne 0) { throw "构建失败，退出码 $LASTEXITCODE" }
    }

    $buildBin = Join-Path $root "build\msvc\bin"
    $exe = Join-Path $buildBin 'AemeathPet.exe'
    if (-not (Test-Path $exe)) { throw "找不到 $exe" }

    Write-Host '==== 2/4 准备 staging ===='
    $name = "AemeathPet-$Version-win64"
    $dist = Join-Path $root 'dist'
    $staging = Join-Path $dist $name
    if (Test-Path $staging) { Remove-Item $staging -Recurse -Force }
    New-Item -ItemType Directory -Force -Path $staging | Out-Null

    Copy-Item $exe $staging
    Copy-Item (Join-Path $root 'assets') (Join-Path $staging 'assets') -Recurse -Force

    foreach ($license in @('LICENSE-CODE', 'LICENSE', 'LICENSE-ASSETS')) {
        $path = Join-Path $root $license
        if (Test-Path $path) { Copy-Item $path $staging }
    }

    $readme = @"
飞行雪绒 (C++ 重构版) v$Version —— 免安装说明
==========================================

1. 双击 AemeathPet.exe 启动。
2. 右下角托盘图标 / 桌宠右键 提供：聊天、设置、点击穿透、窗口置顶、重载动画、打开数据目录、退出。
3. 左键拖动桌宠移动位置，退出时自动保存。
4. 用户数据保存在 %LOCALAPPDATA%\AemeathDeskPet：
     user/settings.json       用户设置
     user/secrets/            API Key（chat-api-key.txt）
     user/state/              聊天与论坛状态
     logs/aemeath.log         运行日志
     cache/                   可再生成缓存
5. 聊天默认走 mock 回复。要接真实模型，在设置里选择 openai / ollama / quick-welfare，
   并把 API Key 写入 %LOCALAPPDATA%\AemeathDeskPet\user\secrets\chat-api-key.txt。

卸载：直接删除本目录与 %LOCALAPPDATA%\AemeathDeskPet 即可（或使用安装器的卸载）。
"@
    Set-Content -Path (Join-Path $staging 'README-INSTALL.txt') -Value $readme -Encoding utf8

    Write-Host '==== 3/4 压缩 ===='
    $zip = Join-Path $dist "$name.zip"
    if (Test-Path $zip) { Remove-Item $zip -Force }
    Compress-Archive -Path (Join-Path $staging '*') -DestinationPath $zip -CompressionLevel Optimal

    Write-Host '==== 4/4 校验和 ===='
    $hash = (Get-FileHash $zip -Algorithm SHA256).Hash.ToLower()
    "$hash  $name.zip" | Set-Content -Path "$zip.sha256" -Encoding ascii

    $sizeMb = [math]::Round((Get-Item $zip).Length / 1MB, 2)
    Write-Host ""
    Write-Host "打包完成：$zip（$sizeMb MB）"
    Write-Host "SHA256  ：$hash"
}
finally {
    Pop-Location
}
