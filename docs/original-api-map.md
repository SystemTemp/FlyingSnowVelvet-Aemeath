# 原版 API 适配清单

## 1. 聊天接口

### OpenAI 兼容

- 端点：`{base}/chat/completions` 或 `{base}/v1/chat/completions`
- 模型列表：`{base}/models`
- Header：`Authorization: Bearer <api_key>`，`Content-Type: application/json`
- Body：`model`、`messages`、`stream`、`temperature`、可选 `max_tokens`、`tools`、`reasoning`
- C++ 落点：`IChatService`、后续 `OpenAICompatibleChatService`

### Ollama

- 默认：`http://localhost:11434/api/chat`
- 兼容：`http://localhost:11434/api/generate`
- Body：`model`、`messages` 或 `prompt`、`stream:true`、`options`
- C++ 落点：后续 `OllamaChatService`

### 福利 API

- Kilo：`https://ai-gateway.kilo.ai/api/v1`
- 备用 token：
  - `https://github.com/MARK42IRPC/FlyingSnowVelvet-Aemeath/releases/download/RESC/free-token.ini`
  - `https://gitee.com/Mark42IRPC/Aemeath-AIdeskpet/releases/download/RESC/free-token.ini`
- C++ 落点：后续 `QuickWelfareChatService`

## 2. 论坛/留言墙 API

- 基地址：`https://forum.fxxr.store/api`
- User-Agent：`FlyingSnowVelvet-Community/1.0`
- 响应：`{ ok, data, error }`
- 鉴权：`Authorization: Bearer <token>`

### 账号

| 接口 | 方法 | 参数 |
|---|---|---|
| `/auth/register` | POST | `username,password,display_name?` |
| `/auth/login` | POST | `username,password` |
| `/auth/logout` | POST | Bearer |
| `/auth/me` | GET | Bearer |

### 帖子/留言

| 接口 | 方法 | 参数 |
|---|---|---|
| `/posts` | GET | `page,per_page,sort,tag?,author?,q?` |
| `/posts/{id}` | GET | `id` |
| `/posts` | POST | `title,content,tags?,images?` |
| `/posts/{id}/replies` | GET | `page,per_page` |
| `/posts/{id}/replies` | POST | `content,parent_id?,images?` |
| `/posts/{id}/like` | POST/DELETE | 点赞/取消 |
| `/replies/{id}/like` | POST/DELETE | 点赞/取消 |
| `/images` | POST | `{data}` base64 或 data URL |
| `/images/{id}` | GET | 图片二进制 |
| `/tags` | GET | `limit` |

C++ 当前已建立：

- `ForumModels.h`
- `IForumService.h`
- `ForumApiClient.h/.cpp`
- `WallModels.h`
- `IWallService.h`
- `WallPage.xaml`

其中 `ForumApiClient` 已按原版路径、方法、header、token 规则搭好请求骨架；JSON 解析待 Windows 端引入 JSON 库后补齐。

## 3. 留言墙客户端边界

本客户端只实现普通用户接口，不包含社区管理接口、管理入口或管理模型。

## 4. 音乐接口

### QQ 音乐

- 搜索/详情/VKey：`https://u.y.qq.com/cgi-bin/musicu.fcg`
- 登录相关：QQ ptlogin 与 y.qq.com Cookie
- 建议 C++ 用 WebView2 登录，不建议完全手写扫码流程。

### 网易云

- 原项目用 `pyncm` 的 EAPI/WEAPI 加密接口。
- C++ 可移植加密逻辑或先通过辅助进程桥接。

### B 站直播点歌

- 房间解析：`https://api.live.bilibili.com/room/v1/Room/room_init`
- 弹幕 token：`https://api.live.bilibili.com/xlive/web-room/v1/index/getDanmuInfo`
- WebSocket：`wss://broadcastlv.chat.bilibili.com/sub`

## 5. 更新/语音包

- 公告：Gitee Release 文本
- 更新包：Hugging Face / ModelScope 双源
- 语音包：ModelScope / Hugging Face ONNX 语音包
- 需要 SHA-256 校验、断点临时文件、双源回退。
