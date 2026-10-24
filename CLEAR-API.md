# 0.1.7 清理 API 取证与安全边界

## 固定来源

iOS 17 runtime headers: `MTACS/iOS-17-Runtime-Headers` commit `d1d960dfaa4107765dd7fcf891e4967c0930d5fd`。

- [CSCombinedListViewController.h](https://github.com/MTACS/iOS-17-Runtime-Headers/blob/d1d960dfaa4107765dd7fcf891e4967c0930d5fd/PrivateFrameworks/CoverSheet.framework/CSCombinedListViewController.h)：列表委托明确声明 `- (void)notificationStructuredListViewController:(id)controller requestsClearingNotificationRequests:(id)requests;`，以及返回 `bool` 的 `notificationStructuredListViewController:shouldAllowInteractionsForNotificationRequest:`。本版只向 owner 当前 delegate 发送该精确请求列表入口，两个参数均是对象，返回 void；不会调用以 section、日期或 All 为范围的 API。
- [NCNotificationRequest.h](https://github.com/MTACS/iOS-17-Runtime-Headers/blob/d1d960dfaa4107765dd7fcf891e4967c0930d5fd/PrivateFrameworks/UserNotificationsKit.framework/NCNotificationRequest.h)：`isCriticalAlert` 返回 bool；`sectionIdentifier`、`notificationIdentifier`、`uuid`、`timestamp`、`clearAction`、`options` 为对象 getter。
- [NCNotificationOptions.h](https://github.com/MTACS/iOS-17-Runtime-Headers/blob/d1d960dfaa4107765dd7fcf891e4967c0930d5fd/PrivateFrameworks/UserNotificationsKit.framework/NCNotificationOptions.h)：`hideClearActionInList` 返回 bool，`lockScreenPersistence` 返回 unsigned long long。
- [CSCoverSheetViewController.h](https://github.com/MTACS/iOS-17-Runtime-Headers/blob/d1d960dfaa4107765dd7fcf891e4967c0930d5fd/PrivateFrameworks/CoverSheet.framework/CSCoverSheetViewController.h)：`isAuthenticated` 返回 bool。必须从当前 owner 的有限父控制器链找到该类型并获得 true；否则停用，不绕过锁屏，不自行弹认证流程。
- [CSNotificationDispatcher.h](https://github.com/MTACS/iOS-17-Runtime-Headers/blob/d1d960dfaa4107765dd7fcf891e4967c0930d5fd/PrivateFrameworks/CoverSheet.framework/CSNotificationDispatcher.h)：系统下游有 `destination:clearNotificationRequests:`（void、两个对象参数），印证系统具有按 request 集合清理的原生路径。本版不绕过列表委托直接取得 dispatcher。
- 辅助参考：[Baw-Appie/Axon AXNManager.m](https://github.com/Baw-Appie/Axon/blob/23716151aa5b4c8cd9f466eb1d6d5c539e2a9291/Tweak/AXNManager.m)，`clearAll:` 使用 `allRequestsForBundleIdentifier:` 的数组交给原生 dispatcher。该项目是较旧接口实例，不当作 iOS17 selector 或行为认证，也不采用其清空本地缓存作为成功反馈的做法。

这些是公开的真实 runtime 声明与开源用法，不是根据 selector 名字臆造 ABI。每次调用前仍核对实际运行时参数数目/编码，getter 对象为 `@`、bool 为 `B`、unsigned long long 为 `Q`、清理返回值为 `v`。头文件中的 id 不提供集合元素的静态类型保证；本版传递原始 request 对象组成的不可变 NSArray，遵循原生列表委托的 requests 集合契约，但未在用户系统小版本进行行为验证。

## 实际实现

- 点击垃圾桶先建立当前应用/全部应用的**符合资格快照**，两个范围始终确认。确认文本给出作用范围和快照准确条数；没有“免确认”选项。
- 不读取正文。以 sectionIdentifier、notificationIdentifier、NSUUID、timestamp 的副本建立内存身份，同时强持有原始 request。确认时要求原始对象仍存在、四字段不变、当前同 key 恰好一个、范围/owner delegate/master/window/设置版本/generation 未变。相同 ID 的替换对象、重复身份、变更通知都不清理。新到通知只用于当前存在性校验，不加入提交列表。
- 请求必须提供可调用的 critical 检测；必须非 critical；有 clearAction；options 有 hideClearActionInList 和 lockScreenPersistence 检测、前者 false、后者为 0；原生 shouldAllowInteractions 为 true。非零 persistence 全部保守跳过，不臆测其枚举语义；检测缺失也跳过。无任何符合项时按钮停用并解释。
- 全局能力、完整枚举或已解锁证明缺失时按钮停用。全部只指可靠枚举到的、快照合格的原生通知，不保证覆盖系统不暴露的隐藏存储。只使用 master allNotificationRequests 或每个 section allNotificationRequests 的完整集合，不把 filtered 列表冒充完整集合。
- 提交只一个精确原生 requests selector，没 clear-all/section fallback、数据库操作、计时清理、自动清理、强制受保护请求移除或用筛选假装清理。
- 筛选、生命周期、设置/恢复与上下文变更取消待确认状态。主线程单次门控阻止重复点击/重复确认；验证完成之前不允许第二次清理。异步块 weak 捕获 coordinator，快照不持久化，不记录通知内容/身份。
- void 返回不表示成功。一次提交后 0.3 秒只读复核，仍存在时再观察 0.6、1.5 秒，最多三次且不重发清理。反馈“已不在列表/仍在”，不把自然消失冒称本功能因果成功。无法枚举或异常明确结果未知，不自动重试。失去当前界面则不向其他界面广播结果。

## 验证边界

生产共享纯 C 资格、确认门控、快照 membership 实际编译执行；包括 512 组资格组合、过期确认、双击、取消、范围/上下文不匹配、确认后再确认、新到/消失/替换/变为受保护请求。Objective-C 的 UIKit 呈现、真实 owner/delegate 路由与通知异步清理仍需真机验收；源码守卫不是这些运行行为的替代。开发测试不调用任何真实通知清理 API、不安装、不注销，不改动 0.1.5/0.1.6 产物。
