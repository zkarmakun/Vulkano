#pragma once
#include <string>
#include <vector>
#include "Render/RenderResources.h"

class FFbxImport
{
public:
    static bool GetStaticMeshData(
        const std::string FilePath,
        std::vector<FStaticVertex>& Vertices,
        std::vector<uint32_t>& Indices);
};
