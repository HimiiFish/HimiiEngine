#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>
#include <vector>

class PerlinNoise {
private:
    std::vector<int> permutation;

public:
    PerlinNoise()
    {
        // 初始化排列表
        permutation.resize(256);
        std::iota(permutation.begin(), permutation.end(), 0);

        // 随机打乱排列
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(permutation.begin(), permutation.end(), g);

        // 复制排列表到第二个部分，避免索引越界
        permutation.insert(permutation.end(), permutation.begin(), permutation.end());
    }

    double noise(double x, double y, double z = 0.0)
    {
        // 确定包含点的单位立方体
        int X = static_cast<int>(floor(x)) & 255;
        int Y = static_cast<int>(floor(y)) & 255;
        int Z = static_cast<int>(floor(z)) & 255;

        // 计算点在单位立方体中的相对坐标
        x -= floor(x);
        y -= floor(y);
        z -= floor(z);

        // 计算缓和曲线
        double u = fade(x);
        double v = fade(y);
        double w = fade(z);

        // 哈希立方体8个角的坐标
        int A = permutation[X] + Y;
        int AA = permutation[A] + Z;
        int AB = permutation[A + 1] + Z;
        int B = permutation[X + 1] + Y;
        int BA = permutation[B] + Z;
        int BB = permutation[B + 1] + Z;

        // 混合8个角的贡献
        double res = lerp(
                w,
                lerp(v, lerp(u, grad(permutation[AA], x, y, z), grad(permutation[BA], x - 1, y, z)),
                     lerp(u, grad(permutation[AB], x, y - 1, z), grad(permutation[BB], x - 1, y - 1, z))),
                lerp(v, lerp(u, grad(permutation[AA + 1], x, y, z - 1), grad(permutation[BA + 1], x - 1, y, z - 1)),
                     lerp(u, grad(permutation[AB + 1], x, y - 1, z - 1),
                          grad(permutation[BB + 1], x - 1, y - 1, z - 1))));
        return (res + 1.0) / 2.0; // 将结果映射到[0,1]范围
    }

private:
    // 缓和曲线 (6t^5 - 15t^4 + 10t^3)
    static double fade(double t)
    {
        return t * t * t * (t * (t * 6 - 15) + 10);
    }

    // 线性插值
    static double lerp(double t, double a, double b)
    {
        return a + t * (b - a);
    }

    // 计算梯度向量并计算点积
    static double grad(int hash, double x, double y, double z)
    {
        int h = hash & 15;
        // 将4位哈希值转换为12个梯度方向之一
        double u = h < 8 ? x : y;
        double v = h < 4 ? y : h == 12 || h == 14 ? x : z;
        return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
    }
};
