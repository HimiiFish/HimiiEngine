#pragma once
#include "Himii/Scene/ScriptableEntity.h"
#include "Himii/Core/Input.h"
#include "Himii/Core/KeyCodes.h"
#include <cmath>

// 简单的二维移动脚本：按 WASD/方向键 在 XY 平面移动实体
class Move2DScript : public Himii::ScriptableEntity {
public:
    float Speed = 1.0f; // units/sec

protected:
    void OnUpdate(Himii::Timestep ts) override {
        auto &tr = GetComponent<Himii::Transform>();
        const float dt = (float)ts;
        float dx = 0.0f, dy = 0.0f;

        using namespace Himii;
        if (Input::IsKeyPressed(Key::A) || Input::IsKeyPressed(Key::Left))  dx -= 1.0f;
        if (Input::IsKeyPressed(Key::D) || Input::IsKeyPressed(Key::Right)) dx += 1.0f;
        if (Input::IsKeyPressed(Key::W) || Input::IsKeyPressed(Key::Up))    dy += 1.0f;
        if (Input::IsKeyPressed(Key::S) || Input::IsKeyPressed(Key::Down))  dy -= 1.0f;

        if (dx != 0.0f || dy != 0.0f) {
            const float len = std::sqrt(dx*dx + dy*dy);
            dx /= len; dy /= len; // 归一化对角速度
            tr.Position.x += dx * Speed * dt;
            tr.Position.y += dy * Speed * dt;
        }
    }
};
