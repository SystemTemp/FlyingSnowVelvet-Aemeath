#pragma once

#include <d2d1.h>
#include <wincodec.h>
#include <wrl/client.h>

#include <filesystem>
#include <string>
#include <unordered_map>

namespace Aemeath::Host {

/// 用 Direct2D + WIC 把 PNG 帧画进 32 位 DIB，再通过 UpdateLayeredWindow 送到分层窗口。
class PetRenderer final {
public:
    PetRenderer() = default;
    ~PetRenderer();

    PetRenderer(const PetRenderer&) = delete;
    PetRenderer& operator=(const PetRenderer&) = delete;

    bool Initialize(HWND hwnd);

    /// 解码 PNG（带缓存），返回原始像素尺寸。
    bool EnsureFrame(const std::filesystem::path& pngPath, UINT* width, UINT* height);

    /// 把一帧按目标尺寸绘制到 (x, y)，并更新分层窗口。
    bool RenderFrame(
        const std::filesystem::path& pngPath,
        int displayWidth,
        int displayHeight,
        int x,
        int y,
        BYTE opacity = 255);

    /// 只把窗口移动到新位置（复用上一帧像素）。
    bool MoveTo(int x, int y);

    void ResetLayer();
    void Clear();

    [[nodiscard]] int FrameWidth() const noexcept { return m_width; }
    [[nodiscard]] int FrameHeight() const noexcept { return m_height; }

private:
    bool EnsureSurface(int width, int height);
    bool DecodeFrame(const std::filesystem::path& pngPath, ID2D1Bitmap** bitmap, UINT* width, UINT* height);
    bool Present(int x, int y, BYTE opacity);

    HWND m_hwnd { nullptr };
    Microsoft::WRL::ComPtr<ID2D1Factory> m_d2dFactory;
    Microsoft::WRL::ComPtr<IWICImagingFactory> m_wicFactory;
    Microsoft::WRL::ComPtr<ID2D1DCRenderTarget> m_renderTarget;

    HDC m_memoryDC { nullptr };
    HBITMAP m_bitmap { nullptr };
    void* m_bits { nullptr };
    int m_width { 0 };
    int m_height { 0 };
    bool m_presentedOnce { false };

    std::unordered_map<std::wstring, Microsoft::WRL::ComPtr<ID2D1Bitmap>> m_cache;
    std::unordered_map<std::wstring, D2D1_SIZE_U> m_sourceSizes;
};

} // namespace Aemeath::Host
