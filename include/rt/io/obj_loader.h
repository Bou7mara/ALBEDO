#pragma once
#include "rt/shapes/triangle.h"
#include <filesystem>
#include <memory>
#include <string>

namespace rt {

    std::shared_ptr<TriangleMesh> LoadOBJ(const std::filesystem::path& path);
    std::shared_ptr<TriangleMesh> LoadOBJFromString(const std::string& objData);

}
