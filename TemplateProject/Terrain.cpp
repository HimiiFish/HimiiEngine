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
        int stoneDepth = 3 + rand() % 3; // 3-5 blocks below dirt
        int stoneStart = std::min(heightMap[x] + stoneDepth, worldHeight);
        for (int y = stoneStart; y < worldHeight; ++y)
        {
            blocks[y][x] = STONE;
        }
        // 泥土层
        int dirtThickness = 3 + rand() % 2; // 3-4 blocks of dirt
        int dirtTop = std::min(heightMap[x] + dirtThickness, worldHeight);
        for (int y = heightMap[x]; y < dirtTop; ++y)
        {
            blocks[y][x] = DIRT;
        }

        // 草地表层
        if (heightMap[x] > 0)
        {
            blocks[std::min(heightMap[x] - 1, worldHeight - 1)][x] = GRASS;
        }

    }
}

void Terrain::GenerateHeightMap(std::vector<int> &heightMap)
{
    PerlinNoise pn;
    double scale = 0.1; // 控制噪声频率

    for (int x = 0; x < worldWidth; ++x)
    {
        double noiseValue = 0.0;
        double amplitude = worldHeight / 4.0;
        double frequency = scale;
        double persistence = 0.5;
        int octaves = 4;

        for (int i = 0; i < octaves; ++i)
        {
            double n = pn.noise(x * frequency, 0.0);
            noiseValue += (n * 2.0 - 1.0) * amplitude;
            amplitude *= persistence;
            frequency *= 2.0;
        }

        int baseHeight = worldHeight / 2;
        heightMap[x] = baseHeight + static_cast<int>(noiseValue);
        heightMap[x] = std::clamp(heightMap[x], 1, worldHeight - 1);
    }
}
