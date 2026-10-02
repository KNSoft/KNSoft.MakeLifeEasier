# Chromium ABE（应用程序绑定加密）解密的工程化落地实践

Chromium 应用程序绑定加密（App-Bound Encryption，以下简称ABE）的解密已不是新鲜事，本文将侧重于讨论其在工程上的落地实践，以及分享过程中 AI & Security 的体会。

最终落地一个**无需 UAC 提权**、**无视浏览器独占锁**，读取并解密浏览器数据的可工程化方案，以及对 **Edge/ChatGPT App 的 Chrome 数据导入功能实现分析**。

[PoC: AbeDecrypt Sample](https://github.com/KNSoft/KNSoft.MakeLifeEasier/tree/main/Source/Samples/AbeDecrypt): 本文 PoC 示例，演示本文例举的 4 种解密方式，获得 Chrome/Edge 里所选 User Data 的 v10/v20 Key，并显示所选 Profile 解密出来的 Cookie 及密码数据：

![](Assets/AbeDecrypt.png)

[KNSoft.ZPigeon](https://github.com/KNSoft/KNSoft.ZPigeon): 支持读取 Chrome/Edge 浏览器各 Profiles 下的各类数据（含 Cookie、密码）及启动可远程控制的无头浏览器实例：

![](Assets/ZPigeon-Cookie.png)

## 背景

最近在做 [KNSoft.ZPigeon](https://github.com/KNSoft/KNSoft.ZPigeon)，一个 AI 加持的 Windows 远程管理平台。之所以想做，是因为我一直相信鸽子的最终形态会是功能极其强大的企业 IT 管理系统，而今在企业 IT、终端安全、Windows 底层研发与 AI 四个领域的积淀够让我离这个目标更进一步了。

其中功能自然包含浏览器数据的获取，ABE 是最后一关。PoC 虽多，但只是证明“方案可行”，真正落地还要兼顾易用性和效率，并处理多 Profile、浏览器运行中的独占锁、权限要求等实际问题，这也是本文重点。

## Chromium 如何加密与保存数据？

相关资料很多，[Chromium](https://github.com/chromium/chromium) 本身也开源，所以实现部分以 Chromium/Chrome/Edge 为基线快速梳理机制；关键机制直接说结论，不再大段摘录代码，未开源部分（浏览器私有实现）再额外说明。

### 数据存放

系统预装 Edge 数据存放于 `%LOCALAPPDATA%\Microsoft\Edge\User Data` 目录，Chrome 类似：

- Local State：JSON，存放密钥（非明文）。DPAPI、ABE 的 Key 都在这里
- Default：默认 Profile 目录，同级目录还可能有 Profile 1、Profile 2 等
  - Network\Cookies：SQLite，cookies 表（Cookie，加密）
  - Login Data：SQLite，logins 表（密码，加密）
  - Login Data For Account: SQLite, 同 `Login Data`，但这包含登录浏览器账号存储的密码
  - History：SQLite，历史记录（明文）
  - ...

加密内容如 `Cookies` 库 `cookies` 表的 `encrypted_value` 列、`Login Data` 库 `logins` 表的 `password_value` 列，以 Blob 形式存储：

```
 0             3       15                  末尾-16 
┌─────────────┬───────┬───────────────────┬───────┐
│ "v10"/"v20" │ nonce │ ciphertext        │ tag   │
│  3 B        │ 12 B  │                   │ 16 B  │
└─────────────┴───────┴───────────────────┴───────┘
版本号         随机数   AES-256-GCM 输出    防篡改校验
```

其中：
- 版本号：目前 Windows 上只有 v10 和 v20 两种加密方式
- nonce：随机生成 12 字节，每条记录独立、只用一次
- ciphertext：密文 ct，与明文等长
- AES-256-GCM(v10/v20 Key, nonce, 明文) 运算（AAD 为空）得到密文 + tag（用于校验）

其中密文要用对应加密版本（v10/v20）的 Key 解密，`Local State` 的 `os_crypt` 节点存放核心密钥：

```json
{
  "os_crypt": {
    "encrypted_key": "<Base64 编码>",
    "app_bound_encrypted_key": "<Base64 编码>"
  }
}
```

- `encrypted_key`：Base64(`"DPAPI"` 5 字节前缀 + DPAPI Blob)，Blob 加密保存 v10 Key
- `app_bound_encrypted_key`：Base64(`"APPB"` 4 字节前缀 + ABE Blob)，Blob 加密保存 v20 Key

去掉开头的前缀后，**根据后面的 Blob 解出对应的 Key 是解密的关键点，下文将对不同加密版本说明这些 Blob 是如何得出的、如何从 Blob 解出 Key**。而密文用对应加密版本的 Key 解密后，得到的内容则根据业务而定，例如：

- `cookies` 的 `encrypted_value`：从数据库 Schema 24 起为 SHA256(host_key) + **cookie 值**，开头的“域名指纹”防止密文被跨站拷贝复用
- `logins` 的 `password_value`：直接是**密码本身**

v10/v20 加密密钥在 `Local State` 里跨 Profile 共享；记录加密格式与密钥 Envelope 均存在版本迁移，因此新版浏览器中仍可能保留旧版格式。

### 加密版本 v10：DPAPI

[Windows CNG DPAPI](https://learn.microsoft.com/en-us/windows/win32/seccng/cng-dpapi) 是数据保护 API。它只绑定用户，除了系统保存的用户级 Master Key（`%APPDATA%\Microsoft\Protect\<SID>\`），调用者还可以再传入一个可选熵参与加密（Chromium 没有使用可选熵，用了也不见得提升安全系数 ┐(￣ヮ￣)┌）。

```text
加密：CryptProtectData([in] 明文, [in, opt] 可选熵, [out] 密文)
解密：CryptUnprotectData([in] 密文, [in, opt] 可选熵, [out] 明文)
```

**同一登录用户下的任意进程都能解开 v10 Key。**

首次需要加密时生成 v10 Key：

1. 生成随机 **32 字节 AES-256 密钥，这就是 v10 Key**
2. 调用 `CryptProtectData(v10 Key)` 得到 DPAPI Blob
3. 把 `"DPAPI"`（5 字节前缀）拼到 DPAPI Blob 前面，再进行 Base64 编码
4. 写入 `Local State` 的 `os_crypt.encrypted_key` 字段

后续根据 `Local State` 的 `os_crypt.encrypted_key` 字段反向运算即可拿到 v10 Key，然后进行加解密。十分简单，几行 PowerShell 就能把 v10 Key 解密并打印出来（以系统内置 Edge 为例）：

```powershell
# 1. 读 Local State，取 encrypted_key，Base64 解码
$blob = [Convert]::FromBase64String((Get-Content "$env:LOCALAPPDATA\Microsoft\Edge\User Data\Local State" -Raw | ConvertFrom-Json).os_crypt.encrypted_key)

# 2. 验证前缀 "DPAPI" 并剥掉，DPAPI 解密
if ($blob.Length -le 5 -or [System.Text.Encoding]::ASCII.GetString($blob, 0, 5) -cne 'DPAPI') { throw 'Invalid encrypted_key: missing DPAPI prefix' }
Add-Type -AssemblyName System.Security
$key = [Security.Cryptography.ProtectedData]::Unprotect($blob[5..($blob.Length-1)], $null, [Security.Cryptography.DataProtectionScope]::CurrentUser)
if ($key.Length -ne 32) { throw "Invalid v10 key length: $($key.Length)" }

# 3. 打印
($key | ForEach-Object { $_.ToString("X2") }) -join ''
```

### 加密版本 v20：ABE（App-Bound Encryption，应用程序绑定加密）

引入 `elevation_service.exe`，注册为**以 SYSTEM 身份执行的系统服务**（如 `GoogleChromeElevationService` / `MicrosoftEdgeElevationService`），开放 COM 接口供浏览器调用，通过 COM 为浏览器封装、解封 v20 Key。

首次需要加密时：

1. 浏览器生成随机 **32 字节 AES-256 密钥，这就是 v20 Key**
2. 浏览器调用 `IElevator::EncryptData`，把 v20 Key 和“保护级别”（见后文）交给服务
3. **服务私有处理，Chromium 源码中目前通过 `GOOGLE_CHROME_BRANDING` 编译器开关控制，如果启用，则调用私有 `PreProcessData`，把原始 v20 Key 转换成私有 Envelope，未来取 Key 时再对等地调用私有 `PostProcessData` 处理。Edge 也有这步，具体见后文**
4. 服务模拟客户端（`IServerSecurity::ImpersonateClient`），并取得 **“调用方路径”**（`I_RpcOpenClientProcess` + `QueryFullProcessImageNameW`），把校验数据与 v20 Key （或者转换成的私有 Envelope） 一起在**调用方用户上下文**中执行 DPAPI 加密
5. 服务退出模拟，再在 **SYSTEM 上下文**中执行 DPAPI 加密，得到 ABE Blob，返回给浏览器
6. 浏览器拼接 `"APPB"` + ABE Blob，Base64 编码后写入 `os_crypt.app_bound_encrypted_key`

后续浏览器通过 `Local State` 的 `os_crypt.app_bound_encrypted_key` 字段拿到 ABE Blob，通过 COM 得到 v20 Key，即可进行加解密。

可见沿用了 [Windows CNG DPAPI](https://learn.microsoft.com/en-us/windows/win32/seccng/cng-dpapi)，并在 SYSTEM 上下文进行验证及加解密操作。

#### Chromium 定义的“保护级别”：

- `PROTECTION_NONE`：不校验应用身份
- `PROTECTION_PATH_VALIDATION`：解封时要求当前调用方的规整路径与封装时一致
- `PROTECTION_PATH_VALIDATION_WITH_ISOLATION`：除路径一致外，还校验进程的隔离状态

默认值是 `PROTECTION_PATH_VALIDATION_WITH_ISOLATION`，即同时校验“调用方的规整路径”和“进程的隔离状态”：

- 调用方的规整路径：
  - 去掉 exe 文件名
  - 去掉末尾的 Temp / Application / 版本号目录
  - 把 `Program Files (x86)` 归一化为 `Program Files`
- 进程的隔离状态：
  - Elevation Service (SYSTEM) 以带有隔离标记（`SetTokenInformation(TokenSecurityAttributes)`，例如 Chrome 的 `GOOGLECHROME://ISOLATION`）的 Token 创建浏览器进程
  - 服务查询调用进程 Token 中是否存在此隔离标记，阻止非隔离客户端读取隔离客户端存储的 Key

虽然默认值指示校验进程的隔离状态，且校验代码确已默认启用，但隔离标记只授予经 Elevation Service 以隔离方式启动的浏览器实例，常规启动的浏览器并不携带。加密时记录的隔离状态为 False，解密时双方一致即可通过，故该校验对普通机器尚无实际约束。即使未来有约束，目测绕过的方式依然很多。当下的关键点，便是过掉路径校验。

#### 私有 Envelope

Chrome 和 Edge 基于 Chromium 预留的扩展都做了近乎相同的私有处理。服务拿到 v20 Key 明文后，先转换成私有 Envelope，再做后续的 DPAPI 加密。Envelope 目前又有 3 个版本：

| 版本 | 总长度 | Envelope 布局 | AEAD |
|---|---:|---|---|
| v1 | 61 | `version[1] + nonce[12] + ciphertext[32] + tag[16]` | AES-256-GCM |
| v2 | 61 | `version[1] + nonce[12] + ciphertext[32] + tag[16]` | ChaCha20-Poly1305 |
| v3 | 93 | `version[1] + cng_block[32] + nonce[12] + ciphertext[32] + tag[16]` | AES-256-GCM |

v1 与 v2 只是加密算法不同，用的 Key 是固定的，通过二进制逆向可得到：

| 常量 | 值 |
|---|---|
| v1 AES-256-GCM Key | `B3 1C 6E 24 1A C8 46 72 8D A9 C1 FA C4 93 66 51 CF FB 94 4D 14 3A B8 16 27 6B CC 6D A0 28 47 87` |
| v2 ChaCha20-Poly1305 Key | `E9 8F 37 D7 F4 E1 FA 43 3D 19 30 4D C2 25 80 42 09 0E 2D 1D 7E EA 76 70 D4 1F 73 8D 08 72 96 60` |

v20 Key 的明文直接用以上硬编码 Key 加密得到密文 ct（`ciphertext[32]`），再组合成 Envelope。

而 v3 加入了 CNG。v20 Key 不再被直接加密，而是先随机生成 32 字节的 **Envelope AES Key**：

1. Envelope AES Key XOR 一个固定的 32 字节 Mask，再在服务的 SYSTEM 上下文中读取 Windows CNG/KSP 保存的 Key（Chrome 是 `Google Chromekey1`，Edge 是 `Microsoft Edgekey1`）加密，得到 `cng_block[32]`
2. v20 Key 用 Envelope AES Key 做 AES-256-GCM 加密，得到 `nonce[12] + ciphertext[32] + tag[16]`

XOR 用的固定 Mask 通过二进制逆向可得到：

| 常量 | 值 |
|---|---|
| v3 XOR Mask | `CC F8 A1 CE C5 66 05 B8 51 75 52 BA 1A 2D 06 1C 03 A2 9E 90 27 4F B2 FC F5 9B A4 B7 5C 39 23 90` |

## 4 种 v20 解密方案

### 过路径校验

#### 1. Drop：把可执行程序扔进浏览器目录

十分简单直接，既然验证客户端路径，那把程序扔进浏览器目录里路径就对了。我想到要考虑的有：

- 需要管理员权限才能向启用 ABE 的浏览器安装目录丢东西（Per-User 安装的浏览器 ABE 也用不了），例如 Windows 内置的 Edge
- 缺少技术上的纵深
  - “进程的隔离状态”或者未来的防御手段可以轻易防住
  - EDR 特征明显，且文件监控极其容易实现

#### 2. Inject：注入到浏览器进程

DLL 注入对大家来说已是基操，但 DLL 也不是注入必需的，Map + `CreateRemoteThread` 就够了。将自身代码映射到浏览器进程，然后用远端线程执行它。

- 无需管理员权限
- EDR 特征明显

#### 3. Hijack：劫持浏览器进程执行代码

这条路线可以说是 Inject 方案的升级版，思路一致，无非是把代码（调用 Elevation Service 接口进行解密）放到浏览器进程空间里执行。只是用 `CreateProcess` + Map，主线程代替远端线程，归并为一个方案也不为过。

如果想做得足够优雅，还是比较考验技术的。[xaitax/Chrome-App-Bound-Encryption-Decryption](https://github.com/xaitax/Chrome-App-Bound-Encryption-Decryption) 是一个值得参考的例子，我在 [KNSoft.ZPigeon](https://github.com/KNSoft/KNSoft.ZPigeon) 中落地的方案也算是 Hijack。

EDR 特征仍然明显，但劫持的方式太多了，看是道高一尺还是魔高一丈。

### 调用 Elevation Service COM 接口 `DecryptData`

过掉路径校验后，问题就是如何调用 Elevation Service 接口。接口定义可以参考 Chromium 的 `elevation_service_idl.idl`。要注意的是，不同版本、不同浏览器接口定义可能不同（例如 Edge 就多了 3 个专有方法，`DecryptData` 的槽位被顺延）。

```
IElevator : IUnknown {
    RunRecoveryCRXElevated(...)      // 槽 3
    EncryptData(...)                 // 槽 4
    DecryptData(BSTR in, BSTR* out, DWORD* lastError)   // 槽 5 ★
}
IElevator2 : IElevator { RunIsolatedChrome, AcceptInvitation }        // 槽 6-7
IElevatorEdgeBase : IUnknown { 3 个 Edge 专有方法 }                    // 槽 3-5（Edge 特有的插入层）
```

| 浏览器/版本 | CLSID（elevation_service 类） | IID | `DecryptData` 槽位 |
|---|---|---|---|
| Edge | `{1FCBE96C-1697-43AF-9140-2897C7C69767}` | `IElevatorEdge {C9C2B807-7731-4F34-81B7-44FF7779522B}`（= EdgeBase(3) + IElevator） | **8** |
| Chrome 127–151 | `{708860E0-F641-4611-8895-7D867DD3675B}` | `IElevatorChrome {463ABECF-410D-407F-8AF5-0DF35A005CC8}` | 5 |
| Chrome 152+ ★ | 同上 | `IElevator2Chrome {1BF5208B-295F-4992-B5F4-3A9BB6494838}`（= IElevator2 + IElevator） | 5 |

### 不依赖浏览器自身服务，对着密文硬解

盘算一下，只要有管理员权限，浏览器 Elevation Service 能做的事情我们也能做，只有 SYSTEM 上下文免不了，不论是 DPAPI 还是 v3 用的、浏览器在 CNG 里藏的那把 Key 都需要 SYSTEM（当然，从 LSA 那想办法拿到 SYSTEM 的 CNG Key 也一样，但这就是另一个话题了）。

#### 4. Elevate：提权到 SYSTEM

之前的 3 个方案多少有入侵行为。即使是 Drop，也欺骗了路径验证，还往人家的安装目录乱扔了东西。

![](Assets/Drop.png)

此方案需要管理员权限，然后给自己提权到 SYSTEM 上下文，进而 Elevation Service 做的加密我们就可以解密，产品化方案更适合基于此路线。

根据 v20 Key 的加密流程，我们照着前文所写的首次加密落盘步骤一步步反过来做：

- 第 6 步的逆：拿到 `Local State` 里 os_crypt.app_bound_encrypted_key，Base64 解码，拆掉前缀 `"APPB"` 得到 ABE Blob
- 第 5 步的逆：切换到 SYSTEM 上下文，调用 DPAPI `CryptUnprotectData` 剥掉一层
- 第 4 步的逆：切换到用户上下文，调用 DPAPI `CryptUnprotectData` 剥掉一层，得到**内层结构**
- 第 3 步的逆：如果大小为 32 字节，则判断无私有 Envelope，跳过本步骤，否则判断 Envelope 版本，对应解密
  - 61 字节 v1：用逆向得到的内嵌 AES-256 Key，AES-256-GCM 打开 nonce[12] + ct[32] + tag[16]
  - 61 字节 v2：同上，换 ChaCha20-Poly1305 算法与对应的内嵌 Key
  - 93 字节 v3：
    - 切到 SYSTEM 上下文打开 Chrome/Edge 在 CNG/KSP 中保存的、用于包裹 Envelope AES Key 的那把 Key：`NCryptOpenStorageProvider(Microsoft Software KSP) + NCryptOpenKey(Google Chromekey1 / Microsoft Edgekey1)`
    - `NCryptDecrypt` 解开 `cng_block[32]`，与逆向得到的固定 Mask 逐字节异或得到 Envelope AES Key
    - 再用它解开其后的 nonce[12] + ct[32] + tag[16]

至此拿到 32 字节 v20 Key。之后每条 "v20" 前缀的记录按 nonce[12] + ct + tag[16] 用它做 AES-256-GCM-Open 即可。

Edge 与 ChatGPT 采取了这个方案，具体实现大同小异。

Edge 本就有全套 Elevation Service，只需把 v20-v3 读取 CNG/KSP 的密钥名称加上 Chrome 的就可以借助自身机制实现导入。逆一下 Edge 的 `elevation_service.exe`，追 `NCryptOpenKey` 调用一看便知：

![](Assets/Edge-Import-1.png)

用户只需在 Edge 里点击一下，无需再显式 UAC 授权即可导入。如果阻止 `MicrosoftEdgeElevationService` 服务启动，则无法导入：

![](Assets/Edge-Import-2.png)

ChatGPT 没有自己已安装到系统的 Elevation Service，故得自己提权到 SYSTEM：

![](Assets/ChatGPT-Import-1.png)

显式 UAC 提权到管理员后，运行 2 个 ChatGPT.exe 子进程，命令行样本：

```text
"C:\Program Files\WindowsApps\OpenAI.Codex_26.901.4073.0_x64__2p2nqsd0c76g0\app\ChatGPT.exe" --owl-browser-profile-import-broker="\\.\pipe\owl-browser-profile-import-9c93e555-ec01-4812-b76c-dbd774bc8436"
"C:\Program Files\WindowsApps\OpenAI.Codex_26.901.4073.0_x64__2p2nqsd0c76g0\app\ChatGPT.exe" --owl-browser-profile-import-system-service="\\.\pipe\owl-browser-profile-import-1369c23f-7770-4a51-b7ee-c85f27101757"
```

它们分别处于用户上下文和 SYSTEM 上下文（注册服务方式提权），通过管道互通有无，共同完成上述一系列解密流程。Chrome 导入核心实现位于其目录中的 `chrome.dll`，直接二进制扫描可见其中包含前文提到的 v1/v2 Key 及 v3 XOR mask，还有相关文本：

![](Assets/ChatGPT-Import-2.png)
![](Assets/ChatGPT-Import-3.png)

正是前述方案，无需进一步逆向分析。

## PoC 实现与 KNSoft.ZPigeon 工程化落地实践

刚才说了 To C 的产品化方案，还例举了 Edge 和 ChatGPT 的实现，前者借助已注册的 SYSTEM 服务，后者需要一次交互的显式 UAC 提权。而对于面向 To B 资产的远程管理则通常以已具备对本机器法律意义上的所有权为前提（企业资产），故无需与使用者交互，且可以得到 EDR 的默许，更适合之前提到的 Inject/Hijack 方案。下面以 [KNSoft.ZPigeon](https://github.com/KNSoft/KNSoft.ZPigeon) 为例，配合 PoC 分享实践上的思路与技术点。至于代码本身，Vibe 实现后尚未得空逐一打磨，若能入眼且可将就一看。代码未必符合本文所述思路，来日再慢慢打磨了。

### 数据读取

ABE v20-v3 加密机制自身需要管理员权限，故无需考虑 Per-User 安装的版本。接下来根据默认路径或者注册表信息找到 Edge/Chrome 的 `User Data` 目录，再枚举其中 Profiles 就能找到目标数据所在位置。注意读取保存的密码时别只顾着 `Login Data` 库而忘了 `Login Data For Account`。

提高一下目标支持的最低 Windows 版本，即可直接使用 `Windows.Data.Json.dll` 而无需引入第三方用于 C/C++ 的 Json 库；同理，SQLite 也可以直接用 `winsqlite3.dll`：

```C
#define SQLITE_API __declspec(dllimport)
#include <winsqlite/winsqlite3.h>
#pragma comment(lib, "winsqlite3.lib")
```

这部分代码在 PoC 示例项目所在仓库 [KNSoft.MakeLifeEasier](https://github.com/KNSoft/KNSoft.MakeLifeEasier) 的 [Net\Browser.c](https://github.com/KNSoft/KNSoft.MakeLifeEasier/blob/main/Source/KNSoft.MakeLifeEasier/Net/Browser.c) 。

### 代码执行

调用 `CreateProcessInternalW` 以 `CREATE_SUSPENDED` 方式创建浏览器进程，调用 `NtAllocateVirtualMemory` + `NtWriteVirtualMemory` 写入 Payload。对各位来说应该已是一气呵成的基操，但要做得优雅还得注意：

- 写入 Payload 往往需要修复重定位，可以根据自身 PE 重定位表精准修复，NTDLL 的 PE Loader 这块实现并不复杂，参考：
  - [KNSoft.NDK 中 `ntdll!LdrProcessRelocationBlockEx`, `ntdll!LdrProcessRelocationBlockLongLong` 的实现](https://github.com/KNSoft/KNSoft.NDK/commit/eb8da94c3a431f1e146df63624ed49c612ffb9e2)
  - [KNSoft.MakeLifeEasier 中 `PE_RelocateImage` 的实现](https://github.com/KNSoft/KNSoft.MakeLifeEasier/blob/26188476d895b5237150b7f214b851a0a13adafe/Source/KNSoft.MakeLifeEasier/PE/Util.h#L56)
- 优雅收尾，`NtProtectVirtualMemory` 将内存页属性由 RW 改为 RX（而不是始终 RWX），再用 `NtFlushInstructionCache` 刷一下

本文 PoC Vibe 出来的比较粗糙，映射了自身整个 PE 过去，实际可以做得更精准。接下来调用 `NtGetContextThread` + `NtSetContextThread` + `NtResumeThread` 即可执行 Payload，Payload 调用 Edge/Chrome 的 COM 接口拿到 v20 Key。

### 数据库锁

Chromium 对 `Login Data` 库的打开设置了独占锁 `PRAGMA locking_mode=EXCLUSIVE`，此时需要设置 `nolock=1` 或 `immutable=1`。之所以不是 Windows 文件句柄的独占锁，是因为 Chromium 的 Windows-only 实验性机制 `set_exclusive_database_file_lock`（直接以独占方式持有文件句柄）正在铺，为 `Login Data` 库的启用还在日程上。

但它已经为 `Cookies` 库启用了，为其它重要的库启用也是迟早的事，工程侧需要考虑此场景。ChatGPT 导入内容包含 Cookie，界面会提示导入前需要先完全关闭 Chrome；Edge 导入内容不包含 Cookie，但在 Chrome 运行时（用导入目标 Profile），仍会有一些数据无法导入。

我们进程的权限不比浏览器低，所以即使它独占了文件，我们仍然有办法访问，可以直接做成 Fallback 应对此场景。PoC 与 [KNSoft.ZPigeon](https://github.com/KNSoft/KNSoft.ZPigeon) 均已实现，技术方案不复杂：

- `NtQueryInformationFile(..., FileProcessIdsUsingFileInformation)` 获取占用数据库文件的 PID 列表
- `NtQueryInformationProcess(..., ProcessHandleInformation, ...)` 获取进程句柄表，找到独占数据库的句柄
- `NtDuplicateObject` 复制句柄，然后读取文件内容到自身进程内存
- `sqlite3_open_v2(":memory:", ...)` + `sqlite3_deserialize` 读取内存数据库

## 结语：AI & Security

可见，浏览器自身的挣扎几近徒劳：徒增解密成本、抬高产品化门槛，却没防住真正的攻击者。大家都在同一个权限平面上，谁防得住谁？也许得借助更高层次的保护，如 Credential Guard、VBS Enclave、TPM；也许该把保护机制交给 AV/EDR/HIDS 这些角色来做。比起这些，我想得更多的是 AI & Security。

AI 能力的提升对各领域都带来了极大的机遇与挑战，安全同样如此。不论 AI for Security 还是 Security for AI 都亟待关注与探索。

本来做 [KNSoft.ZPigeon](https://github.com/KNSoft/KNSoft.ZPigeon) 是想探索如今 AI-Driven 与 Human-Driven 之间如何两全，让 AI 高效生成的大量功能代码稳定高效、可维护、专业度高，以及 AI 自闭环地实现“设计方案-编码-测试-改进”自迭代。但在实现此功能的过程中，GPT 以可能涉及 Cyber Security 为由拒绝了我无数次。

切换到 GLM 5.3 Max，虽然它不拒绝我了，但几经波折：

- 我浏览器的所有 Cookie 被意外清空了
  - 它写 PoC 时，调用 Chrome 的 Elevation Service COM 收到 `E_NOINTERFACE` 错误，实际原因是 **Chrome 接口随版本更新换代，而它用的是老接口的 IID**
  - 但它以为报错原因是 Chrome 没有注册此接口，然后**自行往 HKCU 里写入 TypeLib 信息（`RegisterTypeLibForUser`），写入的信息还是错的**
  - 浏览器优先使用 HKCU 里的错误信息，访问 COM 失败，Chromium 的故障恢复机制主动清空了全部无法解密的 v20 Cookie
  ![](Assets/GLM-5.3-1.png)
- 把 `SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX` 结构套给了 `PROCESS_HANDLE_TABLE_ENTRY_INFO`，然后得出“24H2+ 内核把这个结构的 `Object/UniqueProcessId` 字段删掉了”的结论，我发现并纠正了。它还自己**做实验证明因自己抄错结构得出的错误结论是正确的**，还脑补结构体变更的动机是“不再泄露内核指针”
  ![](Assets/GLM-5.3-2.png)
- 代码逻辑不合理，读取了不需要的变量，而这些变量由于不需要而未经初始化，直到 PoC 运行时被 RTC 抓住而崩溃
  ![](Assets/GLM-5.3-3.png)
- 把自己写的等待逻辑（10s+）计算在方案耗时里，然后凭空脑补把耗时归咎到“Edge 全量导入表加载 + 服务冷启动”，我提出质疑后找到计算问题并修正
  ![](Assets/GLM-5.3-4.png)

还有别的就不一一例举了，但一路过来也算是帮了不少忙。GLM 5.3 的宣传片中说让 AI 站在蓝方，但如果事与愿违，便不得不考虑：

- 如果面对 AI 的进攻，我们能否守得住？
- 如果面对 Mythos/GPT 这些模型的进攻，我们与手里的开源模型是否守得住？

GLM 5.3 在宣传上突出了安全方面的能力，是个很好的开始，至少说明国内前沿的模型开始关注 AI for Security。下次再做类似的事情时，GPT 还是会拒绝我，我还是会用 GLM 5.3，也推荐大家试试。即使不入手 Coding Plan，通过智谱自家的 [AutoClaw](https://autoclaw.zhipuai.cn/) 也能获取一些免费使用额度。

### 相关资料与项目

- [Google Security Blog: Improving the security of Chrome cookies on Windows](https://security.googleblog.com/2024/07/improving-security-of-chrome-cookies-on.html)
- [xaitax/Chrome-App-Bound-Encryption-Decryption](https://github.com/xaitax/Chrome-App-Bound-Encryption-Decryption)
- [runassu/chrome_v20_decryption](https://github.com/runassu/chrome_v20_decryption)
- [GLM-5.3：前沿编程能力与涌现的网络安全能力](https://mp.weixin.qq.com/s/JdBx3B12L7Mhxruf4Nfr4g)

<br>
<hr>

本作品采用 [知识共享署名-非商业性使用-相同方式共享 4.0 国际许可协议 (CC BY-NC-SA 4.0)](http://creativecommons.org/licenses/by-nc-sa/4.0/) 进行许可，如有错漏欢迎指出。  
<br>
**[Ratin](https://github.com/RatinCN) &lt;[ratin@knsoft.org](mailto:ratin@knsoft.org)&gt;**  
*中国国家认证系统架构设计师*  
*[ReactOS](https://github.com/reactos/reactos)贡献者*