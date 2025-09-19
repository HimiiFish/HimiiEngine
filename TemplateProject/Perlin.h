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
        // ��ʼ�����б�
        permutation.resize(256);
        std::iota(permutation.begin(), permutation.end(), 0);

        // �����������
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(permutation.begin(), permutation.end(), g);

        // �������б��ڶ������֣���������Խ��
        permutation.insert(permutation.end(), permutation.begin(), permutation.end());
    }

    double noise(double x, double y, double z = 0.0)
    {
        // ȷ��������ĵ�λ������
        int X = static_cast<int>(floor(x)) & 255;
        int Y = static_cast<int>(floor(y)) & 255;
        int Z = static_cast<int>(floor(z)) & 255;

        // ������ڵ�λ�������е��������
        x -= floor(x);
        y -= floor(y);
        z -= floor(z);

        // ���㻺������
        double u = fade(x);
        double v = fade(y);
        double w = fade(z);

        // ��ϣ������8���ǵ�����
        int A = permutation[X] + Y;
        int AA = permutation[A] + Z;
        int AB = permutation[A + 1] + Z;
        int B = permutation[X + 1] + Y;
        int BA = permutation[B] + Z;
        int BB = permutation[B + 1] + Z;

        // ���8���ǵĹ���
        double res = lerp(
                w,
                lerp(v, lerp(u, grad(permutation[AA], x, y, z), grad(permutation[BA], x - 1, y, z)),
                     lerp(u, grad(permutation[AB], x, y - 1, z), grad(permutation[BB], x - 1, y - 1, z))),
                lerp(v, lerp(u, grad(permutation[AA + 1], x, y, z - 1), grad(permutation[BA + 1], x - 1, y, z - 1)),
                     lerp(u, grad(permutation[AB + 1], x, y - 1, z - 1),
                          grad(permutation[BB + 1], x - 1, y - 1, z - 1))));
        return (res + 1.0) / 2.0; // �����ӳ�䵽[0,1]��Χ
    }

private:
    // �������� (6t^5 - 15t^4 + 10t^3)
    static double fade(double t)
    {
        return t * t * t * (t * (t * 6 - 15) + 10);
    }

    // ���Բ�ֵ
    static double lerp(double t, double a, double b)
    {
        return a + t * (b - a);
    }

    // �����ݶ�������������
    static double grad(int hash, double x, double y, double z)
    {
        int h = hash & 15;
        // ��4λ��ϣֵת��Ϊ12���ݶȷ���֮һ
        double u = h < 8 ? x : y;
        double v = h < 4 ? y : h == 12 || h == 14 ? x : z;
        return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
    }
};
