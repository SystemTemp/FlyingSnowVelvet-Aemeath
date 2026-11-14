param(
    [string]$Configuration = "Debug",
    [string]$Platform = "x64"
)

$ErrorActionPreference = "Stop"

Write-Host "AemeathPetCpp build helper"
Write-Host "Configuration: $Configuration"
Write-Host "Platform: $Platform"

if (-not (Test-Path "AemeathPet.sln")) {
    Write-Warning "未找到 AemeathPet.sln。请先在 Visual Studio 中创建 WinUI 3 C++ 解决方案，并合并 src/ 下源码。"
    exit 1
}

msbuild AemeathPet.sln /m /p:Configuration=$Configuration /p:Platform=$Platform
