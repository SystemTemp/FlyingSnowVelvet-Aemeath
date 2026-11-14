# 与原 Python 版的差距对照

数据来自对 `MARK42IRPC/FlyingSnowVelvet-Aemeath`（`main` @ 87cfedbe，2026-11-13）的实际统计，
以及本仓库当前代码。原版完整历史已保留在 `python-legacy` 分支。

## 0. 量级

| 维度 | Python 原版 | 本仓库 v0.1.0 |
|---|---|---|
| 源文件 | 939 个 `.py`（`lib/` 617 / `legacy_tests/` 263 / `config/` 28 / `install_deps/` 11） | 约 70 个 C++ 文件 |
| 源码体积 | 约 8 MB（`lib/` 约 5.2 MB） | 约 300 KB |
| 资源 | `resc/` 2208 个文件 / 95 MB（GIF 动画、字体、音效、Vosk 模型） | 22 张脚本生成的 PNG（约 200 KB） |
| 文档 | `doc/` 20 个文件 / 0.7 MB（维护手册 136 KB、render 契约 299 KB、DX 后端方案 41 KB…） | 5 个 md |
| 测试 | `legacy_tests` 263 个 + `tests-ng` 9 个 | 39 个用例 |
| 安装 | 原生 C 安装器（`main.c` 115 KB / 卸载 63 KB / 更新 37 KB / 解压 74 KB） | PowerShell 在线安装器 + ZIP |

## 1. 渲染与表现层

| 原版能力 | 本版 | 备注 |
|---|---|---|
| 图层系统（`render/layers`：draws/hosts/order/spec/windows） | ❌ | 只有单窗口单贴图 |
| 粒子系统（`script/practical/` 22 个粒子脚本） | ❌ | 雪花、雪球爆裂、音符、星轨、碰撞… |
| 特效系统（`script/effects/` 5 个） | ❌ | flash_text、smooth_image_show |
| SEanima 动画格式（`script/SEanima/` 5 个：decoder/clip/animation/effects） | ❌ | 原版自有动画容器 |
| 物理（`core/physics.py`、`world_object_physics`） | ❌ | |
| 世界对象（音响/雪球/雪堆/雪豹/沙发/摩托/闹钟） | ❌ | |
| DirectX 原生后端（`native/dx_backend`，C ABI v10，D2D 命令流/渐变/圆角/路径/IME/托盘） | ❌ | 本版是简化的 D2D 贴图；原版后端可直接复用 |
| 屏幕捕获、DPI 多屏、离屏读回 | ❌ | |
| 音频频谱/电平表（`audio_meter`/`audio_spectrum`/`speaker_band`） | ❌ | |

## 2. AI 对话

| 原版能力 | 本版 |
|---|---|
| 快捷福利 API：Kilo 免费池清单缓存 6h、一次最多换 3 个模型、单模型 20s 超时、失败冷却 10~30min、带图优先多模态 | ⚠️ 只有端点和默认模型，无轮询/冷却策略 |
| 备用福利 API：GitHub/Gitee 双源并发取密钥、10s 超时 + 3 次重试、Agnes 2.0/2.5 切换 | ❌ |
| 手动 OpenAI 兼容：流式、上下文、人格、图片输入、多 payload 变体、DashScope 特判 | ⚠️ 流式 + 上下文 + 人格；无图片/多变体 |
| Ollama：自动引导、registry、session、state、`/models` 探测 | ⚠️ 只有 `/api/chat` 流式 |
| 规则回复兜底 | ❌ |
| 上下文记忆持久化（`chat/memory.py`） | ⚠️ 仅进程内 24 条 |
| 自动陪伴（`handler_auto_companion`） | ❌ |
| 工具调用（`native_tools` + `tool_dispatcher`） | ❌ |
| 视觉截图编码（`vision_capture`/`vision_codec`） | ❌ |
| 网络策略（`network_policy`） | ❌ |

## 3. 语音

| 原版能力 | 本版 |
|---|---|
| ONNX 语音包安装（UnRAR、包管理、旧 GSVmove 迁移） | ❌ |
| 本地 TTS 推理（`gsvmove/onnx_runtime`、`native_graph`、`hybrid_worker`） | ❌ |
| 自研 CUDA 推理端（`native/cuda_voice_runtime`，38 个文件） | ❌ |
| DirectML / CUDA overlay 安装 | ❌ |
| Vosk 本地识别 + Push-to-Talk + PCM16 降噪 + 噪声门 | ❌ |
| 台词事件系统（`script/voice/` 24 个 `ams_*` 触发器） | ❌ |
| Windows SAPI TTS | ⚠️ 接口已定，实现未接入构建 |

## 4. 音乐

| 原版能力 | 本版 |
|---|---|
| QQ / 网易云 / 酷狗 三 provider | ❌ |
| `pyncm` EAPI/WEAPI 加密（21 个文件） | ❌ |
| cloudmusic 播放运行时（11 个文件） | ❌ |
| B 站直播点歌（room_init + 弹幕 WebSocket） | ❌ |
| 礼物告警、歌单/队列/进度/音量/历史 | ❌ |
| 本地播放 | ⚠️ 接口已定（MCI 实现未接入构建） |

## 5. 社区与办公

| 原版能力 | 本版 |
|---|---|
| 论坛/留言墙 API（10 端点） | ✅ 完整实现并测试 |
| 论坛 UI（40+ 个 `forum_*`：板块、编辑器、详情、Markdown、贴纸、配色、图片墙、缓存、会话） | ❌ |
| 办公 DSH 侧车（`script/office/` 13 个 + `services/dsh-office-runtime`） | ❌ |
| 游戏包（拉海俄罗斯方块：manifest + code + extensions + assets） | ❌ |
| 游戏模式/管理器/音效 | ❌ |

## 6. 系统集成

| 原版能力 | 本版 |
|---|---|
| 托盘图标与菜单 | ✅ |
| 点击穿透 | ✅ |
| 置顶切换 | ✅ |
| 单实例 | ✅ |
| 开机自启（`app/autostart.py`） | ❌ |
| 桌面快捷方式（安装器） | ✅ |
| 崩溃转储、bug tracker 窗口 | ❌ |
| 状态保存（位置/缩放/置顶/穿透） | ✅ |
| 自动更新（双源探测 + 外层 ZIP 内安装器校验） | ❌（有在线安装器，无更新器） |
| 原生安装器/卸载器/更新器（C，LZMA2 分片、进度 UI） | ❌（PowerShell + ZIP） |

## 7. 建议的追赶顺序

1. **更新器 + 原生安装器**：把 PowerShell 安装器换成 C 版本，并实现"检查更新 → 下载 → 校验 → 替换"。这是一次性工程，收益直接。
2. **语音（SAPI → ONNX）**：先接 Windows SAPI 让语音可用，再移植 `gsvmove` 的 ONNX 推理与语音包安装。
3. **音乐 provider**：QQ/网易云/酷狗 是纯 HTTP + 加密，不涉及 GUI，适合并行推进。
4. **渲染层升级**：把 `PetRenderer` 换成命令流 + 图层，直接托管原版 `native/dx_backend`（C ABI），一步拿到渐变/圆角/路径/多窗口/托盘/IME。
5. **AI 车道补全**：福利池轮询与冷却、备用双源、多模态、工具调用。
6. **论坛 UI / 办公侧车 / 游戏包**：体量最大，放在最后。
