#pragma once
#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>
#include <vector>

// 改进版 Perlin：可指定 seed，提供 2D 噪声、fBm（多倍频）、Ridged、多级 domain warp
class PerlinNoise {
private:
    std::vector<int> permutation;

public:
    explicit PerlinNoise(uint32_t seed = 1337)
    {
        permutation.resize(256);
        std::iota(permutation.begin(), permutation.end(), 0);

        std::mt19937 g(seed);
        std::shuffle(permutation.begin(), permutation.end(), g);
        permutation.insert(permutation.end(), permutation.begin(), permutation.end());
    }

    // 标准 3D Perlin，返回 [0,1]
    double noise(double x, double y, double z = 0.0) const
    {
        int X = static_cast<int>(floor(x)) & 255;
        int Y = static_cast<int>(floor(y)) & 255;
        int Z = static_cast<int>(floor(z)) & 255;

        x -= floor(x); y -= floor(y); z -= floor(z);
        double u = fade(x), v = fade(y), w = fade(z);

        int A = permutation[X] + Y;
        int AA = permutation[A] + Z;
        int AB = permutation[A + 1] + Z;
        int B = permutation[X + 1] + Y;
        int BA = permutation[B] + Z;
        int BB = permutation[B + 1] + Z;

        double res = lerp(
            w,
            lerp(v,
                lerp(u, grad(permutation[AA], x, y, z),     grad(permutation[BA], x - 1, y, z)),
                lerp(u, grad(permutation[AB], x, y - 1, z),  grad(permutation[BB], x - 1, y - 1, z))
            ),
            lerp(v,
                lerp(u, grad(permutation[AA + 1], x, y, z - 1),    grad(permutation[BA + 1], x - 1, y, z - 1)),
                lerp(u, grad(permutation[AB + 1], x, y - 1, z - 1), grad(permutation[BB + 1], x - 1, y - 1, z - 1))
            )
        );
        return (res + 1.0) * 0.5;
    }

    // 便捷 2D 调用
    inline double noise2D(double x, double y) const { return noise(x, y, 0.0); }

    // fBm：分形布朗运动，柔和连贯
    double fbm2D(double x, double y, int octaves, double lacunarity = 2.0, double gain = 0.5) const
    {
        double sum = 0.0, amp = 0.5, freq = 1.0, norm = 0.0;
        for (int i = 0; i < octaves; ++i)
        {
            sum += (noise2D(x * freq, y * freq) * 2.0 - 1.0) * amp; // [-1,1]
            norm += amp;
            amp *= gain;
            freq *= lacunarity;
        }
        sum /= (norm > 0.0 ? norm : 1.0); // 归一
        return (sum + 1.0) * 0.5; // [0,1]
    }

    // Ridged multifractal：山峰清晰，谷底锐利
    double ridged2D(double x, double y, int octaves, double lacunarity = 2.0, double gain = 0.5) const
    {
        double sum = 0.0, amp = 0.5, freq = 1.0, norm = 0.0;
        for (int i = 0; i < octaves; ++i)
        {
            double n = noise2D(x * freq, y * freq) * 2.0 - 1.0; // [-1,1]
            n = 1.0 - std::abs(n);  // 峰值反转
            n *= n;                 // 更尖锐
            sum += n * amp;
            norm += amp;
            amp *= gain;
            freq *= lacunarity;
        }
        sum /= (norm > 0.0 ? norm : 1.0);
        return sum; // [0,1]
    }

    // Domain warp：对输入坐标进行小幅扰动，破除网格感
    void domainWarp2D(double &x, double &y, double warpScale = 0.25, double warpAmp = 2.0) const
    {
        double dx = (noise2D(x * warpScale + 37.2, y * warpScale + 11.7) * 2.0 - 1.0) * warpAmp;
        double dy = (noise2D(x * warpScale - 19.4, y * warpScale + 73.8) * 2.0 - 1.0) * warpAmp;
        x += dx; y += dy;
    }

private:
    static double fade(double t) { return t * t * t * (t * (t * 6 - 15) + 10); }
    static double lerp(double t, double a, double b) { return a + t * (b - a); }
    static double grad(int hash, double x, double y, double z)
    {
        int h = hash & 15;
        double u = h < 8 ? x : y;
        double v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
        return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
    }
};
