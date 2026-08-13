#pragma once

namespace Himii
{
    class MeshAsset;

    /// 按 Position / Normal / UV 生成切线（Lengyel 累积 + Gram-Schmidt）。
    /// 若网格已带有效源切线可跳过；导入路径在缺失时调用。
    void GenerateMeshTangents(MeshAsset &meshAsset);

    /// 任一顶点切线长度接近 0 则视为需要生成。
    bool MeshNeedsGeneratedTangents(const MeshAsset &meshAsset);
}
