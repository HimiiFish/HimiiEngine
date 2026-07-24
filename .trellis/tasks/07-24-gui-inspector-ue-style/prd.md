# GUI Inspector 统一样式（参考 UE 分区）

## 接受标准

- [x] Canvas：Render/Scaler 分区；Reference Resolution 无轴色 Vec2；Controls 化
- [x] Canvas 根 Rect Transform：只读 Driver + Resolved Size
- [x] 子节点 Rect Transform：Anchors/Transform 分区；Rotation 走 PropertyRow
- [x] Image / Text / Button：Appearance/Content/Style/Alignment 等分区 + Controls
- [x] `DrawMultilineTextControl` 加入 InspectorControls
- [x] product-ux：轴色仅空间量；GUI 分区约定

## 非目标

- Button 每状态折叠 Style（UE Brush 级）
- 自定义字体资源选择器
