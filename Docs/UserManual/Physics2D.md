# 2D 物理系统 (Physics 2D)

HimiiEngine 内置 Box2D 物理引擎。

## 启用物理

要让实体受物理系统控制，需要添加以下组件：

### 1. Rigidbody 2D
定义实体的物理属性（质量、速度、阻尼等）。
*   **Body Type**:
    *   `Static`: 不会移动（如地面）。
    *   `Dynamic`: 受力影响（如玩家、箱子）。
    *   `Kinematic`: 仅由脚本控制速度，不完全受物理模拟影响。

### 2. Box Collider 2D
定义实体的碰撞形状（矩形）。
*   **Density**: 密度，影响质量。
*   **Friction**: 摩擦力。
*   **Restitution**: 弹性（反弹系数）。
*   **Restitution Threshold**: 弹性阈值。

## 物理控制

建议在 `OnUpdate` 中通过修改位置或在物理更新阶段施加力来控制物体。
（API 完善中...）
