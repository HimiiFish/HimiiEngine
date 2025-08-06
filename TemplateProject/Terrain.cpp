#include "Terrain.h"
#include "Perlin.h"

Terrain::Terrain()
{
    blocks.resize(worldHeight, std::vector<BlockType>(worldWidth, AIR));
    
}

void Terrain::GenerateTerrain()
{
    std::vector<int> heightMap(worldWidth);
    GenerateHeightMap(heightMap);

    for (int x = 0; x < worldWidth; ++x)
    {
        // 石头层
        int stoneDepth = 10 + rand() % 20;
        for (int y = heightMap[x] + stoneDepth; y < worldHeight - 5; ++y)
        {
            blocks[y][x] = STONE;
        }

        // 泥土层
        int dirtThickness = 3 + rand() % 4;
        for (int y = heightMap[x]; y < heightMap[x] + dirtThickness; ++y)
        {
            blocks[y][x] = DIRT;
        }

        // 草地表层
        if (heightMap[x] > 0)
        {
            blocks[heightMap[x] - 1][x] = GRASS;
        }
    }
}

void Terrain::GenerateHeightMap(std::vector<int> &heightMap)
{
    PerlinNoise pn;
    double scale = 0.05; // 控制噪声频率

    for (int x = 0; x < worldWidth; ++x)
    {
        // 使用多层噪声叠加 (分形布朗运动)
        double noise = 0.0;
        double amplitude = 30.0;
        double frequency = scale;
        double persistence = 0.5; // 每层振幅衰减系数
        int octaves = 4;          // 噪声层数

        for (int i = 0; i < octaves; ++i)
        {
            noise += pn.noise(x * frequency, 0) * amplitude;
            amplitude *= persistence;
            frequency *= 2.0;
        }

        heightMap[x] =  150+static_cast<int>(noise);

        // 确保高度在合理范围内
        heightMap[x] = std::max(20, std::min(worldHeight - 30, heightMap[x]));
    }
}
