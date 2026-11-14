# AemeathPetCpp 架构设计

## 1. 分层

| 层级 | 目录 | 职责 |
|---|---|---|
| UI 层 | `src/AemeathPet.WinUI` | WinUI 3 窗口、页面、XAML、Win32 互操作、托盘入口 |
| 应用层 | 后续 `AemeathPet.Application` | 生命周期、控制器、模块编排 |
| 核心层 | `src/AemeathPet.Core` | 事件总线、宠物状态、配置模型、聊天模型 |
| 服务接口层 | `src/AemeathPet.Services` | AI、TTS、STT、音乐、资源、设置等接口 |
| 资源层 | `assets` | 宠物帧动画、默认配置、音效与角色资源 |

## 2. 核心原则

1. WinUI 只承担视图和用户交互，不写业务规则。
2. 事件、状态机、配置模型使用普通 C++20 编写，尽可能平台无关。
3. 语音、音乐、聊天全部通过接口接入；MVP 先用 Mock。
4. 桌宠窗口不要只依赖普通 WinUI 控件，需要保留 Win32 互操作边界。
5. 运行时数据写入 `%LOCALAPPDATA%/AemeathDeskPet`，程序目录只读。

## 3. MVP 模块

```text
EventBus
  ↓
PetController / ChatController / SettingsController
  ↓
PetWindow / ChatWindow / SettingsWindow
```

## 4. 事件类型

- `AppStarted`
- `AppStopping`
- `PetStateChanged`
- `PetPositionChanged`
- `PetActionRequested`
- `ChatMessageSubmitted`
- `ChatMessageReceived`
- `SettingsChanged`

## 5. 桌宠窗口实现建议

WinUI 3 负责内容层，Win32 负责桌宠窗口行为：

- 获取 HWND：`IWindowNative::get_WindowHandle`
- 置顶：`SetWindowPos(HWND_TOPMOST)`
- 工具窗口：`WS_EX_TOOLWINDOW`
- 不激活或穿透：`WS_EX_NOACTIVATE` / `WS_EX_TRANSPARENT`
- 分层透明：`WS_EX_LAYERED` 或 DirectComposition

## 6. 后续阶段

1. MVP：透明桌宠、拖拽、设置、Mock 聊天。
2. AI：OpenAI 兼容接口、流式输出、人格文本、历史裁剪。
3. 语音：Windows Speech/TTS，再考虑 ONNX/Vosk。
4. 音乐：本地播放，再考虑在线音乐源。
5. 插件：先静态对象，再设计插件 ABI。
