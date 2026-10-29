# 本文件不是权威记录

权威记录是 discussion_planning/2026-10-28-unit-state-flags.md 里的 2026-10-30 用户原话。目标按那份文件，不按本提示词。

# Unit 场景重构

日期：2026-10-30。已讨论并同意，尚未改代码。执行细节可参考本文件，权威边界以 2026-10-28 记录为准。

上一份判据讨论见 [2026-10-28 Unit 状态判据](2026-10-28-unit-state-flags.md)。本文件收紧边界：按功能拆，不按行数拆；`Unit` 不变成 `QObject`。

## 结论

`Unit` 继续是可拷贝的领域对象。撤销快照和阵列克隆都依赖拷贝，所以它不能继承 `QObject`。

信号放在只管当前活对象的场景对象上，暂名 `UnitDocument`。信号载荷用 `QUuid`，不用 `Unit*`。阵列重建后旧指针会悬空。`Unit` 上只放会改数据或回答判据的方法；改完由文档发信号。

## 收到 Unit 上的功能

这些是领域数据或能由领域数据算出来的判据：

- 显隐和锁定。这是用户意图，现在在 `OCCTWidget::m_unit_visibility` 和 `m_unit_locks`。派生副本的 UUID 会变，仍用现有阵列路径记住，不另造身份。
- `presentation_hidden` 的唯一算法。母本还有跟随副本时隐藏网格。只由 `refresh_derived_state()` 写入。阵列重建、取消跟随、删除阵列层、读档后调用。
- 姿态和方向读写。位置、方向，以及 Vector、Pitch/Yaw、Target 三套各自的数据，都走同一组函数。左下角、手柄和阵列传播都调用它。
- 阵列传播停在哪里。整颗脱离看 `follows_array`。单个属性停住看已有的 `override_fields`。不要新加“在这一层截断”的 `bool`。位置、方向、物性可以各自停。
- 身份判断：母本、派生副本、已脱离子喷注器。替换各处手写的 `is_array_child && follows_array`。

能由现有字段算出来的，不另存一个会过期的 `bool`。

## 留在视图里的功能

- `AIS` 的 `Display`、`Erase`、`Remove`、拾取、本地坐标轴和阵列参考几何的绘制。
- 当前选中、手柄正在拖、OCCT 上下文里是否已经显示。
- 对象树、面板、撤销栈、工程文件读写、设置。

锁定只禁止手柄和编辑，不把网格一起拿掉。`presentation_hidden` 不写进工程文件，读档后重算。

## 信号

不要再靠笼统的 `unit_data_updated(Unit*)` 驱动所有界面。场景对象按改动种类发信号：

- `poseChanged(QUuid)`
- `directionChanged(QUuid)`
- `followChanged(QUuid)`
- `presentationChanged(QUuid)`
- `structureChanged()`

视图只在姿态或显隐变化时改网格和坐标轴。对象树只在结构或颜色变化时刷新。数值框只在当前选中对象的姿态或方向变化时更新。

## 文件边界

按职责拆，不按行数拆：

- `Unit` 的姿态、阵列传播、派生判据各自成块。
- `OCCTWidget` 只留视口、拾取和手柄。
- `MainWindow` 只留面板和菜单。
- `UnitDocument` 持有活的 `shared_ptr<Unit>`，负责发上述信号。

## 第一刀

只换写入点和通知方式，交互行为保持不变：

1. 把派生判据收成 `Unit` 上的一个刷新函数。
2. 显示和擦除只读用户显隐、锁定和派生缓存。
3. 用分类信号替换对应的 `unit_data_updated` 刷新。
4. 不搬 `AIS` 句柄，不拆整个 `OCCTWidget`，不改阵列几何规则。

提示分级的未提交改动不要混进这次重构。

## 执行提示词

在 `D:\Git\dpm_manager` 按 `discussion_planning/2026-10-30-unit-document-refactor.md` 做第一刀。不要覆盖或回滚无关改动，未经要求不要提交或推送。

`Unit` 保持可拷贝，不要继承 `QObject`。增加场景对象持有活单元并发信号，载荷只用 `QUuid`。信号分为 `poseChanged`、`directionChanged`、`followChanged`、`presentationChanged`、`structureChanged`。

把显隐、锁定和 `presentation_hidden` 的计算收进 `Unit` 的领域方法。`presentation_hidden` 只有一个写入点，读档后重算，不进工程文件。阵列整颗脱离继续用 `follows_array`，单属性停用现有 `override_fields`，不要新增截断布尔。身份判断替换手写的 `is_array_child && follows_array`。

`OCCTWidget` 只消费这些结果来显示、擦除、拾取和手柄。锁定不隐藏网格。`MainWindow` 的树和数值框只连接对应信号。不要按行数拆文件，不要在这一刀移动 `AIS` 句柄，不要改变现有交互。

完成后构建 `build/codex_msvc142_release` 的 `dpm_manager`。测试阶段可以关闭正在运行的程序。改了 `mainwindow.h` 时确认 `src/app/main.cpp` 一并重编。
