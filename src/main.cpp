#include "rt/core/point2.h"
#include "rt/cameras/perspective_camera.h"
#include "rt/shapes/sphere.h"
#include "rt/scene/scene.h"
#include "rt/io/ppm_writer.h"

#include <fstream>
#include <memory>
#include <iostream>

using namespace rt;

int main() {
    const int imageWidth = 400;
    const int imageHeight = 200;

    std::cout << "Rendering a " << imageWidth << "x" << imageHeight << " image..." << std::endl;

    PerspectiveCamera camera(
        Point3f(0.0f, 0.0f, 0.0f),   // eye
        Point3f(0.0f, 0.0f, -1.0f),  // lookAt: looking down -world-Z
        Vector3f(0.0f, 1.0f, 0.0f),  // up
        90.0f,                        // vertical FOV
        imageWidth, imageHeight
    );

    Scene scene;
    scene.Add(std::make_shared<Sphere>(
        Transform::Translate(Vector3f(0.0f, 0.0f, -1.0f)), 0.5f));

    std::string outputPath = NextImagePath();
    std::ofstream out(outputPath);
    if (!out) {
        std::cerr << "Failed to open " << outputPath << " for writing!" << std::endl;
        return 1;
    }

    WritePPMHeader(out, imageWidth, imageHeight);

    for (int y = 0; y < imageHeight; ++y) {
        for (int x = 0; x < imageWidth; ++x) {
            CameraSample sample{Point2f(x + 0.5f, y + 0.5f)};
            Ray ray = camera.GenerateRay(sample);

            SurfaceInteraction isect;
            if (scene.Intersect(ray, &isect)) {
                // Normal-visualization shading: map [-1,1] per component to [0,1] for display.
                // Cast to Vector3f first so component-wise operations are defined.
                Vector3f n = Vector3f(isect.n);
                WritePixel(out, 0.5f * (n.x + 1.0f), 0.5f * (n.y + 1.0f),
                           0.5f * (n.z + 1.0f));
            } else {
                // Background sky vertical gradient (interpolate by ray direction Y)
                Vector3f unitDir = Normalize(ray.d);
                float t = 0.5f * (unitDir.y + 1.0f);
                Vector3f white(1.0f, 1.0f, 1.0f);
                Vector3f skyBlue(0.5f, 0.7f, 1.0f);
                Vector3f color = (1.0f - t) * white + t * skyBlue;
                WritePixel(out, color.x, color.y, color.z);
            }
        }
    }

    std::cout << "Rendering completed. Output written to " << outputPath << std::endl;
    return 0;
}
