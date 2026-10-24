# 0.1.8 实验性插件内元数据卡片：边界与隐私依据

本版基于 `feature/0.1.7-clear` 的 `7f508f0840a1ffc60ff7cd61c2986bb041f93feb`。不修改旧版包，不合并或推送 main。

## 实际支持与未完成部分

- 插件内模式默认开启。点击 QQ 等应用后，在同一个插件面板下方按真实 notification request 显示应用名称、图标、相对时间、逐条隐藏占位；按时间排序、去重，最多 160 条。不是假数据，也不是消息正文预览。
- **目前所有通知的标题、副标题、正文均隐藏，解锁后也不会显示正文。用户所需的“点击 QQ 后同一框下显示 QQ 信息”仅完成分组及元数据部分，完整消息内容目标尚未完成。** 不应把本版称为完整预览实现。
- 无点按打开应用、快捷回复或卡片展开；实际消息及原生交互请保留原生通知列表。应用胶囊/置顶、布局排序、确认式清理继承上一版的保护逻辑。
- 隐藏原生列表默认关闭；仅在插件内模式开启时可设置。开启会让原生正文/回复入口暂不可见，设置中有明确警告。关闭隐藏、退出界面、关闭插件/分组/插件内模式、错误恢复都会恢复保存的 hidden 状态；不移除、不重挂载、不清除通知。
- 只有已验证签名的 `masterListView` getter、**精确** `NCNotificationListView` 类型、独立且可见的 sibling overlay、可靠完整的枚举和非空卡片才可隐藏。达到 160 条上限（包括当前筛选外应用的总请求量）保留原生以访问较早通知。未知接口/截断枚举保持原生。
- 新协调器令牌防止已销毁协调器排队的异步恢复覆盖另一协调器；不从认证回调同步等待主线程。当前永不持有可见正文，异步清空不产生正文泄漏窗口。将来若实现真实预览，必须重新审计锁定瞬间的生命周期和隐私处理，而不能直接打开现有逻辑门。

## iOS 17 来源核查（非运行验证）

核查公开反汇编运行时头文件仓库 `MTACS/iOS-17-Runtime-Headers`，树对象 `d1d960dfaa4107765dd7fcf891e4967c0930d5fd`。原始头文件不是 Apple 对行为/隐私的公开契约，声明本身不足以证明有效权限。

- [NCNotificationOptions.h](https://github.com/MTACS/iOS-17-Runtime-Headers/blob/main/PrivateFrameworks/UserNotificationsKit.framework/NCNotificationOptions.h)：`contentPreviewSetting` 为 `unsigned long long`；没有可核验的枚举值与逐条合成策略证明。**未猜测枚举映射。**
- [NCNotificationRequest.h](https://github.com/MTACS/iOS-17-Runtime-Headers/blob/main/PrivateFrameworks/UserNotificationsKit.framework/NCNotificationRequest.h) 与 [NCNotificationContent.h](https://github.com/MTACS/iOS-17-Runtime-Headers/blob/main/PrivateFrameworks/UserNotificationsKit.framework/NCNotificationContent.h)：请求/内容模型有内容字段和 `hiddenPreviewsBodyPlaceholder`，但字段存在不是读取或显示的隐私授权。未读取该占位字符串，使用插件自己的通用占位。
- [BBSectionInfo.h](https://github.com/MTACS/iOS-17-Runtime-Headers/blob/main/PrivateFrameworks/BulletinBoard.framework/BBSectionInfo.h)：`contentPreviewSetting` 为 `long long`，有 `showsMessagePreview` 及与默认设置合成 section info 的接口；仍未证明逐条请求在当前锁屏上下文中的有效许可。不能用解锁或单个布尔值绕过 App 的 Never 设置。
- [NCNotificationViewController.h](https://github.com/MTACS/iOS-17-Runtime-Headers/blob/main/PrivateFrameworks/UserNotificationsUIKit.framework/NCNotificationViewController.h) 与 [NCNotificationShortLookViewController.h](https://github.com/MTACS/iOS-17-Runtime-Headers/blob/main/PrivateFrameworks/UserNotificationsUIKit.framework/NCNotificationShortLookViewController.h)：可见 `notificationContentViewHidden`、`viewForPreview`、通知请求设置等声明；不能证明它们是可复用的有效隐私许可，也未验证脱离原生拥有者后锁屏/认证更新和交互生命周期。故未复制原生 view、未强行重挂载短视图、未将视图可见性当权限。
- [NCNotificationStructuredListViewController.h](https://github.com/MTACS/iOS-17-Runtime-Headers/blob/main/PrivateFrameworks/UserNotificationsUIKit.framework/NCNotificationStructuredListViewController.h)：声明 `masterListView` 的类型和 `setDeviceAuthenticated:`。运行时还按实际方法签名/精确 class 检验；头文件不能代替 iOS17 Relaxin 真机验收。

结论：当前证据不足以安全启用完整消息预览，保留 fail-closed，而不是猜枚举或以已解锁代替用户设置。后续要完成真实消息需验证系统有效逐条预览策略，或在真机确认可复用且完整遵守系统隐私生命周期的原生渲染机制。

## 设置、诊断与验证

- `ContainedMode=YES`；`HideNativeList=NO`，关闭 ContainedMode 时隐藏开关禁用且运行时强制无效。设置即时发 Darwin 通知，无自动注销。
- Settings 白名单新增：`containedEnabled`、`hideNativeRequested`、`nativeHideSupported`、`nativeHidden`、`nativeHideReason`、`previewPolicyReason`、`cardsRendered`、`cardsRedacted`。计数为当前快照卡片数，不是已证明真机绘制成功的标志。
- `previewPolicyReason=effective-per-request-policy-unverified; fail-closed`。不将正文、标题、副标题、请求标识写入日志、偏好或网络；偏好延续仅本机应用名称/置顶信息。
- CI：macOS14 / Theos / iOS16.5 SDK / classic linker，roothide 与 rootless 两套实际编译；保留旧断言，新增 Contained.c 391 个生产谓词断言、check_contained.py、现有源契约/Clear/Experience 测试。严格包检验验证准确载荷白名单、版本、双架构、SHA-256 签名页、安装名、依赖和新诊断/隐私降级标记，没有 bypass。
- 未在 iOS SpringBoard 原生运行、安装、注销或清除任何用户通知。编译通过和包签名检查**不等于** iOS17 Relaxin 真机运行通过。

验收建议（用户自愿在设备测试，不由开发脚本执行）：默认原生保留；打开通知中心，点 QQ 查看逐条时间/隐藏占位；关闭插件内模式回退原生筛选；来新通知/应用切换/重复开关界面/旋转/大字/长列表；隐藏开关开启后关闭、退出、停用等逐一核验恢复；达到上限自动保留原生。当前版本不应以“显示消息正文”作为通过项。
