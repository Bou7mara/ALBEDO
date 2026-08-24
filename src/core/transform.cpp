#include "rt/core/transform.h"
#include <cassert>
#include <cmath>
#include <numbers>
#include <utility>
#include <algorithm>
#include <cstring>

namespace rt {

Transform Transform::RotateX(float thetaDeg) {
    float rad = thetaDeg * std::numbers::pi_v<float> / 180.0f;
    float sinTheta = std::sin(rad);
    float cosTheta = std::cos(rad);

    float mat[4][4] = {
        {1, 0, 0, 0},
        {0, cosTheta, -sinTheta, 0},
        {0, sinTheta, cosTheta, 0},
        {0, 0, 0, 1}
    };
    float inv[4][4] = {
        {1, 0, 0, 0},
        {0, cosTheta, sinTheta, 0},
        {0, -sinTheta, cosTheta, 0},
        {0, 0, 0, 1}
    };
    return Transform(mat, inv);
}

Transform Transform::RotateY(float thetaDeg) {
    float rad = thetaDeg * std::numbers::pi_v<float> / 180.0f;
    float sinTheta = std::sin(rad);
    float cosTheta = std::cos(rad);

    float mat[4][4] = {
        {cosTheta, 0, sinTheta, 0},
        {0, 1, 0, 0},
        {-sinTheta, 0, cosTheta, 0},
        {0, 0, 0, 1}
    };
    float inv[4][4] = {
        {cosTheta, 0, -sinTheta, 0},
        {0, 1, 0, 0},
        {sinTheta, 0, cosTheta, 0},
        {0, 0, 0, 1}
    };
    return Transform(mat, inv);
}

Transform Transform::RotateZ(float thetaDeg) {
    float rad = thetaDeg * std::numbers::pi_v<float> / 180.0f;
    float sinTheta = std::sin(rad);
    float cosTheta = std::cos(rad);

    float mat[4][4] = {
        {cosTheta, -sinTheta, 0, 0},
        {sinTheta, cosTheta, 0, 0},
        {0, 0, 1, 0},
        {0, 0, 0, 1}
    };
    float inv[4][4] = {
        {cosTheta, sinTheta, 0, 0},
        {-sinTheta, cosTheta, 0, 0},
        {0, 0, 1, 0},
        {0, 0, 0, 1}
    };
    return Transform(mat, inv);
}

Transform Transform::LookAt(const Point3f& eye, const Point3f& look, const Vector3f& up) {
    Vector3f dir = Normalize(look - eye);
    Vector3f right = Normalize(Cross(Normalize(up), dir));
    Vector3f newUp = Cross(dir, right);

    float cameraToWorld[4][4] = {
        {right.x, newUp.x, dir.x, eye.x},
        {right.y, newUp.y, dir.y, eye.y},
        {right.z, newUp.z, dir.z, eye.z},
        {0, 0, 0, 1}
    };

    float worldToCamera[4][4] = {
        {right.x, right.y, right.z, -(right.x * eye.x + right.y * eye.y + right.z * eye.z)},
        {newUp.x, newUp.y, newUp.z, -(newUp.x * eye.x + newUp.y * eye.y + newUp.z * eye.z)},
        {dir.x, dir.y, dir.z, -(dir.x * eye.x + dir.y * eye.y + dir.z * eye.z)},
        {0, 0, 0, 1}
    };

    return Transform(worldToCamera, cameraToWorld);
}

void Transform::Inverse4x4(const float mat[4][4], float out[4][4]) {
    float temp[4][4];
    std::memcpy(temp, mat, sizeof(temp));
    SetIdentity(out);

    for (int i = 0; i < 4; ++i) {
        int pivot = i;
        float maxVal = std::abs(temp[i][i]);
        for (int j = i + 1; j < 4; ++j) {
            if (std::abs(temp[j][i]) > maxVal) {
                maxVal = std::abs(temp[j][i]);
                pivot = j;
            }
        }

        assert(maxVal > 0.0f && "Singular matrix inside Transform!");
        if (pivot != i) {
            for (int col = 0; col < 4; ++col) {
                std::swap(temp[i][col], temp[pivot][col]);
                std::swap(out[i][col], out[pivot][col]);
            }
        }

        float div = temp[i][i];
        for (int j = 0; j < 4; ++j) {
            temp[i][j] /= div;
            out[i][j] /= div;
        }

        for (int row = 0; row < 4; ++row) {
            if (row != i) {
                float factor = temp[row][i];
                for (int col = 0; col < 4; ++col) {
                    temp[row][col] -= factor * temp[i][col];
                    out[row][col] -= factor * out[i][col];
                }
            }
        }
    }
}

}
