# 2D 逐帧动画

Himii 的 2D 逐帧动画以 **`.anim` 资产** 组织：一个文件绑定一张图集（`AssetType: SpriteFrames`），内部包含 **多条命名动画**（如 `Idle`、`Run`）。运行时通过 **Sprite Animation** 组件选择当前动画名，由 **Sprite Renderer** 负责绘制。

适合 **已按格子切好的角色图集**：单文件管理站立、跑步、跳跃等多段序列，无需为每个动作单独建资产文件。

> **【配图占位】** `images/animation-editor-overview.png` — Animation Editor 整窗：左栏 Animations / Playback，右侧 Atlas Picker 与 Timeline。

---

## 核心概念

| 概念 | 说明 |
|------|------|
| **`.anim` 资产** | 磁盘上的动画文件，内含共享图集与多条命名序列 |
| **命名动画** | `Animations[]` 中每条有 `Name`（如 `Idle`）、FPS、Loop、帧列表 |
| **图集格子** | `AtlasGridCellSize` 与 `AtlasFrameCoordinates`（列, 行） |
| **Sprite Animation 组件** | 场景实体上引用 `.anim`，`Play("Run")` 切换播放 |
| **Animation Editor** | 可视化点选格子、编辑时间轴 |

首次上手建议先完成 [教程：2D 平台跳跃](Tutorial2DPlatformer.md)。

---

## 资产结构

### 新格式（推荐）

保存后 YAML 顶层为 `SpriteFrames`：

```yaml
AssetType: SpriteFrames
Handle: ...
AtlasTextureHandle: ...      # 图集纹理句柄
AtlasGridCellSize: 32        # 单格像素边长，与导出图集时一致
Animations:
  - Name: Idle
    FrameRate: 10
    LoopMode: Loop
    AtlasFrameCoordinates:
      - [0, 0]
      - [1, 0]
      - [2, 0]
  - Name: Run
    FrameRate: 12
    LoopMode: Loop
    AtlasFrameCoordinates:
      - [0, 2]
      - [1, 2]
      # ...
    Frames: []                 # 帧列表模式时填 AssetHandle，与图集模式二选一
```

每条 **命名动画** 独立拥有：

- **FrameRate**：默认播放帧率（组件可覆盖）
- **LoopMode**：`Loop` / `Once` / `PingPong`
- **AtlasFrameCoordinates**：图集格子 `(列, 行)` 的有序列表（主工作流）
- **Frames**：可选，拖入独立 Sprite/纹理句柄时使用（少用）

### 旧格式兼容

旧版 `AssetType: SpriteAnimation`（单条时间轴）在加载时自动迁移为 **一条名为 `default` 的动画**，播放与编辑行为与升级前一致。
  
> 打开旧资产后，左栏 Animations 中出现 `default`。
  
> 文本编辑器中展示 `SpriteFrames` 的 `Animations` 数组片段。

---

## 编辑器：Animation Editor

### 打开方式

- 菜单 **Window → Animation Editor**
- 在 **内容浏览器** 中 **双击** `.anim` 文件
 
> 内容浏览器中 `Idle.anim` / 图集 PNG 并列显示。

### 界面分区

| 区域 | 作用 |
|------|------|
| **Animations** | 命名动画列表：Add / Rename / Delete；选中项决定当前编辑上下文 |
| **Playback** | 当前动画的 **FPS**、**Loop Mode** |
| **Atlas Frames** | **Cell Size**、拖入图集、可选 Fill All Cells |
| **Atlas Picker** | 在图集预览上 **按播放顺序点格子** 组成时间轴 |
| **Timeline** | 已选帧缩略图序列；Move Left/Right 调整顺序 |

**重要**：Atlas Picker 与 Timeline 只修改 **当前在 Animations 列表中选中的** 那条动画（如 `Run`），不会影响 `Idle`。

> 左栏选中 `Run`，列表中 `Idle` 与 `Run` 均可见。

### 推荐工作流（已切图集）

1. **File → Open Animation** 或新建后 **拖入** 已切好的角色图集 PNG（**不会**自动 Slice 填满全图）。
2. 设置 **Cell Size (px)** 与美术导出一致（常见 16 / 32 / 48）。
3. 在 **Animations** 中 **Add** `Idle`、`Run` 等，选中一条。
4. 在 **Atlas Picker** 上 **从左到右、按动作顺序点击格子**；格子高亮表示已加入时间轴。
5. 勾选 **Click Replaces Selected** 时，点击格子会 **替换** Timeline 中当前选中帧，否则 **追加**。
6. 需要时使用 **Fill All Cells** 按网格顺序填满（适合整图循环特效，非角色主流程）。
7. **Save** 或 **Ctrl+S**（见下文「保存」）。
 
> 点选后 Atlas 上高亮格子 + Timeline 出现对应缩略图。

### 帧列表模式（少用）

在 **Frame List** 区域拖入 **Sprite** 或纹理，会切换到「独立帧句柄」模式并清空当前动画的图集帧。与图集点选 **互斥**，一般仅用于非图集素材。

### 预览

右上方 **Play / Stop** 与 **Preview FPS** 仅在编辑器内预览 **当前命名动画**，不影响场景资产直到保存。

### 保存与 Ctrl+S

| 操作 | 行为 |
|------|------|
| Animation Editor 内 **Save** | 写入当前 `.anim` 并导入到项目资源库 |
| 全局 **Ctrl+S**（保存工程） | 仅当 **已打开过磁盘上的 `.anim` 且内容有修改** 时才自动写入动画；**不会**为未保存的新动画弹出另存为 |
| 首次保存新动画 | 在 Animation Editor 用 **Save As** 指定路径，或先 Save As 再使用 Ctrl+S |

---

## 场景中的组件

### 必需组合

| 组件 | 是否必需 | 说明 |
|------|----------|------|
| **Sprite Renderer** | 是 | 负责绘制；无则看不见画面 |
| **Sprite Animation** | 是 | 引用 `.anim`，指定当前动画名与播放状态 |
| **Transform** | 是 | 位置 / 旋转 / 缩放 |

### Sprite Animation 属性

| 属性 | 说明 |
|------|------|
| **Animation** | 拖入 `.anim` 资产句柄 |
| **Current Animation** | 下拉选择命名动画（如 `Idle`、`Run`） |
| **Frame Rate** | 大于 0 时覆盖资产内该动画默认 FPS |
| **Playing** | 是否推进帧（运行时） |
| **Preview In Scene** | 编辑模式下在 Scene 视口预览（不勾选则仅 Play 时播放） |
| **Current Frame** | 编辑时手动 scrub 帧索引（需 Preview In Scene） |
  
> Inspector 中 Sprite Animation 全部字段可见。

### Sprite Renderer：水平翻转

左右朝向请使用 **Flip Horizontal**，**不要** 将 `Transform.Scale.x` 设为 `-1`（会导致碰撞与渲染异常）。脚本见 `SpriteRenderer.FlipHorizontal`。
 
> 同一场景中角色朝左、朝右各一张对比图。

---

## Loop Mode

| 模式 | 行为 |
|------|------|
| **Loop** | 播完最后一帧后回到第一帧 |
| **Once** | 播完后停在最后一帧并停止 **Playing** |
| **PingPong** | 正向播完再反向，往复 |

每条命名动画在资产内单独配置；组件 **Playing** 与 **Frame Rate** 可在实例上覆盖。

---

## C# 脚本 API

```csharp
using HimiiEngine;

public class PlayerController : Entity
{
    private SpriteAnimation _spriteAnimation;

    public override void OnCreate()
    {
        _spriteAnimation = GetComponent<SpriteAnimation>();
        _spriteAnimation?.Play("Idle");
    }

    public override void OnUpdate(float timestep)
    {
        if (_spriteAnimation == null) return;

        if (Input.IsKeyDown(KeyCode.D))
            _spriteAnimation.Play("Run");
        else
            _spriteAnimation.Play("Idle");
    }
}
```

| API | 说明 |
|-----|------|
| `Playing` | 是否播放 |
| `FrameRate` | 实例级帧率覆盖 |
| `CurrentAnimation` | 当前动画名（get/set） |
| `Play()` | 使用当前 `CurrentAnimation` 并重置帧 |
| `Play(string name)` | 切换动画名、重置帧并开始播放 |
| `Stop()` | 停止播放 |

完整组件列表见 [脚本 API - SpriteAnimation](ScriptingAPI.md#spriteanimation-组件)。

---

## 与物理 / 脚本配合

典型 2D 平台角色见 **[教程：2D 平台跳跃](Tutorial2DPlatformer.md)**（移动、跳跃、`[SerializeField]`、动画切换）。

---

## 常见问题

| 现象 | 原因与处理 |
|------|------------|
| 看不见角色 | 缺 **Sprite Renderer**；或动画无帧；或 `Animation` 未指定 |
| Ctrl+S 弹出保存 Animation | 已在编辑器打开未落盘的动画且误触保存；现逻辑已限制；请用 Animation Editor **Save As** 首次落盘 |
| Scale.x = -1 人物消失 | 改用 **Flip Horizontal**；勿用负缩放翻转 |
| 改 walk 却改了 idle | 未在 Animations 列表选中 `walk` 就点图集 |
| Play 不播 | 检查 **Playing**；运行时需要 **Play** 模式；编辑预览需 **Preview In Scene** |
| 动画名对不上 | `Play("名称")` 必须与资产内 `Name` 完全一致（区分大小写） |

---

## 相关文档

- [教程：2D 平台跳跃](Tutorial2DPlatformer.md)
- [编辑器与功能](EditorFeatures.md)
- [脚本 API](ScriptingAPI.md)
- [编辑器界面](EditorInterface.md)
- [2D 物理](Physics2D.md)
