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
        // 石头层
        int surfaceHeight = heightMap[x];
        int dirtThickness = 3 + rand() % 2;
        int stoneDepth = 3 + rand() % 3; // 3-5 blocks below dirt
        for (int y = worldHeight-1; y >=0; --y)
        {
            if (y > heightMap[x])
            {
                // 地表上方的空气区块
                blocks[y][x] = AIR;
            }
            else if (y == heightMap[x])
            {
                // 顶部是草地层
                blocks[y][x] = GRASS;
            }
            else if (y >= heightMap[x] - dirtThickness)
            {
                // 草地下方是泥土层
                blocks[y][x] = DIRT;
            }
            else
            {
                // 更深的石头层
                blocks[y][x] = STONE;
            }
        }
    }
}

void Terrain::GenerateHeightMap(std::vector<int> &heightMap)
{
    // ... (此函数无需修改，保留原样)
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

        int baseHeight = worldHeight / 2;
        heightMap[x] = baseHeight + static_cast<int>(noiseValue);
        heightMap[x] = std::clamp(heightMap[x], 1, worldHeight - 1);
    }
}
