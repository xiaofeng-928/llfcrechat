# OpenXX (llfcchat) 代码审查报告

**审查日期**: 2026-05-13
**审查范围**: GateServer、ChatServer、llfcchat 全部源码
**当前版本**: v1.0.0 (commit 78478e7)

---

## 目录

- [一、必须修复（面试官会追问）](#一必须修复面试官会追问)
- [二、建议修复（展示代码质量）](#二建议修复展示代码质量)

---

## 一、必须修复（面试官会追问）

### 1.1 ChatServer 登录不验证密码

**文件**: `ChatServer/LogicSystem.cpp:88-113`

`LoginHandler` 仅根据 UID 查询用户是否存在，**完全不检查密码**：

```cpp
auto user_info = MysqlMgr::GetInstance()->GetUser(uid);
if (user_info == nullptr) {
    rtvalue["error"] = ErrorCodes::UidInvalid;
    return;
}
rtvalue["error"] = ErrorCodes::Success;  // 只要 UID 存在就登录成功
```

**风险**: 知道任意 UID 即可登录该用户，认证完全失效。后续需加 token 机制解决。

---

### 1.2 ChatServer — 竞态条件：CServer::ClearSession 无锁访问 map

**文件**: `ChatServer/CServer.cpp:35-46`

```cpp
void CServer::ClearSession(std::string uuid) {
    if (_sessions.find(uuid) != _sessions.end()) {       // 无锁读
        UserMgr::GetInstance()->RmvUserSession(_sessions[uuid]->GetUserId()); // 无锁访问
    }
    {
        lock_guard<mutex> lock(_mutex);
        _sessions.erase(uuid);                            // 有锁删除
    }
}
```

`find` 和 `operator[]` 在锁外执行，另一个线程可能在之间删除该条目，导致 TOCTOU 问题和潜在的崩溃。应将整个操作放入锁的保护范围内。

---

### 1.3 两服务器 — 竞态条件：AsioIOServicePool::GetIOService()

**文件**: `ChatServer/AsioIOServicePool.cpp:22-28`, `GateServer/AsioIOServicePool.cpp:23-28`

```cpp
auto& service = _ioServices[_nextIOService++];
if (_nextIOService == _ioServices.size()) {
    _nextIOService = 0;
}
```

`_nextIOService` 是普通 `size_t`，无同步保护。多线程并发调用时 `++` 和重置操作可能交错执行，导致越界访问或总是返回同一个 io_context。应改为 `std::atomic<size_t>` 配合 `fetch_add` + 取模。

---

### 1.4 ChatServer — SendNode 堆缓冲区溢出

**文件**: `ChatServer/MsgNode.cpp:8-17`

```cpp
SendNode::SendNode(const char* msg, short max_len, short msg_id)
    :MsgNode(max_len + HEAD_TOTAL_LEN)  // short 溢出风险
```

`max_len` 为 `short`（有符号 16 位，最大 32767），`max_len + HEAD_TOTAL_LEN` 可能溢出导致分配过小的缓冲区，而后续 `memcpy` 写入原始长度的数据，造成堆溢出。

**关联问题** — `CSession.cpp:50`：
```cpp
_send_que.push(make_shared<SendNode>(msg.c_str(), msg.length(), msgid));
```
`msg.length()` 返回 `size_t`，隐式窄化为 `short`，长消息将触发上述溢出。

---

### 1.5 ChatServer — 消息 ID 验证逻辑错误

**文件**: `ChatServer/CSession.cpp:150`

```cpp
if (msg_id > MAX_LENGTH) {  // MAX_LENGTH=2048 是字节大小，不是消息 ID 范围
```

将消息类型 ID（1005-1023）与缓冲区大小常量（2048）比较，语义完全错误。恰好"能用"只是因为当前 ID 值都小于 2048，但这是错误的逻辑。应验证 msg_id 是否在合法的消息类型范围内。

---

### 1.6 GateServer — CheckPwd 尾部悬挂 return true（潜在认证绕过）

**文件**: `GateServer/MysqlDao.cpp:123`

```cpp
    catch (sql::SQLException& e) {
        return false;
    }
    return true;   // ← 当前不可达，但如果重构移除内部 return false，将导致认证绕过
```

此行是"定时炸弹"——当前因所有内部路径都提前 return 而不可达，但一旦有人重构时不小心删掉内部的某个 `return false`，函数就会对任意输入返回 `true`（认证通过）。应改为 `return false;` 或直接删除。

---

### 1.7 客户端 — HttpMgr 错误分发逻辑 Bug

**文件**: `llfcchat/httpmgr.cpp:72-76`

```cpp
// 任何 HTTP 错误都同时触发四个信号
emit sig_reg_mod_finish(reqId, ...);
emit sig_reset_mod_finish(reqId, ...);
emit sig_login_mod_finish(reqId, ...);
emit sig_get_varifycode_finish(reqId, ...);
```

`mod` 变量（第 64 行已获取）未用于分发，导致任何 HTTP 错误同时通知所有四个模块。比如登录失败也会触发注册/重置/验证码的错误回调，可能引起 UI 状态混乱。应按 `mod` 值只 emit 对应的信号。

---

### 1.8 两服务器 — ConfigMgr::operator= 缺少 return 语句（未定义行为）

**文件**: `ChatServer/ConfigMgr.h:60-66`, `GateServer/ConfigMgr.h:60-66`

```cpp
ConfigMgr& operator=(const ConfigMgr& src) {
    if (&src == this) {
        return *this;
    }
    this->_config_map = src._config_map;
};  // ← 非 void 函数无 return，未定义行为
```

C++ 标准规定非 void 函数必须有返回值，此处缺少 `return *this;`，属于未定义行为。

---

### 1.9 客户端 — QDataStream 与手动缓冲区操作冲突

**文件**: `llfcchat/tcpmgr.cpp:18-53`

```cpp
QDataStream stream(&_buffer, QIODevice::ReadOnly);
// ...从 stream 读取 header...
_buffer = _buffer.mid(sizeof(quint16)*2);  // 替换了底层 QByteArray
// forever 循环下一轮迭代时，stream 的内部状态与 _buffer 不同步
```

`QDataStream` 持有对 `QByteArray` 的引用，`mid()` 替换 `_buffer` 后流的状态失效。在 `forever` 循环中第二次迭代时，stream 读取的数据可能不正确。应在每次循环迭代时重新创建 `QDataStream`。

---

## 二、建议修复（展示代码质量）

### 2.1 两服务器 — 分离线程无法安全关闭 (Use-After-Free)

**文件**:
- `ChatServer/MysqlDao.h:43` — `_check_thread.detach()`
- `GateServer/MysqlDao.h:33` — `_check_thread.detach()`
- `ChatServer/LogicSystem.cpp:22` — `clean_thread.detach()`

分离线程在对象析构后仍可能运行，访问已销毁的成员变量。析构函数未调用 `Close()`，也未 `join` 线程。应改为可 join 的线程，在析构中设置停止标志并等待线程退出。

---

### 2.2 客户端 — MainWindow 内存泄漏

**文件**: `llfcchat/mainwindow.cpp:62-63`

```cpp
void MainWindow::SlotSwitchLogin() {
    _login_dlg = new LoginDialog(this);  // 每次切换创建新的，旧对象未删除
```

每次从注册切换回登录，都会创建新 `LoginDialog` 而不删除旧的，造成内存泄漏。同样的问题存在于 `SlotSwitchReg()`。应在创建新对象前 `delete` 旧的，或复用现有对象。

---

### 2.3 客户端 — _simplified_dlg 无父对象，永不释放

**文件**: `llfcchat/mainwindow.cpp:76`

```cpp
_simplified_dlg = new QWidget();  // 无 parent，MainWindow 析构时不会自动删除
```

应传入 `this` 作为 parent，或在析构中手动 delete。

---

### 2.4 客户端 — _handlers[id] 无边界检查导致崩溃

**文件**: `llfcchat/logindialog.cpp:197`, `registerdialog.cpp:76`, `resetdialog.cpp:89`

```cpp
_handlers[id](jsonDoc.object());  // 若 id 不在 map 中，调用空的 std::function → 抛出 std::bad_function_call
```

应先检查 `if (_handlers.find(id) != _handlers.end())` 再调用，或使用 `at()` 配合 try-catch。

---

### 2.5 ChatServer — JSON 解析无错误检查

**文件**: `ChatServer/LogicSystem.cpp:89-92, 131-134`

```cpp
Json::Reader reader;
Json::Value root;
reader.parse(msg_data, root);  // 返回值未检查
auto uid = root["uid"].asInt();  // 畸形 JSON 时 asInt() 抛出未捕获异常
```

`parse()` 返回值未检查；若 JSON 畸形，`asInt()` 在 null Value 上抛出异常且未被捕获，会导致消息处理线程崩溃。应检查 `parse()` 返回值，并对字段访问做 `isMember()` / `isInt()` 验证。

---

### 2.6 ChatServer — MsgNode 违反 Rule of Three/Five

**文件**: `ChatServer/MsgNode.h:9-30`

`MsgNode` 用 `new char[]` 分配 `_data`，析构函数用 `delete[]` 释放，但没有拷贝构造函数和拷贝赋值运算符。若意外拷贝，会导致 double-free。应添加拷贝操作 `= delete`。

---

### 2.7 ChatServer — 重复登录静默替换会话

**文件**: `ChatServer/LogicSystem.cpp:111-112`

```cpp
session->SetUserId(uid);
UserMgr::GetInstance()->SetUserSession(uid, session);
```

同一用户重复登录时，`SetUserSession` 替换旧会话，但旧 `CSession` 仍存在于 `CServer::_sessions` 中，成为孤立连接。旧客户端无任何通知，连接保持但不再能收发消息。应在替换前主动关闭旧连接并通知旧客户端。

---

### 2.8 ChatServer — 无界消息队列

**文件**: `ChatServer/LogicSystem.cpp:31-37`

`PostMsgToQue` 无限制地向 `_msg_que` 推入消息。若消息到达速度超过处理速度，队列无限增长，耗尽内存。应添加队列上限，超出时丢弃或背压。

---

### 2.9 两服务器 — MySQL 连接池长时间持锁

**文件**: `ChatServer/MysqlDao.h:50-79`, `GateServer/MysqlDao.h:41-69`

`checkConnection()` 在整个检查周期内持有 `mutex_`，包括可能耗时数秒的重连操作，阻塞所有 `getConnection()`/`returnConnection()` 调用。应缩小锁的粒度——仅在操作队列时持锁，重连时不持锁。

---

### 2.10 客户端 — 无自动重连和连接超时

**文件**: `llfcchat/tcpmgr.cpp:58-64, 438-446`

- `errorOccurred` 和 `disconnected` 仅打印日志，无重连尝试、无 UI 通知
- `connectToHost` 无超时设置，服务器不可达时无限挂起

应添加连接超时和断线重连机制，并在 UI 层提示用户连接状态。

---

### 2.11 客户端 — 无心跳/保活机制

TCP 连接无 ping/pong 心跳。NAT 超时或中间设备丢弃连接后双方均无法感知，用户会话"假活"。应添加定期心跳包。

---

### 2.12 两服务器 — ConfigMgr 构造函数未处理配置文件缺失

**文件**: `ChatServer/ConfigMgr.cpp:4-6`, `GateServer/ConfigMgr.cpp:10`

`read_ini()` 在文件缺失时抛出异常但未捕获，服务器直接崩溃且无有意义的错误信息。应在构造函数中 try-catch 并给出清晰的错误提示。

---

### 2.13 两服务器 — CServer::Start 异常时无限重试无延迟

**文件**: `ChatServer/CServer.cpp:28-31`, `GateServer/CServer.cpp:28-31`

```cpp
catch (std::exception& exp) {
    self->Start();  // 无延迟递归重试，可能消耗 CPU
}
```

若出现持续性错误（如文件描述符耗尽），此递归会无延迟地无限重试，消耗 CPU。应添加退避延迟和最大重试次数。

---

### 2.14 GateServer — POST 路由键包含查询参数导致 404

**文件**: `GateServer/HttpConnection.cpp:156`

```cpp
_request.target()  // 直接用作路由键，未剥离查询参数
```

若客户端发送 `POST /user_login?foo=bar`，因键不匹配而返回 404。应在路由查找前剥离查询参数。

---

### 2.15 GateServer — 无输入验证

**文件**: `GateServer/LogicSystem.cpp:24-26, 70-71`

```cpp
auto name = src_root["user"].asString();
auto pwd = src_root["passwd"].asString();
```

未验证字段长度、字符集、格式。空用户名/密码、超长输入、控制字符均可通过注册。

---

### 2.16 客户端 — enable_shared_from_this + QObject 冲突且未使用

**文件**: `httpmgr.h:12`, `tcpmgr.h:12`, `usermgr.h:9`

三个类同时继承 `QObject` 和 `std::enable_shared_from_this<T>`。`shared_ptr` 引用计数与 Qt 父子对象内存模型冲突，若 Qt 父对象删除子对象时 `shared_ptr` 仍持有引用，将导致双重释放。且 `shared_from_this()` 从未被调用。应移除 `enable_shared_from_this` 继承。

---

### 2.17 客户端 — 复制粘贴错误日志

**文件**: `llfcchat/tcpmgr.cpp:144, 153, 179, 187`

搜索用户和好友申请的错误日志显示 `"Login Failed"` 而非正确的操作名称。属于明显的复制粘贴遗留问题。

---

### 2.18 两服务器 — atoi 无错误检查

**文件**: `ChatServer/ChatServer.cpp:33`, `GateServer/GateServer.cpp:28`

```cpp
unsigned short gate_port = atoi(gate_port_str.c_str());  // 失败返回 0，绑定随机端口
```

应使用 `std::stoi` 配合异常处理。

---

### 2.19 客户端 — UserMgr 方法无空指针检查

**文件**: `llfcchat/usermgr.cpp:19-31`

```cpp
QString UserMgr::GetUid() { return _user_info->_uid; }  // _user_info 可能为 nullptr
```

`_user_info` 初始化为 `nullptr`，在 `SetUserInfo()` 调用前访问任何方法将崩溃。应添加空指针检查。

---

### 2.20 客户端 — 大量重复代码

**文件**: `llfcchat/tcpmgr.cpp:74-425`

约 9 个消息处理函数遵循完全相同的 JSON 解析 + 错误检查模式：

```cpp
QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
if(jsonDoc.isNull()){
    qDebug() << "Failed to create QJsonDocument.";
    return;
}
QJsonObject jsonObj = jsonDoc.object();
if(jsonObj.contains("error")){
    int error = jsonObj["error"].toInt();
    // ...各 handler 特有逻辑...
}
```

应提取为 `parseJsonAndCheckError(QByteArray data)` 辅助函数，消除数百行重复代码。

---

### 2.21 命名不一致

| 问题 | 示例 |
|------|------|
| 类名前缀不统一 | `CServer` vs `LogicSystem` |
| SQL 大小写不统一 | `MysqlDao` vs `MySqlPool` |
| 消息 ID 前缀不统一 | `MSG_CHAT_LOGIN` vs `ID_SEARCH_USER_REQ` |
| 成员命名风格混合 | `_sessions` vs `deadline_` vs `b_stop_` |
| 拼写错误 | `VARIFY` → 应为 `VERIFY`，`swich` → 应为 `switch` |

面试时被问到命名规范会扣分。

---

### 2.22 头文件 using namespace std 污染

**文件**: `Singleton.h`, `CServer.h`, `CSession.h`, `MsgNode.h` 等多个头文件

在头文件中使用 `using namespace std;` 会影响所有包含这些头文件的翻译单元，可能导致符号冲突。应在头文件中使用完整的 `std::` 限定。

---

### 2.23 ChatServer — 宏常量应改为 constexpr

**文件**: `ChatServer/const.h:21-26`

`MAX_LENGTH`、`HEAD_TOTAL_LEN` 等使用 `#define` 定义。应使用 `constexpr` 替代，有类型安全、作用域控制和调试友好等优势。

---

### 2.24 GateServer — MIME 类型错误

**文件**: `GateServer/LogicSystem.cpp:11`

使用 `"text/json"` 而非标准 MIME 类型 `"application/json"`。部分 HTTP 客户端可能无法正确解析。

---

### 2.25 两服务器 — 使用已弃用的 Json::Reader

**文件**: `ChatServer/LogicSystem.cpp:89`, `GateServer/LogicSystem.cpp` (同)

`Json::Reader` 在新版 jsoncpp 中已弃用，应改用 `Json::CharReaderBuilder`。

---

### 2.26 两服务器 — 无优雅关闭序列

信号处理仅停止 `io_context`，未等待 AsioIOServicePool 线程、MySQL 检查线程、LogicSystem 工作线程正常退出。应实现有序的关闭流程。

---

## 修复优先级总表

### P0 — 必须修复（逻辑 Bug / UB / 崩溃风险）

| # | 问题 | 文件 |
|---|------|------|
| 1 | ChatServer 登录不验证密码 | ChatServer/LogicSystem.cpp |
| 2 | ClearSession 竞态条件 | ChatServer/CServer.cpp |
| 3 | GetIOService 竞态条件 | 两服务器 AsioIOServicePool.cpp |
| 4 | SendNode 堆缓冲区溢出 | ChatServer/MsgNode.cpp, CSession.cpp |
| 5 | 消息 ID 验证逻辑错误 | ChatServer/CSession.cpp |
| 6 | CheckPwd 悬挂 return true | GateServer/MysqlDao.cpp |
| 7 | HttpMgr 错误分发 Bug | llfcchat/httpmgr.cpp |
| 8 | operator= 缺少 return (UB) | 两服务器 ConfigMgr.h |
| 9 | QDataStream 缓冲区操作冲突 | llfcchat/tcpmgr.cpp |

### P1 — 建议修复（展示工程素养）

| # | 问题 | 文件 |
|---|------|------|
| 10 | 分离线程无法安全关闭 | 两服务器 MysqlDao.h, LogicSystem.cpp |
| 11 | MainWindow 内存泄漏 | llfcchat/mainwindow.cpp |
| 12 | _simplified_dlg 无父对象 | llfcchat/mainwindow.cpp |
| 13 | _handlers[id] 无边界检查 | llfcchat 三个对话框文件 |
| 14 | JSON 解析无错误检查 | ChatServer/LogicSystem.cpp |
| 15 | MsgNode 违反 Rule of Five | ChatServer/MsgNode.h |
| 16 | 重复登录静默替换会话 | ChatServer/LogicSystem.cpp |
| 17 | 无界消息队列 | ChatServer/LogicSystem.cpp |
| 18 | MySQL 连接池长时间持锁 | 两服务器 MysqlDao.h |
| 19 | 无重连/心跳/超时 | llfcchat/tcpmgr.cpp |
| 20 | ConfigMgr 未处理配置缺失 | 两服务器 ConfigMgr.cpp |
| 21 | CServer 无限重试无延迟 | 两服务器 CServer.cpp |
| 22 | POST 路由键含查询参数 | GateServer/HttpConnection.cpp |
| 23 | enable_shared_from_this 未使用 | llfcchat 三个管理类 |
| 24 | 命名不一致 / 拼写错误 | 全局 |
| 25 | using namespace std 污染头文件 | 多个头文件 |

> **总结**: 项目最核心的问题是 **ChatServer 登录无密码验证** 和 **多处竞态条件**——这些是面试中一定会被追问的逻辑缺陷。其次是 **未定义行为**（operator= 缺 return、SendNode 溢出）和 **客户端 Bug**（HttpMgr 错误分发、QDataStream 缓冲区冲突、内存泄漏）。修复这些问题能显著提升项目的工程质量印象。
