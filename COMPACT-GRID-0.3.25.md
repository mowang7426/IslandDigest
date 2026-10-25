# 0.3.25 · 紧凑分类网格

基线 `e99086ae60bf657405b6ae97ff849b76e4e84ae5`；沿用独立分支 `feature/0.3.25-compact-category-grid`，PR base 为 `feature/0.3.24-transition-critical-path`。不合并 main、不创建 Release、不安装或注销、不操作真实通知，不恢复 0.3.4。

## 范围与实际尺寸（普通字体，pt）

仅改展开面板分类区及相关设置、版本、回归。保留 .24 transition critical path、普通隐藏 retained-card readiness、立即擦除隐私内容、80×44 透明折叠胶囊、.23 无常驻 footer。新增 `tests/check_compact_grid.py` 逐字节比较所有非目标生产模块和 .24 基线，Tweak 只容许版本标记及分类默认值差异。

| 项目 | .24 | .25 |
|---|---:|---:|
| 迷你分类按钮真实点击高度 | 44 | 44（动态字体上限仍 80） |
| 迷你分类底色视觉高度 | 36（上下各 4） | 32（上下各 6） |
| 左右底色内缩 / 内容留白 | 4 / 5 | 4 / 5 |
| 迷你底色圆角 | 11 | 9 |
| 网格图标 | 18 | 16 |
| 网格名称基础字体 | 12 | 11（UIFontMetrics，最高 22） |
| 网格数字字体 | 11 | 10 |
| 网格真实按钮间行距 / 列距 | 8 / 6 | 4 / 6 |
| 单元外边距 | 每侧 8 | 每侧 8 |
| 迷你 3 行点击区域高度 | 148 | 140 |
| 含原有 66pt 标题/提示的 3 行分类区 | 214 | 206 |

44pt 真实点击高度不能继续缩；缩小的是非交互底色和内容。标准密度基础高度 52pt、无内缩底色保持；网格图标/字号/行距同样更新。横滑旧选项保留但不作为本次方案，不更改折叠胶囊、卡片或其他区的视觉尺寸。

## 列数与已有设置

- 新缺省是 `LayoutMode=grid`、`GridColumns=auto`、`CategoryStyle=mini`；不向偏好文件写入默认值。
- 自动普通字号：面板宽度低于 650pt 最多 4 列，650pt 起最多 5 列。扣两侧共 16pt 与 6pt 列间距后，每单元至少 82pt；362pt 是 4 列起点，320/359pt 为 3 列，374/390pt 为 4 列，768pt 为 5 列。
- 例如面板宽 374pt：原自动 3 列单元约 115.33pt，现自动 4 列为 85pt；390pt 则约 120.67→89pt。
- 已保存的显式 `two` / `three` 不重置、不偷偷升列。374pt 的显式三列仍约 115.33pt，但视觉高度、图标、字号和行距立即紧凑，不是只改新用户默认值。304pt 的三列每格 92pt，现在也容许三列。
- textScale >1.35 时最低格宽 128pt，>1.9 时 210pt；空间不足逐列退至 1 列。普通 390pt 自动为 4 列，1.6 倍为 2 列，2.3 倍为 1 列。
- 已保存的 `LayoutMode=compact` / `CategoryStyle=standard` 同样不强制重写；需要本次紧凑网格时在设置选择“紧凑多行网格”及“迷你”，想要更多列可选“自动”。既有显式网格列数不用重选即可获得尺寸变化。
- 最多可见 3 行，按 owner 高度的 45% 预算缩减；66pt 标题/提示及 4pt 行距与实际 layout 统一，超出纵向内部滚动，不横滑替代网格。

## 交互与安全保持

名称优先尾截断，数量 99+ 和图钉抗压缩，VoiceOver 保留完整名称/实际数量、选中 trait 与置顶自定动作；至少 44pt 高按钮、点按筛选、长按置顶/取消且取消抬手筛选、选中状态、滚动位置夹紧均保持。完整枚举/卡片分页和异常回退模块无改动。静态/纯 C 测试不等于 UIKit 约束或设备 AX 行为实测。

## 回归与交付契约

本地 Linux 执行 15 组 Python 源码契约 + 15 组共享生产 C 测试，共 30 组成功。新增自动列 361/362/649/650 边界、显式两/三列保持、动态大字、32/44pt 分离、图标/图钉/99+ 固定空间及真实行距预算回归。原生 Foundation 夹具仅在 macOS CI 执行；本地尝试编译 transition 夹具因 Linux legacy Objective-C runtime 不支持 ARC 而失败，不计原生通过。

最终提交必须经 macOS-14 的 roothide/rootless 两任务：全部源码/C/Foundation 回归、iOS16.5 SDK 双 Mach-O 架构 arm64+arm64e 编译、严格 deb 检查、artifact 上传。交付时记录最终 commit/PR/run；逐 artifact 核对 workflow_run.head_sha、远端 ZIP SHA-256 digest，下载原始 ZIP 后严格执行 `tests/check_package.py <deb> 0.3.25 <scheme>`，生成 `SHA256SUMS` 与 `verification.json`。此文不预先声称尚未完成的 CI 结果；最终结果以交付验包清单为准。

16.2 Dopamine（RootHide）与 17 Relaxin（RootHide）都选 `iphoneos-arm64e` deb；`iphoneos-arm64` 是普通 rootless 环境的另一个包。deb 架构标签不等于包内 Mach-O 仅单架构。

未进行真机视觉/Auto Layout/VoiceOver/私有 API 或真实通知行为验收；没有安装、注销或触碰通知。设备后续应手动检查短/长名、99+、置顶、大字降列、旋转、网格纵滚、筛选及卡片分页，并观察隐藏故障是否安全保留原生列表。
