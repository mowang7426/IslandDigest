# 灵屿通知

> **0.1.1、0.1.2、0.1.3 均已收到注销卡死反馈，停止推荐安装。** 不要继续安装这些历史版本。
> 已安装的设备应先在禁用插件注入或安全模式下卸载本包，再恢复正常注入。
> 现有构建和签名检查未覆盖 SpringBoard 启动、通知列表交互及注销后的稳定性。

面向 iOS 15–17 的锁屏通知整理插件。当前源码为 **0.1.4 候选版**，尚未验证真实注销启动；以应用分组和数量展示为主，不生成 AI 文本摘要。

## 0.1.4 候选：筛选恢复重入修正

新增测试在 0.1.3 实现上失败：恢复一个应用的筛选时，同步回调再次调用恢复逻辑，
会在外层系统调用返回前反复恢复同一应用。模拟回调设置八次预算，以安全复现递归，
不让测试进程实际卡死。该缺陷需要已有筛选状态，不能单独解释未操作筛选时的启动卡死。

0.1.4 使用独立的筛选调用保护，回调只记录待协调状态，等当前系统调用返回后再协调；
协调最多两轮，异常仍保留未完成的恢复记录并走原有的有限恢复机制。
日志主路径打开失败后尝试 `/tmp/IslandDigest.log`，并记录 errno；
两个路径都无法打开时输出标准错误。此日志补充不等同于修复卡死。

本机 arm64、arm64e 各 20 项回归与 13 项真实接口测试通过。
候选 DEB 的静态检查通过；原样动态库仍被 Node 签名策略拒绝，
单独重签的测试副本完成了生产初始化、空通知界面生命周期和 30 秒检查点。
重签副本通过不能证明原包在 SpringBoard 中可加载，更不代表真实注销已通过。
本轮没有安装或注销，没有将候选包作为稳定修复版交付。

候选包检查与产物加载测试：

```sh
python3 tests/check_package.py packages/com.local.islanddigest_0.1.4_iphoneos-arm64e.deb 0.1.4
bash tests/run_artifact.sh packages/com.local.islanddigest_0.1.4_iphoneos-arm64e.deb
bash tests/run_artifact.sh packages/com.local.islanddigest_0.1.4_iphoneos-arm64e.deb --resign-copy
```

测试仅在独立进程运行，不安装插件。原样加载失败必须保留为失败结果。
0.1.4 的启动标记为 `/var/mobile/Library/Logs/IslandDigest-0.1.4.startup-pending`；
日志包含此前测试 PID，不能据此误判 SpringBoard 已运行插件。

## 0.1.3 故障后的补充验证

用户确认在本机安装 0.1.3 后注销持续卡住，强制重启后已卸载。故障根因尚未确认。
2026-10-21 增加直接加载 DEB 内动态库的测试，不再仅依赖把源码编入测试模块。
原样动态库在独立 Node 进程中被代码签名策略拒绝；只对提取出的测试副本重新签名后，
生产构造器、Hook 安装、空通知控制器进出和 30 秒启动检查点均完成，且阶段日志成功写入。
该签名拒绝仅证明 Node 测试进程的加载结果，不证明 SpringBoard 中发生相同故障。
测试副本重新签名后的通过结果不验证原包签名，也不等同于真实注销成功。
原 DEB 保持不变，本轮未安装插件、重启 SpringBoard 或交付新修复包。

新增测试入口为 `tests/ArtifactLoad.mm` 与 `tests/run_artifact.js`。
测试会通过生产构造器写入实际阶段日志；本次记录的 PID 为 2860，是独立测试进程，
不能将这些记录当作 SpringBoard 故障现场。启动标记已在检查点删除。

## 0.1.3：真实接口复现、修复与运行验证

在本机 iOS 17.0 的真实 UserNotificationsUIKit 控制器中复现：
`filteredNotificationRequests` 返回按应用组织的字典，旧代码的数组/集合读取逻辑遗漏其中的通知。
选中应用后再刷新，实际分组数由 2 变为 1，已应用筛选数由 1 变为 0。
0.1.3 递归读取字典值及其中的通知集合，合并去重；修复后真实列表连续刷新 30 次仍保持分组与筛选。
这证明修复了筛选数据错误，不能据此认定它就是注销立即卡死的唯一原因。

原生测试已在 arm64、arm64e 分别实际运行，各 31 项通过：

- 模拟数据源回归：空列表、重复刷新、字典通知、筛选与恢复、关键警报、异常清理。
- 真实系统集成：生产 Hook 注册、真实控制器加载、真实附加容器插入与移除、合成通知增删、原生筛选、连续刷新、界面退出恢复。
- 启动未完成时跳过生产 Hook，避免重复执行上次未完成的启动路径。

测试在独立 Node 进程中使用系统框架和合成通知，不向当前 SpringBoard 投递消息。
每个测试进程有 15 秒超时。测试模块通过 CodexApp 提供的原生模块签名流程签名；
之前直接使用 ldid 的测试产物被加载策略拒绝，不应把那个失败解释为整个运行环境不能做原生测试。

本版增加启动检测：初始化前创建 `IslandDigest-0.1.3.startup-pending`，
主队列到达 30 秒检查点后删除；若上次启动中断留下标记，下次跳过本插件的 Hook。
这是被动停用保护，不会主动杀死或重启 SpringBoard。
正常启动后 30 秒内主动再次注销也可能触发保守停用。
日志与标记位于 `/var/mobile/Library/Logs/`，RootHide 可能重定向路径。

**未完成真实 SpringBoard 注销/锁屏实测，仍未取得历史卡死的线程栈。**
本机当前未找到该插件的安装载荷与对应的新故障日志，不能确认用户故障的完整根因。
iOS 15、16 尚未运行验证。

构建与测试：

```sh
bash tests/run.sh
ID_TEST_ARCH=arm64e bash tests/run.sh
make package FINALPACKAGE=1 DEBUG=0 THEOS_PACKAGE_SCHEME=roothide
python3 tests/check_package.py packages/com.local.islanddigest_0.1.3_iphoneos-arm64e.deb
```

## 0.1.2 候选修复与自检

- `viewDidLoad` 仅记录归纳栏状态，延后至通知界面显示后插入，避免在视图加载阶段修改系统列表。
- 移除 `_notificationListDidChangeContent` 底层回调的 hook，改为监听通知插入、修改、移除和清空事件，避免附加视图变化驱动自身刷新。
- 插入或筛选异常后停用当前归纳栏，隐藏并禁用按钮；清理失败保留状态，仅在后续外部事件中有限重试，最多三次。持续恢复失败仍需卸载并重新加载 SpringBoard。
- 对私有初始化器使用 ARC 可识别的类型声明，并核对关键标量参数 ABI。
- 增加不含通知内容的阶段日志：`/var/mobile/Library/Logs/IslandDigest.log`。路径可能被 RootHide 重定向。

以下为 0.1.2 当时的测试记录；0.1.3 已补充真实接口集成并成功运行。
`tests/Regression.mm` 使用真实 UIKit 归纳视图与模拟的系统通知数据源，覆盖延后初始化、重复刷新、空列表、应用筛选、退出恢复、关键警报和异常清理。
它不加载 SpringBoard，不代替真实通知事件、注销与锁屏实测。
测试库已编译，但本次 Python 直接加载受 mmap 限制，Node 执行镜像加载受内核代码签名策略拒绝，故**原生回归尚未运行，不计为通过**。

在允许加载本地原生测试模块的环境中运行：

```sh
bash tests/run.sh
```

包静态检查可独立执行，当前 0.1.2 已通过：

```sh
python3 tests/check_package.py packages/com.local.islanddigest_0.1.3_iphoneos-arm64e.deb
```

## 界面与操作

- 在系统通知列表中插入一张圆角磨砂卡片，展示应用数量、通知总数和横向应用胶囊。
- 胶囊包含应用图标、应用名称与通知数；按最新通知时间排列。
- 点选应用筛选通知，再点一次或点“全部”恢复；关闭锁屏通知界面时恢复本插件的筛选。
- 没有通知时移除归纳卡片；不移动系统时钟，不替换系统通知卡片。
- 通知展开、回复、打开应用与清除仍由系统处理；筛选不删除通知。
- 带有关键警报的应用不参与本插件的隐藏筛选。
- 支持深浅色外观、VoiceOver 标签与选中状态。首版没有独立设置页面。

打开 `preview.html` 可查看交互设计示意。网页使用虚构通知和简化图标，不是实机截图；实际壁纸、时钟、通知卡片和外观由系统提供。

## 兼容范围

开发时读取本机 iOS 17.0（21A329）的 UserNotificationsUIKit 运行时，核对了通知列表、附加视图与筛选接口。
启动时再次检查所需接口；不匹配时不启用插件。
最低部署版本与安装依赖范围已设为 iOS 15.0–17.x；iOS 15、16 的私有接口及界面尚未实机核验，不能将包的安装范围视为所有系统版本均已验证。
通知中心与锁屏可能共用同一系统列表，因此整理栏也可能出现在通知中心。

接口读取和编译通过不等于已通过锁屏实测。尚需安装后验证：
首次加载、已有通知回填、收到/修改/清除通知、反复切换筛选、退出后恢复、
关键警报、专注模式、隐藏预览及与其他锁屏插件的共存。
不在归纳栏显示消息正文，不上传通知内容；消息预览仍由系统隐私设置控制。

## 构建

使用 CodexApp 注入的 `THEOS`、`CA_THEOS_*` 和 `PATH`，支持 arm64 与 arm64e。
默认输出 Rootful 包；设备当前的越狱类型不改变构建目标。

```sh
make package FINALPACKAGE=1 DEBUG=0
```

需要其他包格式时显式指定：

```sh
make package FINALPACKAGE=1 DEBUG=0 THEOS_PACKAGE_SCHEME=rootless
make package FINALPACKAGE=1 DEBUG=0 THEOS_PACKAGE_SCHEME=roothide
```

当前候选构建为原生 RootHide 包 `packages/com.local.islanddigest_0.1.4_iphoneos-arm64e.deb`，
已通过原生回归和真实系统接口集成，尚未完成真实注销实测，不作为稳定修复版推荐安装。
动态库包含 arm64 与 arm64e 两个切片。RootHide 包架构与动态库架构是不同配置。

包生成在 `packages/`。按目标越狱类型选择包，安装后重新载入 SpringBoard。
项目没有自动安装或自动重启脚本。卸载后重新载入 SpringBoard 即可恢复。

## 实现边界

使用 Logos internal 后端与 Objective-C runtime，不链接固定路径的 Substrate。
通过系统 supplementary views 容器承载整理栏，通过原生临时筛选接口更新列表；
保留原始筛选委托返回值，不修改通知授权、专注模式、系统通知分组偏好或通知数据库。
通知变更后在主队列合并刷新，无轮询计时器。
筛选操作记录已尝试隐藏的应用，异常时尝试恢复；失败信息写入系统日志的 `[IslandDigest]` 项。
