# 灵屿通知 0.2.0 · MoWang

基于 `feature/0.1.9-layout`（`ad8f1815daf59d3e513fe733bb367ffc2e003c13`）堆叠。保留 0.1.9 的面板位置/尺寸、卡片布局就绪保护、筛选、排序、置顶、记忆筛选与手动清理 safeguards；本版新增受保护数据门控的解锁通知预览、MoWang 作者元数据及源照片派生的高清 PreferenceLoader 图标。

**通知正文隐私边界：**“解锁时显示通知预览”默认关闭，只有用户明确开启后才为插件授权；卡片读取 title/subtitle/message 前必须在主线程、拥有者界面可见、`protectedDataAvailable` 且 request 的 content/options 均可用。锁定或受保护数据不可用时同步清空卡片及无障碍字符串。iOS 17 运行时头文件只给出不透明的 `contentPreviewSetting` 整数，仓库没有可验证的逐条枚举/有效策略映射，因此不猜值、不用 native view 可见性作权限。明确开启后可能无法保持每个 App 的“永不显示预览”语义；设置页对此警告。未开启、锁定或任一门控未知时保留“通知内容已隐藏”。不写正文到日志、偏好、网络。

要显示预览，请在理解上述限制后，手动开启“解锁时显示通知预览（默认关闭）”。隐藏原生通知列表仍默认关闭；原生列表可继续使用系统正文、回复与展开。卡片仅为静态预览，不承诺点按打开或快捷回复。

## 0.2.0 隐私与运行时限制

- 明确采用 fail-closed：没有 owner、content/options、主线程、解锁/受保护数据条件时不读取正文。
- `contentPreviewSetting` 未被解释或猜测；这是无法证明每 App Never 语义时的安全选择，而非绕过系统策略。
- 锁屏通知到达 `UIApplicationProtectedDataWillBecomeUnavailable` 时同步清除已渲染的卡片文本、AX label/value/hint；恢复后重新快照。
- 图标来自用户提供的第二张源图（`photo_7764197E.jpeg`），裁切为人物肖像方图并输出 1024px master 与 2x/3x PNG；第一张截图未作为图标。
- 作者及包元数据统一为 **MoWang**。

## 以下为 0.1.8 隐私及历史功能说明

基于 `feature/0.1.7-clear` / `7f508f0840a1ffc60ff7cd61c2986bb041f93feb`。新增同面板逐条真实通知元数据卡片（默认开启）和隐藏原生列表开关（默认关闭，依赖插件内模式）。历史版本在未验证逐条策略时始终显示占位；原生通知列表默认保留以查看实际内容及回复。达到卡片上限、接口不匹配或枚举不可靠时保留原生；退出、停用或恢复时撤销隐藏，不清除任何通知。

隐私头文件证据、当前限制、新设置和诊断、完整测试/真机验收边界见 `IDContainedSupport.inc` 与下方历史记录。macOS CI 编译不代表 iOS17 Relaxin 真机验证通过。以下 0.1.7 清理与历史说明予以保留；插件内模式下应用选择仅改变本面板卡片范围，不筛掉原生消息。

## 继承的 0.1.7 通知清理功能

基于 `feature/0.1.6-experience` 的 `0c9f871917c393f1b9fcce48d6246acaf1712769`，保留下面四项体验功能及 0.1.5 已工作的注入/设置/恢复基线。新增标题栏垃圾桶和设置中的手动清理开关（默认开启，始终确认）。选中 App 仅处理该 App；“全部”处理可靠枚举的全部应用中合格项。点击时建立不可变原生 request 快照，确认时重验证资格/存在性/身份/界面上下文，新到通知不纳入。关键、不可清理、受保护或缺失保护检测能力的请求保留；能力或解锁状态不能验证时停用并显示原因。原生真实清除不可撤销，不使用隐藏替代。

清理 API 的固定运行时头文件出处、严格资格策略与实现限制见 [CLEAR-API.md](CLEAR-API.md)。清理后最多三次只读复核，只报告快照通知已消失/仍在或结果未知，绝不因 void 调用返回就报成功。取消、切换筛选、关闭界面和设置变化会使待确认过期，不自动/定时清理，不重复派发。

用户已确认 **0.1.5「很好，可以了」**（iOS17 + Relaxin，小版本未知）；0.1.6/0.1.7 的新增交互仍需该设备验收。0.1.5 与 0.1.6 原包保持不变，可回退。开发测试不安装、不注销、不删除设备真实通知。

## 保留的功能与使用方式

1. **按应用置顶**：长按应用胶囊约 0.55 秒置顶 / 取消，带图钉标记，触感与无障碍播报；长按取消点按跟踪，不同时切换筛选。VoiceOver 提供“置顶应用 / 取消置顶”自定义操作。设置 → 灵屿通知 → 管理置顶应用提供可读名称开关和确认后取消全部置顶。无通知的置顶应用不会制造空胶囊，有通知时再次靠前。
2. **排序**：默认最近通知优先，也可按通知数量降序或应用名称（系统本地语言）排序；置顶始终优先，再按所选模式；平局用固定应用标识稳定排序。更新比较包括日期、名称、图标、关键提醒状态，不只比较数量。设置变更先重新快照和排序再渲染。
3. **布局**：默认保持原先紧凑横向胶囊；可切换多行网格。窄屏一列、手机通常两列、宽屏三列。最多三行且按可用高度缩小，超出在面板内部滚动。支持旋转、字体缩放（应用文字缩放至 22pt、行高上限 80pt）和有界高度；通过 guarded preferredContentSize 更新 supplementary，0.1.9 改为独立的窗口边界约束调整 UIKit 回退（不修改系统 safe-area），不替换系统卡片，不从每次布局重建面板。
4. **筛选记忆（默认关闭）**：默认每次新打开显示全部；开启“记住上次筛选的应用”后，下次真正出现才尝试恢复。持久期望应用与当前原生筛选分离，退出时同步恢复原生隐藏应用，布局与数据刷新不会反复重置点击。目标已无通知则清空期望并显示全部。新应用通知更新数量而不抢走选择。关闭总开关/分组会恢复全部并清除期望。关键提醒保护及原生回复、展开、清除保持不变。

不重排系统通知卡片、不自动清除；只有用户点击并确认才提交系统真实清除。不复制内容、不使用 AI、不联网发送通知。

## 本地偏好与隐私边界

沿用 `com.local.islanddigest`、current user / any host CFPreferences；保留 Enabled / Grouping / ShowCounts 与 Darwin 实时通知。新增 LayoutMode=compact|grid、SortMode=recent|count|name、RememberFilter=false、PinnedApps、DesiredApp。管理页只保存/显示已置顶和当前观察到通知的应用名称，不枚举全部已安装应用，也不要求输入 bundle ID。最多置顶 128 项，当前观察缓存最多 128 项，名称缓存最多两者并集；这些标识仅在本机功能偏好中使用，不写入诊断白名单，不记录通知标题、正文或内容。卸载后的偏好清理由包管理器/用户决定。

## 安装与验收

* Relaxin 使用 **原生 RootHide (`iphoneos-arm64e`)** 包；Rootless 包仅供其他 Rootless 环境，不要在 Relaxin 误装。确认 PreferenceLoader、插件注入正常，按包管理器流程自行升级。保留 0.1.5 原包，出现问题可在包管理器降级，必要时关闭插件注入后再操作；勿强制忽略依赖。
* 打开通知中心，至少用两个应用的通知测试：点按筛选 / 全部、长按置顶不筛选、图钉、名称管理与重置、三种排序、横向 / 网格滚动、旋转和大字。
* 默认模式：筛选应用 → 退出 → 再进应全部；记忆模式：同流程再次进入应恢复应用；把该应用通知清完再进应全部；新应用通知不抢选择。退出后、关闭启用或分组后，原生通知应全恢复。测试关键提醒、原生展开、回复与手动清除。
* 诊断保留刷新、重试初始化 / 恢复分组。`hooks-installed` 仅说明注册钩子，`panel-visible` 仅说明插入调用成功；两者都不代替肉眼和真机运行验证。

## 验证与兼容性

沿用已证明的 iPhoneOS16.5 SDK、iOS15 最低目标、Theos/RootHide 双方案、arm64+arm64e、classic linker、禁用 chained fixups；不引入工具链变更。严格检查版本、安装路径、依赖、资源白名单、两组可执行文件的架构、SHA-256 CodeDirectory 每页签名。

```sh
python3 tests/check_source.py
cc -Wall -Wextra -Werror tests/Experience.c -o /tmp/id-experience-tests
/tmp/id-experience-tests
cc -Wall -Wextra -Werror tests/Clear.c -o /tmp/id-clear-tests
/tmp/id-clear-tests
python3 tests/check_package.py packages/<roothide>.deb 0.1.7 roothide
python3 tests/check_package.py packages/<rootless>.deb 0.1.7 rootless
```

Experience.c 编译并执行生产实际调用的排序/列数/可见行数纯 C 实现，覆盖置顶优先、三种排序、平局、同数量时间变更、小屏/宽屏/大字边界。源码契约与包检查是静态验证，不是 UIKit 生命周期测试。历史 Node/iOS harness 在 macOS runner 不运行，绝不算作本版通过。私有通知接口、supplementary 高度传播、滚动/手势与 Relaxin 新版运行行为仍需用户真机确认。

---

# 历史 0.1.5 与更早开发记录（不是 0.1.6 验证结果）

# 灵屿通知 0.1.5

按应用分类的通知选择器，保留原生通知、回复、展开、滑动清除；不复制通知卡片、不生成 AI 摘要、不联网发送通知内容。

## 当前验证边界

用户反馈 0.1.4 安装和注销不再卡住，但完全无效果；这不证明未注入或任何单一根因。0.1.1–0.1.3 有卡死反馈，不推荐安装。0.1.5 是待真机验收版本，编译和软件包签名检查不等同于实际 SpringBoard 运行通过。本轮不在设备安装或注销。

设备新信息：iOS 17，Relaxin，具体小版本未知。已核查 [Relaxin 官方源码](https://github.com/owngoal-dev/Relaxin) 快照 `f44e0acfef4453989381736e0c86b9c14e7ea045`：README 标明 iOS/iPadOS 16.5.1–17.3.1、RootHide；`DevKit/Helpers/update-sileo.sh` 使用 `iphoneos-arm64e`；系统注入源码有 `.safe_mode` 和 `DISABLE_TWEAKS` 检查。因此 Relaxin 对应本项目**原生 RootHide 包**，不是仅因 iOS 17 就改装 Rootless 包。官方源码支持全局插件注入，但不能据此确认用户设备已开启。PreferenceLoader 的实际已安装版本未知；包显式依赖 `preferenceloader`，由包管理器解析，若无法解析请勿强制忽略依赖。

## 0.1.5 改动

* 设置 → **灵屿通知**：启用、按应用分组/筛选、显示数量，使用同一 mobile/current-user/any-host CFPreferences 域；Darwin 通知实时更新。
* 系统设置显示版本、实际系统版本、PID、更新时间、初始化次数、钩子注册、控制器触达、是否可枚举、分区数、应用数、通知数、筛选能力、面板路径与失败原因；超过五分钟提示历史记录。无诊断记录只显示未收到，绝不冒充运行中。
* 初始化不再要求所有私有类和可选回调在最初一次检查全部存在；有限延迟 1/3/8/20/125 秒重试，之后停止；设置可手动重试。新版本启动保护有两分钟时效，登记完钩子即清理，不再等待 30 秒。保护目录改为已存在的 Preferences，避免依赖 Logs 目录；旧版标记不影响新版。保护不可写时仍安全停用并报告。
* 同时覆盖加载、出现和可见布局路径，已有控制器可补建状态；布局回调仅调度异步刷新，不同步插入 UI。退出界面后异步恢复筛选，无 dealloc 钩子或销毁时 UI 修改。
* 私有 supplementary 能力独立检查，不兼容时尝试公开 UIKit 子控制器和 safe-area 面板；不替换系统通知单元。回退路径是否影响某个系统列表的布局仍须真机验收。
* 应用筛选独立检查 ABI；不可用仍可显示分类数量，但应用按钮禁用并显示原因。关键提醒保持可见，筛选只调用原生接口；保留原有有界重入协调。
* 已精确核查 Logos internal 的 `register_hook`：继承方法会保存父类 IMP 并在目标子类 `class_addMethod`，不是直接修改父类；没有证据说明它在本项目中错误。本版没有据此杜撰根因。
* `IslandDigest.dylib` 仅注入 `com.apple.springboard`。设置入口由 PreferenceLoader 加载独立 Preferences bundle，不向 `com.apple.Preferences` 注入通知钩子。
* 两种构建均包含 arm64/arm64e，沿用 iPhoneOS16.5 SDK、iOS15 最低目标、classic linker、禁用 chained fixups。严格包检查同时校验 tweak 和 Preferences 可执行文件的双架构、SHA-256 CodeDirectory 每页签名、部署版本、系统依赖、路径和资源白名单。

## 安装后验收（用户自行执行）

1. Relaxin 确认插件注入开启且非安全模式；在包管理器正常安装 0.1.5 RootHide 包及 PreferenceLoader，不要强制忽略依赖。按包管理器流程自行重启桌面；开发过程未替你执行。
2. 重新打开系统设置 → 灵屿通知，查看状态。`hooks-installed` 仅说明钩子注册，`owner-observed` 说明控制器已触达，`panel-visible` 才说明面板插入调用成功，仍需肉眼验证。
3. 保留至少两个应用的原生通知，打开通知中心，检查“按应用分组”面板和应用数量；点应用仅筛选对应应用，点“全部”恢复。测试原生回复、展开、清除和关键提醒。
4. 切换三个开关，确认不注销也生效；退出通知中心确认原始通知恢复。
5. 若无效果，先“重试初始化 / 恢复分组”，重新打开通知中心，再“刷新运行状态”；提供页面完整截图、iOS 17 具体小版本及 Relaxin 版本。没有 SpringBoard 诊断时先排查注入/安全模式/日志读取，不将其硬说成 ABI 错误。

## 检查命令

```sh
python3 tests/check_source.py
python3 tests/check_package.py packages/<roothide>.deb 0.1.5 roothide
python3 tests/check_package.py packages/<rootless>.deb 0.1.5 rootless
```

`tests/run.sh` / artifact harness 为历史独立 iOS 进程回归，不是本版已运行测试，不是设备验收。

---

# 历史开发记录（以下版本和测试不代表 0.1.5 验证结果）

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
