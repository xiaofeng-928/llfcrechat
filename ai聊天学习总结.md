# AI 对话功能代码总结

## 整体架构

```
┌──────────────┐    HTTP/HTTPS     ┌──────────────┐
│  llfcchat    │ ───────────────→  │  SenseNova   │
│  (Qt客户端)   │    LLM API调用    │  LLM 大模型   │
└──────┬───────┘                   └──────────────┘
       │ TCP (自定义协议)
       ▼
┌──────────────┐
│  ChatServer  │   ← 仅做消息持久化，不调用LLM
│  (C++服务端)   │
└──────┬───────┘
       │ SQL
       ▼
┌──────────────┐
│    MySQL     │
│ ai_conversation │
└──────────────┘
```

**核心设计**：客户端直接调用 LLM API，服务端只负责消息持久化。

---

## 1. 数据库层

**文件**：`sql/ai_conversation.sql`

每个用户一行，用 JSON 列存储完整对话：

```sql
CREATE TABLE IF NOT EXISTS ai_conversation (
    uid INT NOT NULL PRIMARY KEY,
    messages JSON NOT NULL,  -- JSON数组
    updated_time DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
);
-- messages 格式: [{"role":"user","content":"你好"}, {"role":"assistant","content":"你好！"}]
```

---

## 2. 协议定义

客户端 `llfcchat/global.h` 和服务端 `ChatServer/ChatServer/const.h` 定义了6个消息ID：

```cpp
MSG_AI_CHAT_REQ    = 1028   // 客户端→服务端：保存用户消息
MSG_AI_CHAT_RSP    = 1029   // 服务端→客户端：确认保存
MSG_AI_HISTORY_REQ = 1030   // 客户端→服务端：请求历史
MSG_AI_HISTORY_RSP = 1031   // 服务端→客户端：返回历史
MSG_AI_SAVE_REQ    = 1032   // 客户端→服务端：保存AI回复
MSG_AI_SAVE_RSP    = 1033   // 服务端→客户端：确认保存
```

---

## 3. 服务端代码

### 3.1 LogicSystem — 三个Handler

**文件**：`ChatServer/ChatServer/LogicSystem.cpp`

注册（构造函数中）：

```cpp
_fun_callbacks[MSG_AI_CHAT_REQ]   = std::bind(&LogicSystem::AiChatHandler, ...);
_fun_callbacks[MSG_AI_HISTORY_REQ] = std::bind(&LogicSystem::AiHistoryHandler, ...);
_fun_callbacks[MSG_AI_SAVE_REQ]   = std::bind(&LogicSystem::AiSaveHandler, ...);
```

| Handler | 功能 | 核心调用 |
|---------|------|---------|
| `AiChatHandler` | 保存用户消息 | `MysqlMgr::AppendAiMessage(uid, "user", content)` |
| `AiSaveHandler` | 保存AI回复 | `MysqlMgr::AppendAiMessage(uid, "assistant", content)` |
| `AiHistoryHandler` | 读取历史 | `MysqlMgr::GetAiMessages(uid)`，截取最后N条 |

### 3.2 MysqlDao — 数据库操作

**文件**：`ChatServer/ChatServer/MysqlDao.cpp`

两个关键方法：

- **`AppendAiMessage(uid, role, content)`** — 智能追加：
  - 行不存在 → `INSERT` 用 `JSON_ARRAY(JSON_OBJECT(...))` 创建
  - 行已存在 → `JSON_ARRAY_APPEND` 追加到现有 JSON 数组

```cpp
// 行不存在时
INSERT INTO ai_conversation (uid, messages)
VALUES (?, JSON_ARRAY(JSON_OBJECT('role', ?, 'content', ?)))

// 行已存在时
UPDATE ai_conversation
SET messages = JSON_ARRAY_APPEND(messages, '$', JSON_OBJECT('role', ?, 'content', ?))
WHERE uid = ?
```

- **`GetAiMessages(uid)`** — 读取并解析 `messages` JSON 列为 `Json::Value` 数组

### 3.3 MysqlMgr — 透传层

**文件**：`ChatServer/ChatServer/MysqlMgr.cpp`

直接转发到 `_dao`，无额外逻辑：

```cpp
void MysqlMgr::AppendAiMessage(int uid, const std::string& role, const std::string& content) {
    _dao.AppendAiMessage(uid, role, content);
}
Json::Value MysqlMgr::GetAiMessages(int uid) {
    return _dao.GetAiMessages(uid);
}
```

### 3.4 CSession — 消息ID校验

**文件**：`ChatServer/ChatServer/CSession.cpp`

将校验范围扩展到 `MSG_AI_SAVE_RSP`：

```cpp
if (msg_id < MSG_CHAT_LOGIN || msg_id > MSG_AI_SAVE_RSP) {
    // 非法消息ID
}
```

---

## 4. 客户端代码

### 4.1 配置读取

**文件**：`llfcchat/config.ini`

```ini
[LLM]
ApiKey = sk-chQYcado8vFO6dkCormyyygQYBqwp4nS
ApiUrl = https://token.sensenova.cn/v1/chat/completions
Model = sensenova-6.7-flash-lite
```

### 4.2 TcpMgr — 网络层

**文件**：`llfcchat/tcpmgr.cpp`

处理三个RSP消息，解析JSON后emit信号：

```cpp
// 收到 ID_AI_CHAT_RSP   → emit sig_ai_chat_rsp(rsp)
// 收到 ID_AI_SAVE_RSP   → emit sig_ai_save_rsp(rsp)
// 收到 ID_AI_HISTORY_RSP → emit sig_ai_history_rsp(messages数组)
```

三个信号定义在 `llfcchat/tcpmgr.h`：

```cpp
void sig_ai_chat_rsp(QJsonObject rsp);
void sig_ai_save_rsp(QJsonObject rsp);
void sig_ai_history_rsp(QJsonArray messages);
```

### 4.3 MainWindow — UI + AI核心逻辑（重点）

**文件**：`llfcchat/mainwindow.h` / `llfcchat/mainwindow.cpp`

#### 关键成员变量

```cpp
QPushButton* _ai_chat_btn;       // "AI对话"切换按钮
QWidget* _ai_panel;              // AI面板容器（右侧）
QListWidget* _ai_list;           // 消息列表
QTextEdit* _ai_input;            // 输入框
QPushButton* _ai_send_btn;       // 发送按钮
QPushButton* _ai_clear_btn;      // 清空按钮
QNetworkAccessManager* _net_mgr; // HTTP客户端（调用LLM）
bool _llm_busy;                  // 防并发锁
QList<LruEntry> _lru_messages;   // LRU上下文缓存
int _lru_max_size = 20;          // LRU最大条数
QString _pending_llm_type;       // "ai_chat" 或 "summary"
```

#### 信号连接

```cpp
connect(_ai_chat_btn, &QPushButton::clicked, this, &MainWindow::onAiChatToggle);
connect(_ai_send_btn, &QPushButton::clicked, this, &MainWindow::onAiSendClicked);
connect(_ai_clear_btn, &QPushButton::clicked, this, &MainWindow::onAiClearClicked);
connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_ai_chat_rsp, this, &MainWindow::onAiChatRsp);
connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_ai_save_rsp, this, &MainWindow::onAiSaveRsp);
connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_ai_history_rsp, this, &MainWindow::onAiHistoryRsp);
connect(_net_mgr, &QNetworkAccessManager::finished, this, &MainWindow::onLlmReplyFinished);
```

---

## 5. 完整对话流程

```
① 用户点击发送 → onAiSendClicked()
   ├─ 获取输入文本，显示用户气泡(AiUserBubble)
   ├─ lruAddMessage("user", content)     → LRU缓存
   ├─ 发送 ID_AI_CHAT_REQ 到服务端       → 持久化用户消息
   ├─ lruBuildMessages() 构建请求体       → system提示 + LRU历史
   ├─ QNetworkAccessManager POST LLM API
   ├─ 显示 "思考中..." 占位气泡
   └─ _llm_busy = true, 禁用按钮

② LLM回复到达 → onLlmReplyFinished(reply)
   ├─ 移除 "思考中..." 占位气泡
   ├─ 解析 choices[0].message.content
   ├─ 显示AI气泡(AiReplyBubble)
   ├─ lruAddMessage("assistant", content)
   └─ 发送 ID_AI_SAVE_REQ 到服务端       → 持久化AI回复

③ 用户打开AI面板 → onAiChatToggle()
   ├─ 调整窗口布局，显示 _ai_panel
   └─ 发送 ID_AI_HISTORY_REQ             → 加载历史记录

④ 历史记录返回 → onAiHistoryRsp(messages)
   ├─ 清空 _ai_list 和 _lru_messages
   └─ 遍历数组恢复气泡 + LRU缓存
```

---

## 6. LRU上下文缓存

```cpp
// 保留最近20条消息作为上下文发送给LLM
void lruAddMessage(role, content) {
    _lru_messages.append({role, content});
    if (_lru_messages.size() > _lru_max_size)
        _lru_messages.removeFirst();
}

QJsonArray lruBuildMessages() {
    // 先加系统提示
    systemMsg = {"role": "system", "content": "你是一个智能AI助手，请用中文回答问题。"};
    // 再加全部LRU历史
    for (auto& entry : _lru_messages) {
        arr.append({"role": entry.role, "content": entry.content});
    }
    return arr;
}
```

---

## 7. 两种气泡样式

| 气泡类 | 颜色 | 对齐 | 用途 | 特殊属性 |
|--------|------|------|------|---------|
| `AiUserBubble` | 绿色 | 右对齐 | 用户消息 | 无 |
| `AiReplyBubble` | 蓝色 | 左对齐 | AI回复 | `setProperty("is_ai_reply", true)` 用于定位和移除 |

---

## 8. 额外功能：聊天总结

"总结"按钮收集当前聊天记录，调用同一个LLM API，与AI对话共享 `_net_mgr`，通过 `_pending_llm_type` 区分：

| 场景 | `_pending_llm_type` | 系统提示 | 结果展示 |
|------|---------------------|---------|---------|
| AI对话 | `"ai_chat"` | "你是一个智能AI助手，请用中文回答问题。" | `AiReplyBubble` |
| 聊天总结 | `"summary"` | "你是一个聊天记录总结助手。请用简洁的中文总结以下聊天记录的要点。" | `SummaryBubble` |

---

## 9. LLM API 请求格式

```json
{
    "model": "sensenova-6.7-flash-lite",
    "messages": [
        {"role": "system", "content": "你是一个智能AI助手，请用中文回答问题。"},
        {"role": "user", "content": "第一轮问题"},
        {"role": "assistant", "content": "第一轮回复"},
        {"role": "user", "content": "当前问题"}
    ],
    "stream": false
}
```

请求头：

```
Content-Type: application/json
Authorization: Bearer sk-chQYcado8vFO6dkCormyyygQYBqwp4nS
```

响应解析：

```cpp
// reply JSON 结构
{
    "choices": [{
        "message": {
            "role": "assistant",
            "content": "AI的回复内容"
        }
    }]
}
// 解析代码
QString content = doc["choices"][0]["message"]["content"].toString();
```

---

## 10. 技术要点总结

| 技术点 | 本项目实践 |
|--------|-----------|
| **LLM调用** | 客户端直接HTTP POST，Bearer Auth，OpenAI兼容格式 |
| **上下文管理** | LRU缓存最近20条，每次请求带上完整历史 |
| **消息持久化** | MySQL JSON列存储，`JSON_ARRAY_APPEND` 增量追加 |
| **流式 vs 非流式** | 使用 `stream: false`，一次返回完整回复 |
| **并发控制** | `_llm_busy` 锁防止重复请求 |
| **UI反馈** | "思考中..."占位气泡，请求完成后替换 |
| **架构选择** | 客户端调LLM → 灵活但暴露ApiKey；服务端调LLM → 安全但增加延迟 |

---

## 11. 涉及文件清单

| 层级 | 文件路径 | AI相关改动 |
|------|---------|-----------|
| 数据库 | `sql/ai_conversation.sql` | 新增表 |
| 服务端协议 | `ChatServer/ChatServer/const.h` | 新增6个消息ID |
| 服务端逻辑 | `ChatServer/ChatServer/LogicSystem.h` | 新增3个Handler声明 |
| 服务端逻辑 | `ChatServer/ChatServer/LogicSystem.cpp` | 注册+实现3个Handler |
| 服务端数据库 | `ChatServer/ChatServer/MysqlDao.h` | 新增2个方法声明 |
| 服务端数据库 | `ChatServer/ChatServer/MysqlDao.cpp` | 实现AppendAiMessage、GetAiMessages |
| 服务端管理 | `ChatServer/ChatServer/MysqlMgr.h` | 新增2个方法声明 |
| 服务端管理 | `ChatServer/ChatServer/MysqlMgr.cpp` | 透传到Dao |
| 服务端会话 | `ChatServer/ChatServer/CSession.cpp` | 扩展消息ID校验范围 |
| 服务端配置 | `ChatServer/ChatServer/config.ini` | 新增[LLM]配置段 |
| 客户端协议 | `llfcchat/global.h` | 新增6个消息ID |
| 客户端网络 | `llfcchat/tcpmgr.h` | 新增3个信号 |
| 客户端网络 | `llfcchat/tcpmgr.cpp` | 处理3个RSP消息 |
| 客户端UI | `llfcchat/mainwindow.h` | 新增成员变量和方法声明 |
| 客户端UI | `llfcchat/mainwindow.cpp` | 实现全部AI逻辑 |
| 客户端配置 | `llfcchat/config.ini` | 新增[LLM]配置段 |
| 客户端构建 | `llfcchat/llfcchat.pro` | 新增network模块依赖 |
