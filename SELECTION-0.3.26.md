# 0.3.26 分类点击有限优化

基线：`134718acfcfdc82037737f47d821b573c9668b00`（0.3.25）。独立分支 `feature/0.3.26-category-selection`，PR base `feature/0.3.25-compact-category-grid`。不合 main、不建 Release、不安装、不注销、不操作真实通知。

## 实际定位与改动

* `selectIdentifier:` 原来同步 `IDWritePreference(DesiredApp)`，其中包含 `CFPreferencesSynchronize`。现在立即更新会话选择和内存缓存，仅提交主队列邮箱；后台串行写入器处理磁盘。最多一个在途值和一个最新值，连续点击合并，不堆积每次点击的后台任务。
* 清空记忆、已消失分类清空也使用同一写入器；旧写结束后才允许最新写开始，旧写不能在最新写完成后反向覆盖。偏好重载在写入未完成时读取最新邮箱，关闭记忆即使无存活 owner 也提交清空。清空不携带通知内容。
* 仅可靠、已渲染、第一页、无清理反馈/开场/过渡且模型、选项、master 身份一致的内嵌 All→All 点击跳过重复 snapshot/filter/render。点击已选 App 仍切回 All；非零页仍重置；模型或隐私擦除导致状态失效时仍执行原路径。
* `showCards` 去掉父视图布局前的额外 `cardStack layoutIfNeeded`。仍标记 child dirty，并由已有父布局的 `layoutSubviews` 布局 cardScroll/cardStack，再计算滚动锚点；render 的 owner/panel 强制布局、readiness 与原生隐藏门控全部保留。
* 原生产 snapshot 缓存、generation/revision/master 失效校验和 `IDNotificationPageRequests(...snapshotByApp...)` 索引保持不变。没有声称原实现每次都全量枚举。

## 边界

这不是卡片分片重构：snapshot/card model 构造、可见卡片 UIKit 构建、原生 applyFilter 及必要布局仍在原同步路径，160 卡片上限未改。没有引入异步 UI 分类展示，因此没有新旧分类异步竞态；同步隐私擦除、动作身份/认证门控、分页与筛选语义不变。没有全局 hook、SplitView 改动或恢复 0.3.4。

保留 .25 紧凑网格和显式列偏好、.24 性能改动、胶囊周期、.23 无永久 footer、底部诊断与所有安全动作门控。后台偏好写仍沿用现有失败语义（不自动无限重试）；进程在持久化完成前被终止可能丢失最后选择。已经开始的旧值写入不能取消，但必定先于最新写入结束；不承诺跨进程原子事务。

无真机 trace，不能证明秒级卡顿根因或宣称彻底修复。实际去除的是点击栈内同步持久化和一个冗余初步布局，及限定范围的重复点击工作；真实 QQ/Minis 大列表延迟仍需两设备实测。

## 回归与验包方法

* `tests/check_selection.py`：严格限定 .25→.26 生产差异，保留 snapshot/index、render/filter/action/hooks/privacy 的字节身份；校验全部 DesiredApp 写入统一入口、无 owner 清空和 pending 缓存重载。
* `tests/run_selection_fixture.py`：macOS Foundation 执行真实 `IDDeferredSelection.inc`；阻塞第一笔磁盘替身期间提交 1000 次新选择后清空，再提交 1000 次选择最终 Minis；验证主线程零写、复制可变输入、背压、最新值、清空顺序。执行从生产 `selectIdentifier:` 提取的 All→All 判定，覆盖 19 个失效情形。
* 四种变异必须被击杀：取消输入复制、丢失 dirty 最新值、取消 pending 背压、错误跳过非零页重置。测试磁盘边界使用替身，不写设备偏好。
* 运行既有源码/C/Foundation 回归；历史 footer 基线只归一化精确的布局差异，历史 .25 网格契约保留，并由新增 .26 契约约束当前文件。
* macOS-14 CI 对 RootHide/rootless 真正编译，严格 `check_package.py` 已将 0.3.26 加入全部版本适用检查。以最终 HEAD 的成功 run 为准，核对 artifact `workflow_run.head_sha` 和下载 ZIP SHA-256 digest，再对每个下载 deb 执行严格检查。
* 交付目录 `/var/minis/shared/islanddigest-0.3.26-build/`，含 deb、`SHA256SUMS` 与 `verification.json`。RootHide 使用 Debian `iphoneos-arm64e` 包（适用用户 iOS 16.2 Dopamine RootHide、17 Relaxin RootHide），普通 rootless 使用 `iphoneos-arm64` 包；两者实际 Mach-O 都按现有工程构建 arm64+arm64e，不可只根据文件名猜测二进制切片。
