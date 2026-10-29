# Unit 状态判据

日期：2026-10-28。重构按本文件执行。

## 结论

同步、隐藏、跟随、独立这些行为应该收成少量明确判据，调用处只问判据，不再各自重算。判据可以放在 `Unit` 上，但要分成三类。能由现有字段算出来的，不要再存一个会过期的 `bool`。

现在的问题不是缺标志，而是同一件事有多份真相：

- 用户显隐在 `OCCTWidget::m_unit_visibility`，另有 `UnitDisplayState`。
- 锁定在 `m_unit_locks`。
- 阵列跟随是 `follows_array`。
- 母本网格隐藏是运行时的 `presentation_hidden`，由“还有跟随副本”推出来。
- 方向模式、目标作用域在 `Injector` 上，阵列展开再解释一次。

## 三类判据

| 种类 | 例子 | 存在哪 | 能否进工程文件 |
| --- | --- | --- | --- |
| 用户意图 | 显隐、锁定、`follows_array`、目标作用域 | `Unit` 或已有的 `UnitDisplayState` | 能 |
| 派生缓存 | `presentation_hidden`、是否被副本代表 | `Unit`，只由一个写入函数更新 | 不能，读档后重算 |
| 视图瞬时状态 | 当前选中、手柄正在拖、OCCT 是否已 `Display` | 留在 `OCCTWidget` | 不能 |

用户意图回答“用户要什么”。派生缓存回答“场景现在必须怎样”。两者不要混成一个 `hidden`。

母本就是例子：树上仍然可见，网格必须拿掉。`m_unit_visibility == true` 且 `presentation_hidden == true` 可以同时成立。

## 建议留在 Unit 上的判据

已有、继续当用户意图：

- `is_array_child`
- `follows_array`
- `follows_parent_transform`
- `has_array_spec` / `has_fill_spec`

已有、只当派生缓存：

- `presentation_hidden`。唯一写入点是阵列成员变化之后的同步函数。选择、坐标轴、预览都只读它。

不建议新加的永久布尔：

- “正在被手柄拖着”。拖拽结束就没了。
- “这个网格当前在 OCCT 上下文里”。那是上下文的状态，不是模型。
- “方向已经和目标对齐”。这是几何结果，应由目标点和位置算出。

可以考虑以后补一个派生缓存，而不是手写布尔：

- `represented_by_following_copies`。真值等于 `presentation_hidden` 的来源。有了它就不要再在每个调用点写 `has_array_spec && 存在 follows_array 的子节点`。

## 复用入口

不要为显隐、拾取、坐标轴各写一套条件。只留两个函数：

1. `refresh_unit_derived_flags(Unit &)`
   - 输入是阵列子节点和用户意图。
   - 输出只改派生缓存，例如 `presentation_hidden`。
   - 阵列重建、取消 follow、删除阵列层、读档之后调用。
2. `apply_unit_presentation(Unit &)`
   - 只读用户显隐、锁定和派生缓存。
   - 决定 `Display`、`Remove`、停用拾取、本地坐标轴。
   - 选择和手柄预览不再自己判断母本。

伪代码：

```cpp
void refresh_unit_derived_flags(Unit &unit)
{
    unit.presentation_hidden = unit_has_following_copy(unit);
}

void apply_unit_presentation(Unit &unit)
{
    if (!user_visible(unit) || unit.presentation_hidden)
    {
        remove_mesh_and_axes(unit);
        return;
    }
    display_mesh_and_axes(unit);
    if (user_locked(unit))
    {
        detach_gizmo_and_editing(unit);
    }
}
```

锁定是否连网格一起拿掉，要和现在的“锁定不能挂手柄，但网格还在”保持一致。所以上面的 `user_locked` 只应禁止手柄和编辑，不应塞进隐藏条件。这是复用时必须分开的两条规则。

Target 模式同理。不要加 `aims_at_target`。复用一个函数：

```cpp
bool target_hitpoint_follows_copy(const Injector &injector);
QVector3D resolved_aim_point(const Injector &injector, const UnitArraySpec &spec);
```

阵列平移、旋转、镜像、填充都调用它。Target Hitpoint 返回固定目标点；Vector 模式继续按作用域跟随副本。

## 不在这一步做的事

- 不把 `OCCTWidget` 按文件行数切开。
- 不把派生缓存写进 `project_session`。
- 不把 `AIS` 句柄从 `Unit` 里拆走。那是下一阶段，和布尔判据无关。

## 2026-10-30 用户原话

后续重构以本文件为准。用户要求原样记下：

> 拆东西坚决不能按行数拆，一定要按功能拆，此外，你看看现在还有哪些功能值得集成到unit类里面的？请记住巧用qt信号槽机制

因此：只按功能拆文件；能收进 Unit 的领域行为收进 Unit；视图和界面用 Qt 信号槽接这些变化，不按行数切开 OCCTWidget。

## 2026-10-30 模块化要求

用户原话：

> globalfunction那是当时我想写设置用的，你要是觉得没有就删了吧，这次的改造要求将程序改的尽量模块化，可复用化，请计入文件

已删除未使用的 src/common/global_functions.h，并从 CMakeLists.txt 去掉它。设置已由 unit_system 和 app_config 承担。

这次改造以可复用模块为准：按功能拆，不按行数拆。能独立调用的计算、阵列、单位、读写留在模型、common、io。界面只通过 Qt 信号槽使用这些模块。
