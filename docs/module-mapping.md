# 原 Python/Qt 项目到 C++/WinUI 3 模块映射

| 原项目模块 | 目标模块 | 迁移策略 | 优先级 |
|---|---|---|---|
| `lib/core/qt_desktop_pet.py` | `AemeathPet.WinUI/windows/PetWindow` | 重写为 WinUI + Win32 互操作桌宠窗口 | P0 |
| `lib/script/main.py` | `App.xaml.cpp` + 应用控制器 | 重写生命周期启动/退出 | P0 |
| `lib/core/event` | `AemeathPet.Core/Events` | 类型安全事件总线替代动态回调 | P0 |
| `lib/core/timing` | `TimerScheduler`（后续） | C++ 定时器/线程池 | P1 |
| `lib/script/ui` | WinUI 3 XAML 窗口和页面 | 重做 UI，不直接迁移 Qt 控件 | P0/P1 |
| `config` | `AppSettings` + JSON Store | 先保留关键配置 | P0 |
| `resc/GIF` | `assets/pet/<id>/manifest.json` | 资源清单化 | P0 |
| `lib/script/chat` | `IChatService` + `ChatController` | MVP 使用 Mock；后续 OpenAI 兼容 | P1 |
| `lib/script/gsvmove` | `PetBehaviorStateMachine` | 首版 Idle/Dragging/Talking | P1 |
| `lib/script/microphone_stt` | `ISpeechRecognitionService` | 延后；可先空实现 | P2 |
| `lib/script/music` | `IMusicPlayerService` | 首版不接在线源 | P2 |
| `lib/script/obj-*` | `ObjectController`（后续） | 暂不迁移动态对象 | P3 |
| `lib/script/practical`/粒子 | `ParticleSystem`（后续） | 延后 | P3 |
| `services` | `AemeathPet.Services.*` | 按接口拆分 | P2/P3 |

## MVP 必须保留

- 桌宠显示
- 透明置顶无边框
- 拖拽和位置保存
- 右键菜单
- 托盘入口（可第二步补）
- 设置窗口
- 简化事件系统
- 用户数据目录与日志

## 首版应降级/延后

- 在线多源音乐
- Vosk STT
- ONNX TTS/GSVmove 语音包
- Ollama 自动引导
- 图片多模态
- 工具调用
- 粒子、小玩法、动态插件
