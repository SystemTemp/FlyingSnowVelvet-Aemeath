<#
.SYNOPSIS
    飞行雪绒 (C++ 版) 在线安装器。

.DESCRIPTION
    两种安装来源：

    1) 默认（二进制在线安装）：从 GitHub Release 下载官方构建的 win64 ZIP，
       校验 SHA-256 后安装。最终用户不需要编译器。
    2) -FromSource（源码在线安装）：gh repo clone SystemTemp/FlyingSnowVelvet-Aemeath，
       用本机 Visual Studio 2022 生成工具链现场构建后再安装。适合开发者。

    安装位置默认 %LOCALAPPDATA%\Programs\AemeathPet，
    运行期数据始终在 %LOCALAPPDATA%\AemeathDeskPet。

.EXAMPLE
    .\install-online.ps1
    .\install-online.ps1 -Version v0.1.0
    .\install-online.ps1 -FromSource
    .\install-online.ps1 -Uninstall
#>
[CmdletBinding()]
param(
    [string]$Version = 'latest',
    [string]$Repo = 'SystemTemp/FlyingSnowVelvet-Aemeath',
    [string]$InstallDir = "$env:LOCALAPPDATA\Programs\AemeathPet",
    [switch]$FromSource,
    [switch]$Uninstall,
    [switch]$NoShortcut,
    [switch]$NoLaunch
)

$ErrorActionPreference = 'Stop'
$appName = '飞行雪绒'
$exeName = 'AemeathPet.exe'

function Write-Step([string]$Text) {
    Write-Host ""
    Write-Host "== $Text ==" -ForegroundColor Cyan
}

function Test-Command([string]$Name) {
    return [bool](Get-Command $Name -ErrorAction SilentlyContinue)
}

# ---------------------------------------------------------------------------
# 卸载
# ---------------------------------------------------------------------------
if ($Uninstall) {
    Write-Step "卸载 $appName"
    if (Test-Path $InstallDir) {
        Remove-Item $InstallDir -Recurse -Force
        Write-Host "已删除程序目录：$InstallDir"
    }

    $startMenu = Join-Path $env:APPDATA 'Microsoft\Windows\Start Menu\Programs'
    $shortcut = Join-Path $startMenu "$appName.lnk"
    if (Test-Path $shortcut) { Remove-Item $shortcut -Force }

    $desktop = [Environment]::GetFolderPath('Desktop')
    $desktopShortcut = Join-Path $desktop "$appName.lnk"
    if (Test-Path $desktopShortcut) { Remove-Item $desktopShortcut -Force }

    $uninstallKey = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\AemeathPet'
    if (Test-Path $uninstallKey) { Remove-Item $uninstallKey -Recurse -Force }

    Write-Host "已移除快捷方式与卸载注册项。"
    Write-Host "用户数据（设置/密钥/日志）保留在 $env:LOCALAPPDATA\AemeathDeskPet，如需彻底清理请手动删除。"
    return
}

# ---------------------------------------------------------------------------
# 取得安装包
# ---------------------------------------------------------------------------
$work = Join-Path $env:TEMP ("aemeath-install-" + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Force -Path $work | Out-Null

try {
    $payloadDir = $null

    if ($FromSource) {
        Write-Step '源码模式：gh repo clone + 本地构建'
        if (Test-Command gh) {
            Write-Host "使用 gh CLI 克隆 $Repo ..."
            & gh repo clone $Repo (Join-Path $work 'src') -- --depth 1
            if ($LASTEXITCODE -ne 0) { throw "gh repo clone 失败，退出码 $LASTEXITCODE" }
        } elseif (Test-Command git) {
            Write-Host "未找到 gh，改用 git clone ..."
            & git clone --depth 1 "https://github.com/$Repo.git" (Join-Path $work 'src')
            if ($LASTEXITCODE -ne 0) { throw "git clone 失败，退出码 $LASTEXITCODE" }
        } else {
            throw '需要 gh 或 git 才能使用 -FromSource。请先安装 GitHub CLI（https://cli.github.com/）。'
        }

        $sourceDir = Join-Path $work 'src'
        $buildScript = Join-Path $sourceDir 'scripts\build-msvc.ps1'
        if (-not (Test-Path $buildScript)) {
            throw "仓库里没有 scripts/build-msvc.ps1，说明该分支不是 C++ 版。"
        }

        Write-Host '开始本地构建（需要 Visual Studio 2022 生成工具）...'
        & pwsh -NoProfile -File $buildScript -Configuration Release
        if ($LASTEXITCODE -ne 0) { throw "构建失败，退出码 $LASTEXITCODE" }

        $payloadDir = Join-Path $sourceDir 'dist'
        & pwsh -NoProfile -File (Join-Path $sourceDir 'scripts\package.ps1') -SkipBuild -Version 'dev'
        if ($LASTEXITCODE -ne 0) { throw "打包失败，退出码 $LASTEXITCODE" }

        $zip = Get-ChildItem (Join-Path $sourceDir 'dist') -Filter '*.zip' |
            Sort-Object LastWriteTime -Descending | Select-Object -First 1
        if (-not $zip) { throw '没有找到构建产物 ZIP。' }
        Expand-Archive -Path $zip.FullName -DestinationPath (Join-Path $work 'payload') -Force
        $payloadDir = Join-Path $work 'payload'
    } else {
        Write-Step '二进制模式：从 GitHub Release 下载'
        $api = if ($Version -eq 'latest') {
            "https://api.github.com/repos/$Repo/releases/latest"
        } else {
            "https://api.github.com/repos/$Repo/releases/tags/$Version"
        }

        Write-Host "查询发布信息：$api"
        $release = Invoke-RestMethod -Uri $api -Headers @{ 'User-Agent' = 'AemeathPet-Setup' }
        Write-Host ("版本：{0}" -f $release.tag_name)

        $asset = $release.assets | Where-Object { $_.name -like '*win64*.zip' } | Select-Object -First 1
        if (-not $asset) {
            throw "该 Release 没有 win64 ZIP 资产。可用 -FromSource 从源码安装。"
        }

        $zipPath = Join-Path $work $asset.name
        Write-Host ("下载：{0}（{1:N1} MB）" -f $asset.name, ($asset.size / 1MB))
        Invoke-WebRequest -Uri $asset.browser_download_url -OutFile $zipPath -UseBasicParsing

        $shaAsset = $release.assets | Where-Object { $_.name -like "$($asset.name).sha256" } | Select-Object -First 1
        if ($shaAsset) {
            $shaPath = Join-Path $work ($shaAsset.name)
            Invoke-WebRequest -Uri $shaAsset.browser_download_url -OutFile $shaPath -UseBasicParsing
            $expected = ((Get-Content $shaPath -Raw) -split '\s+')[0].Trim().ToLower()
            $actual = (Get-FileHash $zipPath -Algorithm SHA256).Hash.ToLower()
            if ($expected -ne $actual) {
                throw "SHA-256 校验失败：期望 $expected，实际 $actual"
            }
            Write-Host 'SHA-256 校验通过。'
        } else {
            Write-Host '该发布没有 .sha256 资产，跳过校验。' -ForegroundColor Yellow
        }

        Expand-Archive -Path $zipPath -DestinationPath (Join-Path $work 'payload') -Force
        $payloadDir = Join-Path $work 'payload'
    }

    # 兼容 ZIP 里多套一层目录
    if ($payloadDir -and -not (Test-Path (Join-Path $payloadDir $exeName))) {
        $nested = Get-ChildItem $payloadDir -Directory | Select-Object -First 1
        if ($nested -and (Test-Path (Join-Path $nested.FullName $exeName))) {
            $payloadDir = $nested.FullName
        }
    }
    if (-not $payloadDir -or -not (Test-Path (Join-Path $payloadDir $exeName))) {
        throw "安装包里没有 $exeName。"
    }

    # -----------------------------------------------------------------------
    # 安装
    # -----------------------------------------------------------------------
    Write-Step "安装到 $InstallDir"
    if (Test-Path $InstallDir) {
        Remove-Item $InstallDir -Recurse -Force
    }
    New-Item -ItemType Directory -Force -Path $InstallDir | Out-Null
    Copy-Item (Join-Path $payloadDir '*') $InstallDir -Recurse -Force

    $exePath = Join-Path $InstallDir $exeName
    if (-not $NoShortcut) {
        Write-Step '创建快捷方式'
        $shell = New-Object -ComObject WScript.Shell

        $startMenu = Join-Path $env:APPDATA 'Microsoft\Windows\Start Menu\Programs'
        New-Item -ItemType Directory -Force -Path $startMenu | Out-Null
        $shortcut = $shell.CreateShortcut((Join-Path $startMenu "$appName.lnk"))
        $shortcut.TargetPath = $exePath
        $shortcut.WorkingDirectory = $InstallDir
        $shortcut.Description = '飞行雪绒桌面宠物 (C++)'
        $shortcut.Save()

        $desktop = [Environment]::GetFolderPath('Desktop')
        $desktopShortcut = $shell.CreateShortcut((Join-Path $desktop "$appName.lnk"))
        $desktopShortcut.TargetPath = $exePath
        $desktopShortcut.WorkingDirectory = $InstallDir
        $desktopShortcut.Description = '飞行雪绒桌面宠物 (C++)'
        $desktopShortcut.Save()
        Write-Host '已创建开始菜单与桌面快捷方式。'
    }

    Write-Step '注册卸载信息'
    $uninstallKey = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\AemeathPet'
    New-Item -Path $uninstallKey -Force | Out-Null
    $installerScript = Join-Path $InstallDir 'uninstall.ps1'
    $uninstallCommand = "powershell.exe -NoProfile -ExecutionPolicy Bypass -File `"$installerScript`" -Uninstall"
    Set-ItemProperty -Path $uninstallKey -Name DisplayName -Value "飞行雪绒 (C++)"
    Set-ItemProperty -Path $uninstallKey -Name DisplayVersion -Value $Version
    Set-ItemProperty -Path $uninstallKey -Name Publisher -Value 'SystemTemp'
    Set-ItemProperty -Path $uninstallKey -Name InstallLocation -Value $InstallDir
    Set-ItemProperty -Path $uninstallKey -Name DisplayIcon -Value $exePath
    Set-ItemProperty -Path $uninstallKey -Name UninstallString -Value $uninstallCommand
    Set-ItemProperty -Path $uninstallKey -Name NoModify -Value 1 -Type DWord
    Set-ItemProperty -Path $uninstallKey -Name NoRepair -Value 1 -Type DWord

    # 自带一份卸载脚本，保证卸载项可用
    $selfPath = $PSCommandPath
    if ($selfPath -and (Test-Path $selfPath)) {
        Copy-Item $selfPath $installerScript -Force
    }

    Write-Host ""
    Write-Host "安装完成：$exePath" -ForegroundColor Green
    Write-Host "用户数据目录：$env:LOCALAPPDATA\AemeathDeskPet"

    if (-not $NoLaunch) {
        Write-Step '启动'
        Start-Process -FilePath $exePath -WorkingDirectory $InstallDir
        Write-Host '已启动。'
    }
}
finally {
    if (Test-Path $work) {
        Remove-Item $work -Recurse -Force -ErrorAction SilentlyContinue
    }
}
