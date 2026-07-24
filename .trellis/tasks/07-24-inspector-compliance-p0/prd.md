# Inspector 合规 P0

## 目标

消除 Properties / 粒子编辑器中对原始 `AssetHandle` 的展示，并将 SoundPlayer、ParticleEmitter 相关 UI 迁到 `InspectorControls`。

## 接受标准

- [x] SoundPlayer：`DrawObjectReferenceField` + checkbox/float Controls；无 handle 数字
- [x] ParticleEmitter 组件抽屉：同上；创建/打开按钮走 `DrawActionButtonRow`
- [x] `AssignSoundAssetFromContentBrowserPayload` / `AssignParticleEmitterAssetFromContentBrowserPayload` 进入 `InspectorControls`
- [x] Particle Emitter Editor：去掉 `Handle: %llu`；贴图用文件名 + `DrawObjectReferenceField`；属性行使用 Controls

## 非目标（后续 P1）

- UIText / UIButton / Script 类名行
- AnimationEditor / ProjectSettings 全面收口
