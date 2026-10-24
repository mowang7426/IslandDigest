# 0.3.16 折叠诊断增强版

## 目的与边界
基于干净的 0.3.15 HEAD `5bdc4babe58f6cc72c4b5c23514430e60d5dc5a1`。
`waiting-for-user-pan-began` 是初始化值，旧版没有记录 Possible/Changed/Ended，不能据此断言完全没有 raw callback。本版保留唯一启动周期的 `_handlePresentGesture:` Began 判定及原有条件，不凭 selector 名称改触发逻辑。诊断增强不等于已证实修复。

仍保留稳定刷新、隐私锁定清空、手动展开后同周期不重折叠，以及通知到达/锁屏唤醒/owner 重建不创建下拉周期。不改变原生隐藏、清理或通知操作权限，不引入已弃用的 0.3.4。

## 一次用户操作与截图
1. 用户自行选择正确环境包完成升级后，在设置→灵屿通知确认版本 0.3.16、启用、按应用分组、插件内卡片及“每次下拉先折叠”开启。不要为诊断改变隐藏或预览设置。
2. 从已解锁界面真正下拉一次通知中心，停留约 2 秒，观察是否胶囊；本轮不要手动展开、清理或打开通知。
3. 收起返回设置，等约 1 秒，点击页面顶部“刷新折叠诊断”，只截顶部中文摘要。不需要滚到底部长截图。若 raw=0，摘要即能分辨 helper/indirect 是否也为0；必要时下一轮才看旧详细诊断。
4. 版本/PID/新鲜度用于排除旧进程或旧快照。计数按本次 SpringBoard 进程累计，不是每次下拉自动归零；屏幕状态枚举 P/B/C/E/X/F 分别为 Possible/Began/Changed/Ended/Cancelled/Failed，0/1/2/3/4/5；未知或非主线程状态记 -1，详表单独累计 unknown。raw=0 时 pre/post 的零初始化值不表示真的收到 Possible。

## 如何定位
- raw=0、helper/indirect>0：系统走了其他已验证观测路径；不是立即把该 probe 改作折叠信号。
- raw>0、B=0：入口收到回调，但尚未在入口观察到 Began；详细 pre/post 计数可检查 orig 内状态迁移。最后状态不是整次手势所有状态。
- B>0、接受=0：查看独立最后拒绝（重复、开关、模式、screen-off 支持、参数或线程）。普通 Possible/Changed/Ended 是观测，不伪装成拒绝。
- 接受>0、消费=0：检查直用拥有者数及 directApply/beginAppearance/render 的跳过原因。
- 已消费：查看消费阶段、周期、匿名 owner、胶囊 hidden/alpha/window/height、panelHeight/constraintHeight。beginAppearance 是挂载前状态消费，不等于胶囊可见；directApply/render 的样本在实际布局后记录。消费包含应用展开状态，并非每次都代表折叠成功。
- 返回设置产生的 clearReason 与最后接受/拒绝分开存储，不覆盖判断依据。详细诊断保留每个阶段最近样本、最后消费以及退出清理前样本；后续新操作会更新最近样本，因此按上述一次测试流程截图。

## 观测与性能
- raw/helper/indirect 三条路径使用固定大小标量结构、互斥锁与单调事件号；显式从0初始化全部计数。记录 class 名，不调用对象 description；不记录内存地址、通知 request、正文或 app 标识。匿名来源/owner ID 仅在当前进程递增。
- raw入口统计位于原有条件之前，orig 后记录 post；每条路径有总次数、pre/post 状态直方图、是否识别器、主线程及 offMain 次数。非主线程不读取 UIKit state。
- Changed 观测不调用 IDStatus、不读取偏好、不写 plist、不派发每帧日志。关键非 Changed 事件和消费者最多保留一个1秒尾随 flush；原有 Began、结束和配置状态仍保留，退出前保存样本并 flush。只有固定数量最近样本，无无限日志缓冲区。实际帧率仍需设备测量。
- 两个新增 probe 仅在 SBCoverSheetSlidingViewController 经 void/arity/object-argument ABI 验证后安装，orig 各调用一次；绝不在 probe 启动或结束周期。没有 UIView/UIGestureRecognizer 全局 hook。
- 状态记录仍沿用原诊断 plist，不写用户偏好；详细设置输出为白名单字段。

## 固定真实头文件依据
- https://github.com/MTACS/iOS-17-Runtime-Headers/blob/5a141185d0adb16bf83e96edc565abab5ec5de53/PrivateFrameworks/SpringBoard.framework/SBCoverSheetSlidingViewController.h
  包含 `_handlePresentGesture:`、`_presentGestureBeganWithGestureRecognizer:`、`_handleIndirectPresentGesture:`。
- https://github.com/MTACS/iOS-17-Runtime-Headers/blob/5a141185d0adb16bf83e96edc565abab5ec5de53/PrivateFrameworks/SpringBoard.framework/SBCoverSheetSystemGesturesDelegate.h
  包含 presentGestureRecognizer/indirectPresentGestureRecognizer。本版没有额外调用或 hook delegate getter。

## 验证
- 本地全部既有 Python 源码合同、14套共享生产 C 测试；新增计数器用100000次 Changed、全状态、嵌套回调、未知/非主线程和分路隔离测试。
- macOS Actions 两个矩阵环境：既有生产枚举测试、新增直接 include 生产诊断模块的 Foundation 测试（识别器为 mock），实际 Theos arm64/arm64e tweak+prefs 编译及严格验包。
- 新模块测试验证 Changed 零持久化、probe 不启动周期、接受/拒绝经 clear 后保留、清前 live/compact、匿名来源与 plist 可序列化。它不是 UIKit 真机测试。
- 包检查强制0.3.16元数据、环境路径/install-name、双架构、签名页哈希、依赖、SpringBoard-only filter、精确文件白名单、PreferenceLoader/图标/面板版本和生产诊断符号。
- 不安装、不注销、不操作真实通知；不宣称设备下拉触发、最终视觉或性能已验证。实际 CI/包哈希以交付报告为准。
