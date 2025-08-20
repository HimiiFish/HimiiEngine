#include "Terrain.h"

Terrain::Terrain(int width, int height) : worldWidth(width), worldHeight(height)
{
    blocks.resize(worldHeight, std::vector<BlockType>(worldWidth, AIR));
    
}

void Terrain::GenerateTerrain()
{
    std::vector<int> heightMap(worldWidth);
    GenerateHeightMap(heightMap);

    for (int x = 0; x < worldWidth; ++x)
    {
        const int h = heightMap[x];
        const int dirtThickness = 3; // 固定厚度，避免 rand 带来的不可预测分支与开销

        // 先上半部分（> h）全部 AIR
        for (int y = h + 1; y < worldHeight; ++y)
        {
            blocks[y][x] = AIR;
        }

        if (h >= 0 && h < worldHeight)
        {
            blocks[h][x] = GRASS; // 表层草
        }

        // 下面 dirtThickness 层为 DIRT，再往下为 STONE
        const int dirtStart = std::max(0, h - dirtThickness);
        for (int y = dirtStart; y < h; ++y)
        {
            blocks[y][x] = DIRT;
        }

        for (int y = 0; y < dirtStart; ++y)
        {
            blocks[y][x] = STONE;
        }
    }
}

void Terrain::GenerateHeightMap(std::vector<int> &heightMap)
{
    // 保持简洁的多倍频 Perlin 合成
    double scale = 0.1;
    double amplitude = worldHeight / 4.0;
    double frequency = scale;
    double persistence = 0.5;
    int octaves = 4;

    for (int x = 0; x < worldWidth; ++x)
    {
        double noiseValue = 0.0;
        double currentAmplitude = amplitude;
        double currentFrequency = frequency;

        for (int i = 0; i < octaves; ++i)
        {
            double n = perlinNoise.noise(x * currentFrequency, 0.0);
            noiseValue += (n * 2.0 - 1.0) * currentAmplitude;
            currentAmplitude *= persistence;
            currentFrequency *= 2.0;
        }

    const int baseHeight = worldHeight / 2;
    int h = baseHeight + static_cast<int>(noiseValue);
    heightMap[x] = std::clamp(h, 1, worldHeight - 1);
    }
}
