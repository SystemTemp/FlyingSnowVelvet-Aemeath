#include "PetRenderer.h"

#include "Aemeath/Core/Logging/Logger.h"

#include <windowsx.h>

#include <algorithm>
#include <string>

namespace Aemeath::Host {

using Microsoft::WRL::ComPtr;

namespace {

std::wstring ToCacheKey(const std::filesystem::path& path) {
    return path.wstring();
}

} // namespace

PetRenderer::~PetRenderer() {
    Clear();
}

bool PetRenderer::Initialize(HWND hwnd) {
    m_hwnd = hwnd;

    if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, IID_PPV_ARGS(&m_d2dFactory)))) {
        Aemeath::Core::Logging::Logger::Instance().Error("render", "D2D1CreateFactory 失败");
        return false;
    }

    if (FAILED(CoCreateInstance(
            CLSID_WICImagingFactory,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&m_wicFactory)))) {
        Aemeath::Core::Logging::Logger::Instance().Error("render", "WIC 工厂创建失败");
        return false;
    }

    const D2D1_RENDER_TARGET_PROPERTIES properties = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

    if (FAILED(m_d2dFactory->CreateDCRenderTarget(&properties, m_renderTarget.GetAddressOf()))) {
        Aemeath::Core::Logging::Logger::Instance().Error("render", "D2D DC 渲染目标创建失败");
        return false;
    }

    return true;
}

bool PetRenderer::EnsureSurface(int width, int height) {
    if (width <= 0 || height <= 0) {
        return false;
    }
    if (m_bitmap != nullptr && m_memoryDC != nullptr && m_width == width && m_height == height) {
        return true;
    }

    ResetLayer();

    BITMAPINFO info {};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = width;
    // 负高度 = top-down DIB，和 D2D 的坐标方向一致。
    info.bmiHeader.biHeight = -height;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;

    m_memoryDC = CreateCompatibleDC(nullptr);
    if (m_memoryDC == nullptr) {
        return false;
    }

    m_bitmap = CreateDIBSection(m_memoryDC, &info, DIB_RGB_COLORS, &m_bits, nullptr, 0);
    if (m_bitmap == nullptr) {
        DeleteDC(m_memoryDC);
        m_memoryDC = nullptr;
        return false;
    }

    SelectObject(m_memoryDC, m_bitmap);
    m_width = width;
    m_height = height;

    const RECT rect { 0, 0, width, height };
    if (FAILED(m_renderTarget->BindDC(m_memoryDC, &rect))) {
        Aemeath::Core::Logging::Logger::Instance().Error("render", "BindDC 失败");
        return false;
    }
    return true;
}

bool PetRenderer::DecodeFrame(
    const std::filesystem::path& pngPath,
    ID2D1Bitmap** bitmap,
    UINT* width,
    UINT* height) {
    const auto key = ToCacheKey(pngPath);

    if (const auto it = m_cache.find(key); it != m_cache.end()) {
        *bitmap = it->second.Get();
        const auto size = m_sourceSizes[key];
        *width = size.width;
        *height = size.height;
        return true;
    }

    ComPtr<IWICBitmapDecoder> decoder;
    if (FAILED(m_wicFactory->CreateDecoderFromFilename(
            pngPath.c_str(),
            nullptr,
            GENERIC_READ,
            WICDecodeMetadataCacheOnDemand,
            &decoder))) {
        return false;
    }

    ComPtr<IWICBitmapFrameDecode> frame;
    if (FAILED(decoder->GetFrame(0, &frame))) {
        return false;
    }

    UINT sourceWidth = 0;
    UINT sourceHeight = 0;
    if (FAILED(frame->GetSize(&sourceWidth, &sourceHeight)) || sourceWidth == 0 || sourceHeight == 0) {
        return false;
    }

    ComPtr<IWICFormatConverter> converter;
    if (FAILED(m_wicFactory->CreateFormatConverter(&converter))) {
        return false;
    }
    if (FAILED(converter->Initialize(
            frame.Get(),
            GUID_WICPixelFormat32bppPBGRA,
            WICBitmapDitherTypeNone,
            nullptr,
            0.0,
            WICBitmapPaletteTypeMedianCut))) {
        return false;
    }

    ComPtr<ID2D1Bitmap> converted;
    if (FAILED(m_renderTarget->CreateBitmapFromWicBitmap(converter.Get(), nullptr, &converted))) {
        return false;
    }

    m_sourceSizes[key] = D2D1::SizeU(sourceWidth, sourceHeight);
    m_cache[key] = converted;

    *bitmap = converted.Get();
    *width = sourceWidth;
    *height = sourceHeight;
    return true;
}

bool PetRenderer::EnsureFrame(const std::filesystem::path& pngPath, UINT* width, UINT* height) {
    if (m_renderTarget == nullptr) {
        return false;
    }
    // 解码需要一个已绑定 DC 的渲染目标；先用 1x1 表面兜底。
    if (m_bitmap == nullptr && !EnsureSurface(1, 1)) {
        return false;
    }
    ID2D1Bitmap* bitmap = nullptr;
    return DecodeFrame(pngPath, &bitmap, width, height);
}

bool PetRenderer::RenderFrame(
    const std::filesystem::path& pngPath,
    int displayWidth,
    int displayHeight,
    int x,
    int y,
    BYTE opacity) {
    if (m_renderTarget == nullptr) {
        return false;
    }
    if (!EnsureSurface(displayWidth, displayHeight)) {
        return false;
    }

    ID2D1Bitmap* bitmap = nullptr;
    UINT sourceWidth = 0;
    UINT sourceHeight = 0;
    if (!DecodeFrame(pngPath, &bitmap, &sourceWidth, &sourceHeight)) {
        return false;
    }

    const auto destination = D2D1::RectF(0.0f, 0.0f, static_cast<float>(displayWidth), static_cast<float>(displayHeight));
    const auto source = D2D1::RectF(0.0f, 0.0f, static_cast<float>(sourceWidth), static_cast<float>(sourceHeight));

    m_renderTarget->BeginDraw();
    m_renderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
    m_renderTarget->Clear(D2D1::ColorF(0, 0.0f));
    m_renderTarget->DrawBitmap(
        bitmap,
        destination,
        1.0f,
        D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
        source);
    const auto hr = m_renderTarget->EndDraw();
    if (FAILED(hr)) {
        Aemeath::Core::Logging::Logger::Instance().Error("render", "EndDraw 失败");
        // 设备丢失等情况：重建渲染目标后由下一帧重试。
        m_renderTarget.Reset();
        const D2D1_RENDER_TARGET_PROPERTIES fallback = D2D1::RenderTargetProperties(
            D2D1_RENDER_TARGET_TYPE_DEFAULT,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
        if (FAILED(m_d2dFactory->CreateDCRenderTarget(&fallback, m_renderTarget.GetAddressOf()))) {
            return false;
        }
        return false;
    }

    return Present(x, y, opacity);
}

bool PetRenderer::Present(int x, int y, BYTE opacity) {
    if (m_memoryDC == nullptr || m_hwnd == nullptr) {
        return false;
    }

    const auto screenDC = GetDC(nullptr);
    if (screenDC == nullptr) {
        return false;
    }

    POINT destination { x, y };
    POINT source { 0, 0 };
    SIZE size { m_width, m_height };
    BLENDFUNCTION blend {};
    blend.BlendOp = AC_SRC_OVER;
    blend.SourceConstantAlpha = opacity;
    blend.AlphaFormat = AC_SRC_ALPHA;

    const BOOL ok = UpdateLayeredWindow(
        m_hwnd,
        screenDC,
        &destination,
        &size,
        m_memoryDC,
        &source,
        0,
        &blend,
        ULW_ALPHA);

    ReleaseDC(nullptr, screenDC);
    if (!ok) {
        Aemeath::Core::Logging::Logger::Instance().Warn("render", "UpdateLayeredWindow 失败");
    } else if (!m_presentedOnce) {
        m_presentedOnce = true;
        Aemeath::Core::Logging::Logger::Instance().Info(
            "render",
            "\u9996\u5e27\u5df2\u5448\u73b0\uff1a" + std::to_string(m_width) + "x" + std::to_string(m_height)
                + " @ " + std::to_string(x) + "," + std::to_string(y));
    }
    return ok != FALSE;
}

bool PetRenderer::MoveTo(int x, int y) {
    return Present(x, y, 255);
}

void PetRenderer::ResetLayer() {
    if (m_bitmap != nullptr) {
        DeleteObject(m_bitmap);
        m_bitmap = nullptr;
    }
    if (m_memoryDC != nullptr) {
        DeleteDC(m_memoryDC);
        m_memoryDC = nullptr;
    }
    m_bits = nullptr;
    m_width = 0;
    m_height = 0;
}

void PetRenderer::Clear() {
    m_cache.clear();
    m_sourceSizes.clear();
    ResetLayer();
    m_renderTarget.Reset();
    m_wicFactory.Reset();
    m_d2dFactory.Reset();
    m_hwnd = nullptr;
}

} // namespace Aemeath::Host

