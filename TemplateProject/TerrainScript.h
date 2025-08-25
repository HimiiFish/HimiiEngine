#pragma once
#include "Himii/Scene/ScriptableEntity.h"
#include "glm/glm.hpp"

class CubeLayer; // fwd

// 将 Terrain 生成与参数封装为脚本组件，挂到 Terrain 实体上
class TerrainScript : public Himii::ScriptableEntity {
public:
    // 可公开的参数（Inspector 暂时可直接通过获取组件方式编辑）
    int Width = 128;
    int Depth = 128;
    int Height = 32;
    bool Dirty = true; // 参数变化后置 true 触发重建

    struct NoiseSettings {
        uint32_t seed = 1337;
        float biomeScale = 0.02f;
        float continentScale = 0.008f;
        float continentStrength = 0.6f;
        float plainsScale = 0.10f;
        int    plainsOctaves = 5;
        float plainsLacunarity = 2.0f;
        float plainsGain = 0.5f;
        float mountainScale = 0.06f;
        int    mountainOctaves = 5;
        float mountainLacunarity = 2.0f;
        float mountainGain = 0.5f;
        float ridgeSharpness = 1.5f;
        float warpScale = 0.15f;
        float warpAmp = 2.5f;
        float detailScale = 0.25f;
        float detailAmp = 0.15f;
        float heightMul = 1.0f;
        float plateau = 0.15f;
        int   stepLevels = 4;
        float curveExponent = 1.1f;
        float valleyDepth = 0.05f;
        float seaLevel = 0.0f;
        float mountainWeight = 0.56f;
    } Noise;

protected:
    void OnCreate() override {}
    void OnDestroy() override {}
    void OnUpdate(Himii::Timestep) override {}
};
