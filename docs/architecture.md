# 架构（C++ 重构版 v0.1.0）

## 1. 分层

```
┌─────────────────────────────────────────────────────────────┐
│ AemeathPet.Host（Win32 可执行）                              │
│   main.cpp            进程入口、单实例、服务组装、命令分发     │
│   PetHostWindow       分层透明置顶窗口、拖拽、右键、帧定时器   │
│   PetRenderer         Direct2D + WIC → DIB → UpdateLayeredWindow │
│   TrayIcon            托盘图标与菜单                          │
│   SettingsDialog      设置窗口（原生控件）                     │
│   ChatDialog          聊天窗口（后台线程取流式回复）           │
├─────────────────────────────────────────────────────────────┤
│ AemeathPet.Application（编排）                                │
│   AppController       设置加载/保存、聊天转发、留言墙查询      │
├─────────────────────────────────────────────────────────────┤
│ AemeathPet.Infrastructure（实现）                             │
│   Http/WinHttpClient  WinHTTP：普通请求 + 流式字节回调         │
│   Http/MockHttpClient 可编程假客户端（离线测试）               │
│   Chat/StreamingChatSession  OpenAI 兼容 SSE / Ollama NDJSON   │
│   Chat/ChatProviderRegistry  provider 选择与默认值             │
│   Forum/ForumApiClient       论坛 10 端点                      │
│   Config/JsonSettingsStore   JSON 稀疏读写 + 原子替换          │
│   Pet/PetAssetLoader         manifest.json → AnimationLibrary  │
│   Json/SimpleJson            自研解析/序列化                    │
├─────────────────────────────────────────────────────────────┤
│ AemeathPet.Services（接口，无实现）                            │
│   IChatService IStreamingChatService IChatProvider             │
│   IHttpClient IStreamingHttpClient IForumService               │
│   ISettingsStore IMusicPlayerService IMusicProvider            │
│   ITextToSpeechService ISpeechRecognitionService               │
│   IUpdateService IWallService                                  │
├─────────────────────────────────────────────────────────────┤
│ AemeathPet.Core（平台无关）                                    │
│   Events/EventBus     类型安全事件总线（any + type_index）     │
│   Pet/PetState        状态与位置                               │
│   Pet/PetBehaviorStateMachine  Idle/Dragging/Talking/Hidden    │
│   Pet/AnimationPlayer 帧调度（纯时间驱动，可单测）             │
│   Pet/PetDirector     闲时随机动作（xorshift 可播种）          │
│   Logging/Logger      线程安全日志（文件 + OutputDebugString） │
│   Platform/UserPaths  数据目录分层（可覆盖根）                 │
└─────────────────────────────────────────────────────────────┘
```

依赖方向严格单向：`Host → Application → Infrastructure → Services → Core`。

## 2. 桌宠窗口为什么不用 WinUI 3

需求是「每帧贴图 + 透明分层 + 点击穿透 + 置顶 + 不抢焦点」。WinUI 3 的 XAML 层做这些最终都要回到 Win32/D2D，还要额外引入 Windows App SDK 运行时与 MSIX 打包；而纯 Win32 宿主只需 Windows SDK。

渲染链路：

```
WIC 解码 PNG → ID2D1Bitmap（按路径缓存）
        ↓
ID2D1DCRenderTarget（BindDC 到 32bpp top-down DIBSection）
        ↓
BeginDraw / Clear(透明) / DrawBitmap(缩放) / EndDraw
        ↓
UpdateLayeredWindow(hwnd, ..., ULW_ALPHA)  ← 一步完成位置 + 尺寸 + 像素
```

因此移动窗口不需要 `SetWindowPos`，直接带上新坐标重新 present 即可。

## 3. 帧循环

```
SetTimer(16ms) → OnTimer
  ├─ 计算真实 delta（挂起后钳制到 16ms，避免一次补太多帧）
  ├─ 若处于 Talking 且超过 4.2s → EndTalking
  ├─ PetDirector.Tick(delta, activity)
  │    只在 Idle 下工作；返回建议播放的动画 id
  ├─ AnimationPlayer.Advance(delta) → 当前帧
  └─ 帧变化时 RenderCurrentFrame()
```

`AnimationPlayer` 与 `PetDirector` 都不依赖窗口系统，所以可以用假时间做单元测试（见 `AnimationTests.cpp`）。

## 4. 聊天链路

```
ChatDialog（UI 线程）
  └─ 点击发送 → 起后台线程 → m_sendHandler(prompt) → std::future
                         ↓
        ChatServiceFactory::Create(config, http)
                         ↓
        StreamingChatSession::SendStreamingAsync
                         ├─ IStreamingHttpClient 可用 → SSE/NDJSON 逐块回调
                         └─ 否则 → 普通请求一次性解析
                         ↓
        回复通过 PostMessage(WM_APP+9) 投回 UI 线程追加到记录
```

只有用户选中的来源会被使用，不跨来源自动回退（与原版策略一致）；出错时返回 `id="error"` 的友好消息而不是抛异常穿透 UI。

## 5. 数据目录

```
%LOCALAPPDATA%\AemeathDeskPet\
  user\settings.json     稀疏设置（只写偏离默认值的字段）
  user\secrets\          chat-api-key.txt 等
  user\state\            聊天/论坛状态
  cache\                 可再生成
  logs\aemeath.log       滚动式追加
```

`AEMEATH_DESK_PET_HOME` 可整体覆盖根目录（便于便携/测试）。

## 6. 构建与验证

- CMake + MSVC（Ninja 或 VS 生成器）
- `/utf-8 /permissive- /W4`，`UNICODE WIN32_LEAN_AND_MEAN NOMINMAX`
- `scripts/build-msvc.ps1` 自动注入 vcvars64 环境（不要求预先打开开发者命令行）
- `AemeathPet.Tests` 39 个用例；CI 在 windows-latest 上构建并运行
