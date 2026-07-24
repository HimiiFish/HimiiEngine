# Inspector 三列行：紧凑引用、Reset、行距

## Goal

把 Properties Inspector 属性行统一成 UE 风格三列骨架，并收紧 Properties 整窗行距。

## Requirements

- 所有可编辑属性行经 `DrawPropertyRow`：`标签 | 值 | Reset`。
- Reset 仅偏离默认时显示，图标为 `↺`；引用类 Reset = Clear 到 None。
- 右侧列只放 Reset；打开 Editor 放在引用名称框右侧（值区域内），并与双击共用回调。
- 引用槽改为紧凑布局：缩略图 + 限宽名称（约 max 200），去掉槽内大号 `X`。
- Properties 窗口成对调用 `BeginInspectorPropertiesStyle` / `EndInspectorPropertiesStyle`（约 50%/55% ItemSpacing.y / FramePadding.y）。
- Drawer 禁止为「打开 Editor」再单独画一整行 `DrawActionButtonRow`（Create New 等动作行可保留）。

## Acceptance Criteria

- [x] `InspectorControls` 各 `Draw*Control` / `DrawObjectReferenceField` 走三列行并支持可选 Reset。
- [x] Properties 整窗行距收紧；全局 ImGui Style 未改。
- [x] Tilemap / ParticleEmitter 等去掉独立 Open Editor 行，走引用旁按钮 / 双击。
- [x] SoundPlayer Volume 等关键字段 Reset 回到组件默认值（如 Volume=1）。
- [x] HimiiEditor 可编译通过。

## Notes

- ImGui 绘制逻辑只留在 `InspectorControls`（及 ActionButtonRow 回调内）。
