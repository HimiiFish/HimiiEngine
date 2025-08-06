#pragma once
#include <iostream>
#include <vector>
#include <random>
#include <cmath>
#include <algorithm>
#include "glm/glm.hpp"

enum BlockType
{
	AIR=0,
	DIRT,
	STONE,
	GRASS
};

const glm::vec4 blockColors[] = {
        {0.0f, 0.0f, 0.0f, 1.0f},
        {0.68f, 0.42f, 0.18f, 1.0f},
        {0.12f, 0.36f, 0.26f, 1.0f},
        {0.08f, 0.72f, 0.25f, 1.0f},
};

class Terrain
{
	public:
        Terrain();
        virtual ~Terrain() = default;

        void GenerateTerrain();
        void GenerateHeightMap(std::vector<int> &heightMap);

        std::vector<std::vector<BlockType>> GetBlocks()
        {
            return blocks;
        }

        int GetWidth()
        {
            return worldWidth;
        }
        int getHeight()
        {
            return worldHeight;
         }

	private:
        int worldWidth = 100;
        int worldHeight = 30;

		std::vector<std::vector<BlockType>> blocks;
 };