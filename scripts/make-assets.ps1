<#
.SYNOPSIS
    生成占位级别的默认宠物帧动画（PNG，32 位带 alpha）。

.DESCRIPTION
    本仓库不附带美术资源。这个脚本用 System.Drawing 画一只简单的雪绒形象，
    让程序开箱就能显示动画；替换 assets/pet/<id>/ 下的 PNG 即可换成正式美术。

    产物：
      assets/pet/default/idle/0..5.png   （6 帧，8fps，循环）
      assets/pet/default/talk/0..5.png   （6 帧，10fps，循环）
      assets/pet/default/drag/0..1.png   （2 帧，6fps，循环）
      assets/pet/default/wave/0..3.png   （4 帧，8fps，循环）
#>
[CmdletBinding()]
param(
    [int]$Size = 240,
    [string]$PetId = 'default'
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

$root = Split-Path -Parent $PSScriptRoot
$petDir = Join-Path $root "assets\pet\$PetId"

function New-Canvas {
    $bitmap = New-Object System.Drawing.Bitmap($Size, $Size, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $graphics.Clear([System.Drawing.Color]::Transparent)
    return @{ Bitmap = $bitmap; Graphics = $graphics }
}

function Draw-Body {
    param(
        [System.Drawing.Graphics]$Graphics,
        [double]$Breath,     # -1..1
        [double]$MouthOpen,  # 0..1
        [double]$WaveAngle,  # 0..360，<0 表示不挥手
        [bool]$Blink,
        [double]$Squash      # 拖拽时的拉伸
    )

    $cx = $Size / 2.0
    $cy = $Size * 0.58
    $baseRadius = $Size * 0.30

    $breathScale = 1.0 + 0.035 * $Breath
    $radiusX = $baseRadius * $breathScale * (1.0 + 0.10 * $Squash)
    $radiusY = $baseRadius * $breathScale * (1.0 - 0.10 * $Squash)

    # 地面阴影
    $shadowRect = New-Object System.Drawing.RectangleF(
        [single]($cx - $radiusX * 0.85), [single]($cy + $radiusY * 0.86),
        [single]($radiusX * 1.7), [single]($radiusY * 0.28))
    $shadowBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(38, 40, 60, 110))
    $Graphics.FillEllipse($shadowBrush, $shadowRect)
    $shadowBrush.Dispose()

    # 围巾 / 飘带
    $ribbonPen = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(220, 120, 190, 235), [single]($Size * 0.035))
    $ribbonPen.StartCap = [System.Drawing.Drawing2D.LineCap]::Round
    $ribbonPen.EndCap = [System.Drawing.Drawing2D.LineCap]::Round
    $ribbonPath = New-Object System.Drawing.Drawing2D.GraphicsPath
    $ribbonPath.AddBezier(
        [single]($cx + $radiusX * 0.45), [single]($cy + $radiusY * 0.15),
        [single]($cx + $radiusX * 1.15), [single]($cy + $radiusY * 0.05),
        [single]($cx + $radiusX * 1.05), [single]($cy + $radiusY * 0.55),
        [single]($cx + $radiusX * 1.35), [single]($cy + $radiusY * 0.80))
    $Graphics.DrawPath($ribbonPen, $ribbonPath)
    $ribbonPath.Dispose()
    $ribbonPen.Dispose()

    # 身体（径向渐变）
    $bodyRect = New-Object System.Drawing.RectangleF(
        [single]($cx - $radiusX), [single]($cy - $radiusY),
        [single]($radiusX * 2), [single]($radiusY * 2))
    $gradient = New-Object System.Drawing.Drawing2D.LinearGradientBrush(
        $bodyRect,
        [System.Drawing.Color]::FromArgb(255, 255, 255, 255),
        [System.Drawing.Color]::FromArgb(255, 205, 228, 248),
        90.0)
    $Graphics.FillEllipse($gradient, $bodyRect)
    $gradient.Dispose()

    # 身体描边
    $outline = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(210, 138, 176, 214), [single]($Size * 0.012))
    $Graphics.DrawEllipse($outline, $bodyRect)
    $outline.Dispose()

    # 高光
    $glossRect = New-Object System.Drawing.RectangleF(
        [single]($cx - $radiusX * 0.55), [single]($cy - $radiusY * 0.75),
        [single]($radiusX * 0.55), [single]($radiusY * 0.38))
    $gloss = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(150, 255, 255, 255))
    $Graphics.FillEllipse($gloss, $glossRect)
    $gloss.Dispose()

    # 呆毛
    $ahogePen = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(230, 118, 160, 205), [single]($Size * 0.016))
    $ahogePen.StartCap = [System.Drawing.Drawing2D.LineCap]::Round
    $ahogePen.EndCap = [System.Drawing.Drawing2D.LineCap]::Round
    $Graphics.DrawBezier(
        $ahogePen,
        [single]($cx - $radiusX * 0.12), [single]($cy - $radiusY * 0.98),
        [single]($cx + $radiusX * 0.10), [single]($cy - $radiusY * 1.35),
        [single]($cx + $radiusX * 0.42), [single]($cy - $radiusY * 1.10),
        [single]($cx + $radiusX * 0.30), [single]($cy - $radiusY * 0.86))
    $ahogePen.Dispose()

    # 眼睛
    $eyeOffsetX = $radiusX * 0.38
    $eyeOffsetY = -$radiusY * 0.12
    $eyeRadius = $radiusX * 0.12
    $eyeBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(240, 46, 58, 78))

    foreach ($side in @(-1, 1)) {
        $ex = $cx + $side * $eyeOffsetX
        $ey = $cy + $eyeOffsetY
        if ($Blink) {
            $blinkPen = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(240, 46, 58, 78), [single]($Size * 0.012))
            $blinkPen.StartCap = [System.Drawing.Drawing2D.LineCap]::Round
            $blinkPen.EndCap = [System.Drawing.Drawing2D.LineCap]::Round
            $Graphics.DrawLine($blinkPen, [single]($ex - $eyeRadius), [single]$ey, [single]($ex + $eyeRadius), [single]$ey)
            $blinkPen.Dispose()
        } else {
            $eyeRect = New-Object System.Drawing.RectangleF(
                [single]($ex - $eyeRadius), [single]($ey - $eyeRadius * 1.25),
                [single]($eyeRadius * 2), [single]($eyeRadius * 2.5))
            $Graphics.FillEllipse($eyeBrush, $eyeRect)

            $pupilRect = New-Object System.Drawing.RectangleF(
                [single]($ex - $eyeRadius * 0.35), [single]($ey - $eyeRadius * 0.90),
                [single]($eyeRadius * 0.7), [single]($eyeRadius * 0.7))
            $pupilBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(235, 255, 255, 255))
            $Graphics.FillEllipse($pupilBrush, $pupilRect)
            $pupilBrush.Dispose()
        }
    }
    $eyeBrush.Dispose()

    # 腮红
    $blushBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(90, 255, 150, 175))
    foreach ($side in @(-1, 1)) {
        $bx = $cx + $side * $radiusX * 0.62
        $by = $cy + $radiusY * 0.20
        $blushRect = New-Object System.Drawing.RectangleF(
            [single]($bx - $radiusX * 0.15), [single]($by - $radiusY * 0.07),
            [single]($radiusX * 0.30), [single]($radiusY * 0.14))
        $Graphics.FillEllipse($blushBrush, $blushRect)
    }
    $blushBrush.Dispose()

    # 嘴
    $mouthY = $cy + $radiusY * 0.32
    if ($MouthOpen -gt 0.15) {
        $mouthRect = New-Object System.Drawing.RectangleF(
            [single]($cx - $radiusX * 0.16), [single]($mouthY - $radiusY * 0.04),
            [single]($radiusX * 0.32), [single]($radiusY * 0.10 + $radiusY * 0.16 * $MouthOpen))
        $mouthBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(230, 90, 110, 140))
        $Graphics.FillEllipse($mouthBrush, $mouthRect)
        $mouthBrush.Dispose()
    } else {
        $mouthPen = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(220, 90, 110, 140), [single]($Size * 0.011))
        $mouthPen.StartCap = [System.Drawing.Drawing2D.LineCap]::Round
        $mouthPen.EndCap = [System.Drawing.Drawing2D.LineCap]::Round
        $Graphics.DrawArc(
            $mouthPen,
            [single]($cx - $radiusX * 0.16), [single]($mouthY - $radiusY * 0.10),
            [single]($radiusX * 0.32), [single]($radiusY * 0.20),
            20, 140)
        $mouthPen.Dispose()
    }

    # 挥手的手臂
    if ($WaveAngle -ge 0) {
        $armPen = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(235, 168, 200, 230), [single]($Size * 0.028))
        $armPen.StartCap = [System.Drawing.Drawing2D.LineCap]::Round
        $armPen.EndCap = [System.Drawing.Drawing2D.LineCap]::Round
        $radians = $WaveAngle * [Math]::PI / 180.0
        $armStartX = $cx + $radiusX * 0.72
        $armStartY = $cy - $radiusY * 0.10
        $armEndX = $armStartX + [Math]::Cos($radians) * $radiusX * 0.55
        $armEndY = $armStartY - [Math]::Sin($radians) * $radiusY * 0.65
        $Graphics.DrawLine(
            $armPen,
            [single]$armStartX, [single]$armStartY,
            [single]$armEndX, [single]$armEndY)
        $armPen.Dispose()
    }
}

function Save-Frame {
    param(
        [System.Drawing.Bitmap]$Bitmap,
        [string]$Path
    )
    $directory = Split-Path -Parent $Path
    if (-not (Test-Path $directory)) {
        New-Item -ItemType Directory -Force -Path $directory | Out-Null
    }
    $Bitmap.Save($Path, [System.Drawing.Imaging.ImageFormat]::Png)
}

function New-Animation {
    param(
        [string]$Name,
        [int]$Frames,
        [scriptblock]$FrameStyle
    )
    for ($i = 0; $i -lt $Frames; $i++) {
        $canvas = New-Canvas
        $style = & $FrameStyle $i $Frames
        Draw-Body -Graphics $canvas.Graphics `
            -Breath $style.Breath `
            -MouthOpen $style.MouthOpen `
            -WaveAngle $style.WaveAngle `
            -Blink $style.Blink `
            -Squash $style.Squash
        Save-Frame -Bitmap $canvas.Bitmap -Path (Join-Path $petDir "$Name\$i.png")
        $canvas.Graphics.Dispose()
        $canvas.Bitmap.Dispose()
    }
    Write-Host ("  生成 {0}: {1} 帧" -f $Name, $Frames)
}

Write-Host "生成宠物资源到 $petDir"

New-Animation -Name 'idle' -Frames 6 -FrameStyle {
    param($i, $total)
    $phase = 2.0 * [Math]::PI * $i / $total
    [pscustomobject]@{
        Breath = [Math]::Sin($phase)
        MouthOpen = 0.0
        WaveAngle = -1.0
        Blink = ($i -eq 4)
        Squash = 0.0
    }
}

New-Animation -Name 'talk' -Frames 6 -FrameStyle {
    param($i, $total)
    [pscustomobject]@{
        Breath = 0.25
        MouthOpen = 0.5 + 0.5 * [Math]::Sin(2.0 * [Math]::PI * $i / $total)
        WaveAngle = -1.0
        Blink = $false
        Squash = 0.0
    }
}

New-Animation -Name 'drag' -Frames 2 -FrameStyle {
    param($i, $total)
    [pscustomobject]@{
        Breath = -0.2
        MouthOpen = 0.35
        WaveAngle = -1.0
        Blink = $false
        Squash = 1.0 - 1.6 * $i
    }
}

New-Animation -Name 'wave' -Frames 4 -FrameStyle {
    param($i, $total)
    [pscustomobject]@{
        Breath = 0.35
        MouthOpen = 0.25
        WaveAngle = 40.0 + 35.0 * [Math]::Sin(2.0 * [Math]::PI * $i / $total)
        Blink = $false
        Squash = 0.0
    }
}

Write-Host '完成。'
