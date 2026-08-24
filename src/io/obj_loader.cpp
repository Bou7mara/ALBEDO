#include "rt/io/obj_loader.h"
#include <fstream>
#include <sstream>
#include <map>
#include <tuple>
#include <vector>
#include <stdexcept>

namespace rt {

namespace {

struct CornerRaw {
    int pIdx = 0;
    int tIdx = 0;
    int nIdx = 0;
};

void ParseCorner(const std::string& token, CornerRaw& c) {
    c.pIdx = 0;
    c.tIdx = 0;
    c.nIdx = 0;
    if (token.empty()) return;

    size_t firstSlash = token.find('/');
    if (firstSlash == std::string::npos) {
        c.pIdx = std::stoi(token);
        return;
    }
    c.pIdx = std::stoi(token.substr(0, firstSlash));
    size_t secondSlash = token.find('/', firstSlash + 1);
    if (secondSlash == std::string::npos) {
        std::string vtStr = token.substr(firstSlash + 1);
        if (!vtStr.empty()) c.tIdx = std::stoi(vtStr);
        return;
    }
    std::string vtStr = token.substr(firstSlash + 1, secondSlash - firstSlash - 1);
    if (!vtStr.empty()) c.tIdx = std::stoi(vtStr);
    std::string vnStr = token.substr(secondSlash + 1);
    if (!vnStr.empty()) c.nIdx = std::stoi(vnStr);
}

std::shared_ptr<TriangleMesh> LoadOBJFromStream(std::istream& is) {
    std::vector<Point3f> rawPositions;
    std::vector<Point2f> rawUVs;
    std::vector<Normal3f> rawNormals;
    std::vector<std::vector<CornerRaw>> faces;

    std::string line;
    while (std::getline(is, line)) {

        if (!line.empty() && line.back() == '\r') line.pop_back();

        std::istringstream lineStream(line);
        std::string tag;
        if (!(lineStream >> tag)) continue;

        if (tag == "v") {
            float x, y, z;
            if (lineStream >> x >> y >> z) {
                rawPositions.emplace_back(x, y, z);
            }
        } else if (tag == "vt") {
            float u, v;
            if (lineStream >> u >> v) {
                rawUVs.emplace_back(u, v);
            }
        } else if (tag == "vn") {
            float x, y, z;
            if (lineStream >> x >> y >> z) {
                rawNormals.emplace_back(x, y, z);
            }
        } else if (tag == "f") {
            std::vector<CornerRaw> faceCorners;
            std::string token;
            while (lineStream >> token) {
                CornerRaw c;
                ParseCorner(token, c);
                faceCorners.push_back(c);
            }
            if (faceCorners.size() < 3) {
                throw std::runtime_error("OBJ face has fewer than 3 vertices");
            }
            faces.push_back(faceCorners);
        }
    }

    bool hasAnyUVs = !rawUVs.empty();
    bool hasAnyNormals = !rawNormals.empty();

    auto mesh = std::make_shared<TriangleMesh>();
    std::map<std::tuple<int, int, int>, int> vertexMap;

    auto getUnifiedIndex = [&](const CornerRaw& c) -> int {
        if (c.pIdx == 0) {
            throw std::runtime_error("Invalid position index 0 in OBJ face");
        }
        int finalP = (c.pIdx < 0) ? (static_cast<int>(rawPositions.size()) + c.pIdx) : (c.pIdx - 1);
        if (finalP < 0 || finalP >= static_cast<int>(rawPositions.size())) {
            throw std::runtime_error("Position index out of bounds in OBJ face");
        }

        int finalT = -1;
        if (c.tIdx < 0) {
            finalT = static_cast<int>(rawUVs.size()) + c.tIdx;
        } else if (c.tIdx > 0) {
            finalT = c.tIdx - 1;
        }

        int finalN = -1;
        if (c.nIdx < 0) {
            finalN = static_cast<int>(rawNormals.size()) + c.nIdx;
        } else if (c.nIdx > 0) {
            finalN = c.nIdx - 1;
        }

        auto key = std::make_tuple(finalP, finalT, finalN);
        auto it = vertexMap.find(key);
        if (it != vertexMap.end()) {
            return it->second;
        }

        int newIdx = static_cast<int>(mesh->positions.size());
        mesh->positions.push_back(rawPositions[finalP]);

        if (hasAnyUVs) {
            Point2f uv(0.0f, 0.0f);
            if (finalT >= 0 && finalT < static_cast<int>(rawUVs.size())) {
                uv = rawUVs[finalT];
            }
            mesh->uvs.push_back(uv);
        }

        if (hasAnyNormals) {
            Normal3f norm(0.0f, 0.0f, 0.0f);
            if (finalN >= 0 && finalN < static_cast<int>(rawNormals.size())) {
                norm = rawNormals[finalN];
            }
            mesh->normals.push_back(norm);
        }

        vertexMap[key] = newIdx;
        return newIdx;
    };

    for (const auto& face : faces) {
        std::vector<int> unifiedIndices;
        unifiedIndices.reserve(face.size());
        for (const auto& c : face) {
            unifiedIndices.push_back(getUnifiedIndex(c));
        }

        for (size_t i = 1; i + 1 < unifiedIndices.size(); ++i) {
            mesh->indices.push_back(unifiedIndices[0]);
            mesh->indices.push_back(unifiedIndices[i]);
            mesh->indices.push_back(unifiedIndices[i + 1]);
        }
    }

    return mesh;
}

}

std::shared_ptr<TriangleMesh> LoadOBJFromString(const std::string& objData) {
    std::istringstream is(objData);
    return LoadOBJFromStream(is);
}

std::shared_ptr<TriangleMesh> LoadOBJ(const std::filesystem::path& path) {
    std::ifstream is(path);
    if (!is.is_open()) {
        throw std::runtime_error("Failed to open OBJ file: " + path.string());
    }
    return LoadOBJFromStream(is);
}

}
