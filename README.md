# 飞行雪绒 · C++ 重构版

> 原项目：[FlyingSnowVelvet-Aemeath](https://github.com/MARK42IRPC/FlyingSnowVelvet-Aemeath)（Python / PySide6）
> 本仓库：同名的 **Windows x64 原生 C++20 重构版**，不再跟随上游 Python 代码同步。

目标是把原版的桌宠体验用原生代码重做：无 Python 运行时、无 Qt、单 EXE、透明置顶窗口直绘。

---

## 当前状态（v0.1.0）

**已经能跑起来**，是一个可交互的桌宠 MVP，而不是只有骨架：

| 能力 | 状态 | 说明 |
|---|---|---|
| 透明置顶无边框桌宠窗口 | ✅ | `WS_EX_LAYERED + WS_EX_TOOLWINDOW + WS_EX_TOPMOST`，Direct2D 画进 32bpp DIB 后 `UpdateLayeredWindow` |
| 帧动画 | ✅ | `manifest.json` 驱动的序列帧；支持 idle / talk / drag / wave，逐帧缓存解码（WIC） |
| 左键拖拽移动 | ✅ | 拖动即时移动窗口，退出自动保存坐标 |
| 右键菜单 + 托盘图标 | ✅ | 聊天、设置、点击穿透、置顶、重载动画、打开数据目录、退出 |
| 点击穿透 | ✅ | 运行时切换 `WS_EX_TRANSPARENT` |
| 闲时自动动作 | ✅ | `PetDirector` 随机间隔播放资源里存在的闲时动画，拖拽/聊天时抑制 |
| 聊天（流式） | ✅ | OpenAI 兼容 SSE、Ollama NDJSON、Mock 三种通道；`Enter` 发送、后台线程取回复 |
| 设置窗口 | ✅ | 置顶、穿透、缩放、AI 来源、endpoint、模型、人格文件、TTS 开关（写回 `settings.json`） |
| 设置持久化 | ✅ | JSON 稀疏读写 + 原子替换（tmp → rename） |
| 论坛/留言墙 API | ✅ | 10 个端点的完整客户端（登录/发帖/回复/点赞/图片/标签），Bearer、UA 与原版一致 |
| 日志 / 单实例 / 用户目录 | ✅ | `%LOCALAPPDATA%\AemeathDeskPet\{user,cache,logs}` |
| 自动化测试 | ✅ | 40 个用例，覆盖事件总线、JSON、设置、HTTP、动画、聊天 SSE、论坛解析 |
| 在线安装 | ✅ | `installer/install-online.ps1`（免编译，从 Release 下载）或 `-FromSource`（gh clone + 本地构建） |
| CI | ✅ | GitHub Actions：MSVC 构建 + 跑测试 |

### 与 Python 原版相比仍然缺的（按优先级）

1. **渲染层深度**：原版有图层系统、粒子系统（22 个粒子脚本）、离屏合成、DPI 多屏、屏幕捕获、DirectX 原生后端（`native/dx_backend`，C ABI v10）。本版只有单窗口单图元的 Direct2D 贴图。
2. **AI 对话完整度**：原版 5 条来源车道（快捷福利池的模型清单轮询/冷却/多模态优先、备用福利双源取密钥、手动 OpenAI 兼容、Ollama 自动引导、规则回复）+ 上下文记忆 + 人格存储 + 工具调用 + 视觉输入。本版只有 3 条简化通道 + 一次会话内历史 + 人格文件。
3. **语音**：原版 ONNX 语音包安装 + 本地 TTS 推理 + CUDA/DirectML overlay + Vosk 本地识别 + Push-to-Talk + 降噪。本版 TTS/STT 尚是接口占位（SAPI 实现文件已规划，未接入本轮构建）。
4. **音乐**：原版 QQ/网易云/酷狗三 provider、`pyncm` EAPI/WEAPI 加密、cloudmusic 播放运行时、B 站直播点歌与礼物告警。本版无。
5. **论坛 UI**：原版 40+ 个 `forum_*` 组件（板块、编辑器、详情、Markdown、贴纸、配色、图片墙）。本版只有 API 客户端。
6. **办公 DSH 侧车、游戏包（拉海俄罗斯方块）、世界对象（音响/雪球/雪堆/雪豹/沙发/摩托/闹钟）**：本版无。
7. **更新器 / 原生安装器**：原版有 C 写的完整安装器/卸载器/更新器（4 路 LZMA2 分片、SHA-256 校验、进度 UI）。本版是 PowerShell 在线安装器 + ZIP。
8. **资源与美术**：原版 `resc/` 有 2208 个文件（GIF 动画、字体、音效、Vosk 模型）。本版自带的是脚本生成的占位形象（`scripts/make-assets.ps1`）。

> 说明：本版刻意选择 **Win32 + Direct2D 自绘** 而不是 WinUI 3。原因是透明分层、逐帧合成、点击穿透、每帧贴图这些需求走 XAML 控件层会不断回落到 Win32/D2D，反而不如直接自绘可控，而且不依赖 Windows App SDK / MSIX 打包。`src/AemeathPet.WinUI` 里的 XAML 骨架保留作后续可选前端，不参与主构建。

---

## 构建

要求：Windows 10/11 x64、**Visual Studio 2022 生成工具（MSVC v143 + Windows SDK 10）**、CMake 3.25+（VS 自带即可）。

```powershell
# 一键构建 + 跑测试（脚本自动定位 vcvars64 / CMake / Ninja）
pwsh -File scripts/build-msvc.ps1 -Configuration Release -RunTests
```

手动构建：

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
.\build\bin\AemeathPet.Tests.exe     # 40 个用例
.\build\bin\AemeathPet.exe           # 启动桌宠
```

> 可执行文件需要与 `assets/` 同级。`build-msvc.ps1` 不会自动复制，运行前请把 `assets` 拷到 `build\msvc\bin\`，或直接使用 `scripts/package.ps1` 产出的发行目录。

## 打包

```powershell
pwsh -File scripts/package.ps1 -Version 0.1.0
# → dist/AemeathPet-0.1.0-win64.zip  +  .sha256
```

## 安装

**在线安装（推荐，最终用户）**：

```powershell
pwsh -File installer/install-online.ps1
```

从 GitHub Release 下载官方构建、校验 SHA-256、安装到 `%LOCALAPPDATA%\Programs\AemeathPet`，创建开始菜单/桌面快捷方式与卸载项。

**源码安装（开发者）**：

```powershell
pwsh -File installer/install-online.ps1 -FromSource
```

用 `gh repo clone SystemTemp/FlyingSnowVelvet-Aemeath` 拉源码，再用本机 MSVC 现场构建并安装。

**卸载**：

```powershell
pwsh -File installer/install-online.ps1 -Uninstall
```

## 运行期数据

默认在 `%LOCALAPPDATA%\AemeathDeskPet`（可用 `AEMEATH_DESK_PET_HOME` 覆盖）：

```
user/settings.json      用户设置（只保存偏离默认值的字段）
user/secrets/           chat-api-key.txt 等密钥
user/state/             聊天 / 论坛会话状态
cache/                  可再生成缓存
logs/aemeath.log        运行日志
```

## 目录结构

```
CMakeLists.txt
assets/                  默认配置、人格、宠物帧资源（脚本生成的占位形象）
docs/                    架构、模块映射、MVP 范围、原版 API 映射、差距对照
installer/               在线安装器（二进制模式 / 源码模式 / 卸载）
scripts/                 build-msvc.ps1、package.ps1、make-assets.ps1
src/
  AemeathPet.Core/         事件总线、宠物状态机、帧调度、行为导演、日志、用户目录
  AemeathPet.Services/     服务接口（聊天/流式聊天/provider/HTTP/论坛/音乐/语音/更新/设置）
  AemeathPet.Infrastructure/ WinHTTP、JSON、设置、OpenAI 兼容 SSE、Ollama、论坛客户端、资源加载
  AemeathPet.Application/  编排层（AppController）
  AemeathPet.Host/         Win32 宿主：桌宠窗口、D2D 渲染、托盘、设置/聊天窗口、入口
  AemeathPet.Tests/        40 个自研断言用例
  AemeathPet.WinUI/        （保留）WinUI 3 前端骨架，不参与主构建
```

## 配置 AI 对话

1. 打开设置，来源选择 `openai` / `ollama` / `quick-welfare`；
2. 填写 endpoint 与模型；
3. API Key 写到 `%LOCALAPPDATA%\AemeathDeskPet\user\secrets\chat-api-key.txt`（不进 settings.json）；
4. 人格文本默认读 `assets/persona/default.txt`。

## 许可证

代码与素材沿用原仓库 `LICENSE-CODE` / `LICENSE-ASSETS`。第三方平台能力仅用于学习与个人测试。
