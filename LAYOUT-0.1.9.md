# 0.1.9：空白卡片与面板位置修复候选

基线：`feature/0.1.8-contained`，`4dfbe7a41dd06971308367577211c0ce539cef7b`。新分支 `feature/0.1.9-layout`，仅向 0.1.8 分支提出堆叠 PR；不改 main、不覆盖旧包。本轮不安装、不注销、不清理真实通知。

## 用户反馈与证据边界

用户在 iOS17 + Relaxin/RootHide 确认 0.1.8 能隐藏原生列表。真实截图分类计数为全部9、QQ7、Minis2；面板贴近屏幕上方、卡片区域完全空白、底部提示原生已隐藏。分类计数来自 group.count，并不能证明卡片已创建或已可见。截图不是 0.1.9 的运行验证。

### 可从 0.1.8 源码直接确认的问题

1. `Tweak.xm/updatePanelGeometry` 把面板高度叠加给 **系统通知 owner.additionalSafeAreaInsets.top**，同时用 owner.safeAreaLayoutGuide.topAnchor 减去相同高度定位面板。两者抵消，面板仍贴在旧安全区顶部，不会自然下降到时钟下方；系统后代却继承了新增的大顶部安全区。该操作可能连带改变原生时钟/列表布局，因此本版完全停止修改系统 additionalSafeAreaInsets，而不是改动时钟视图。
2. `IDDigestUI.inc` 内卡片 UIScrollView 未设置 contentInsetAdjustmentBehavior，默认 Automatic，却处在上述被扩大安全区的层级内。其内容仍由 contentLayoutGuide 定位，故 UIKit 可以为卡片滚动内容再加很大的 adjustedContentInset.top，把占位卡片推到可视区域之外；卡片区和 footer 本身仍有正常 frame，符合截图形态。原版 outer stack 的 content/frame layout-guide 宽度链已经存在，不能凭空说原版“缺了宽度约束”。**本轮没有该设备 adjustedContentInset、子视图 frame 的运行时抓取；自动安全区耦合是代码明确存在的缺陷及可解释的机制，不冒称已证明截图唯一根因。** 新版对分类和卡片滚动区均明确 Never，卡片 contentInset 为零，消除此机制。
3. `refresh` 新建 overlay 后并不无条件 render，而沿用旧协调器缓存判断 unchanged/revision/selection；视图被重建时数据缓存不代表新视图已画出卡片。新版在 addSubview/约束/didMove/inserted 完成后强制 render，并在真实 builtCardCount 与模型数量不同的时候补建。
4. `render` 原先先 updateNativeListVisibility，再 showGroups/showCards。隐藏守卫只验证空 cardScroll 至少44×44，加上模型 cards.count；即便零卡片子视图也可隐藏原生。cardsRendered 同样误报模型数。新版先建卡片、测量及完成布局，最后校验并隐藏；模型数不再充当绘制证据。
5. `sizeThatFits` 原先固定 `category+300`（最多540/owner高度68%），不看实际卡片高度，造成少通知或空卡片时也有大片空区。新版按指定宽度 required fitting 测量 cardStack 实际高度，叠加 footer，再按用户上限和窗口实际余量取小。

## 本版布局与回退实现

- 卡片外层明确 translatesAutoresizingMaskIntoConstraints=NO、最小64点高；内部垂直 stack 的四边 padding 与父卡片约束完整；labels 明确参与 Auto Layout。保留已有 contentLayoutGuide 四边与 frameLayoutGuide 等宽链，强制布局后再判断。
- builtCardCount 是实际 addArrangedSubview 的卡片数，dropCards 同步归零；cardsRendered/cardsRedacted 改为实际创建数量。新增 cardsLayoutReady 单独报告几何就绪，不把创建数量说成屏幕已验证绘制。
- 原生隐藏增加：builtCardCount 与当前模型数一致、渲染快照一致、每张卡片尺寸至少44×44、卡内至少一个非空实际文字标签有正尺寸、至少一张真实卡片与滚动可视区有 >=44×22 的相交区域。空间不足/空壳/未完成布局就恢复原生；保留精确类/方法签名、独立 sibling、所有权 token、可靠枚举和总范围160上限守卫。
- 坐标从实际 UIWindow 安全区转换到 owner，再与 owner.bounds 取交集，按新纯 C 生产函数 IDLayoutPanel 限界；无 window 时保守使用 owner 的当前安全区。旋转和布局变化重新计算，底部保留110点，极小空间不隐藏原生；不重新挂载系统卡片，不遍历或修改时钟/相机/手电筒。
- **不声称实现了已验证的锁屏时钟自动避让。** UIKit 安全区不包括锁屏时钟；默认向下220点是保守位置预留，系统小版本、壁纸时钟及其他插件布局仍可能不同。底部110点也是几何预留，非已读取快捷按钮 frame 的精确避让。用户可手动调节；锁屏与通知中心共用设置，没有猜测场景区分。

## 用户设置

设置 → 灵屿通知 → 面板位置与大小（单位：UIKit 点，不是截图像素）：

| 设置 | 默认 | 可调范围 | 作用 |
|---|---:|---:|---|
| 顶部向下偏移 | 220 | 0–420，10点步长 | 从窗口安全区/owner交集顶部向下；空间不够会限界 |
| 左右边距 | 8 | 8–64，2点步长 | 两边等量缩进，调整横向宽度；不支持任意拖拽 |
| 面板最大高度 | 360 | 240–600，10点步长 | 上限而非固定高度；少卡片自动缩小 |
| 恢复默认位置与大小 | — | 即时可逆 | 只重置以上三项，不改置顶、筛选或通知 |

滑块有界、拒绝 NaN/Infinity，约120ms合并通知；拖动时不重建设置单元格，无逐次内容日志。空间受限时压缩分类可视区并在分类内部滚动，给卡片保留空间。若时间被挡，增大顶部偏移；若太大，降低最大高度。最终位置仍受窗口高度限制。

## 正文预览尚未解决（不是空白修复的同义词）

0.1.8 的 IDContainedPreviewAllowed 始终 fail-closed；即便布局正确，也只应该出现应用名/图标/相对时间及“通知内容已隐藏”，不是 QQ 正文。这一限制不解释“整个区域连占位都没有”；本版修复的是布局与隐藏就绪保护，**仍未实现真实标题/副标题/消息正文预览、点按打开、快捷回复或展开**。

复核 iOS17 运行时头文件：NCNotificationOptions.contentPreviewSetting 只有整型声明，无可证明的有效逐条策略/枚举；NCNotificationViewController 与 NCNotificationShortLookViewController 有 `_updateLookView:withTitleFromProvidedStaticContent:`、`_updateWithProvidedStaticContent` 和 `notificationContentViewHidden`，但方法声明不证明 providedStaticContent 已依照每App Never/锁屏认证最终脱敏，也没有完整批准字符串回调/失效时序的可验证契约。仓库固定头文件树 `d1d960dfaa4107765dd7fcf891e4967c0930d5fd` 未提供对应 NCNotificationStaticContentProviding/Delegate 协议定义；直接按 framework 路径查询协议文件也返回404。没有据此新增未验证 Hook、猜枚举、把解锁当权限、抓取隐藏原生正文或重挂载系统控制器。原始证据来源继续见 CONTAINED-0.1.8.md。

要查看当前真实消息，请关闭“隐藏原生通知列表”，恢复系统正文/回复入口。元数据占位与布局状态在 footer 明示，标题/正文不进入网络、日志或偏好；锁定/受保护数据失效继续清空所有标签和AX字符串。

## 验证范围

便携源码/配置契约、隐私/精确原生隐藏契约、391项 contained 谓词、Experience 排序/布局、Clear 资格/确认/快照逻辑以及新 IDLayoutPanel 173269 个断言/组合均执行。新契约直接检查 Never/挂载后render/先布局后hide/真实子视图守卫/不修改系统安全区，旧版不满足这些回归条件。CI继续 macOS14、iOS16.5 SDK、两套 Theos 和 classic linker；双 scheme 各编译 arm64/arm64e 并严格检查最终DEB、代码签名页、准确载荷白名单和新功能字符串。

**便携几何和源码契约不等于 UIKit Auto Layout 实测。macOS构建不等于 iOS17 + Relaxin 真机运行通过。** 未执行 tests/run.sh 的设备原生 Regression/Integration 或真实锁屏/时钟/大字/多插件验收，不伪报这些历史测试为本轮通过。最终CI链接、提交和包哈希记录在交付文件中。
