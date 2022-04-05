#include "Definitions.h"
#include "Ray.h"
#include "HittableList.h"
#include "Sphere.h"
#include "Color.h"
#include "Camera.h"
#include "Lambertian.h"
#include "Metal.h"

#include <iostream>
#include <fstream>
#include <chrono>

using std::endl;
using std::cout;
using std::cerr;
using std::ofstream;

//#define DIFFUSE_LAMBERTIAN

double hit_sphere(const point3& center, double radius, const ray& r) 
{
    vec3 oc = r.origin() - center;
    auto a = r.direction().length_squared();
    auto half_b = dot(oc, r.direction());
    auto c = oc.length_squared() - radius * radius;
    auto discriminant = half_b * half_b - a * c;

    if (discriminant < 0) 
    {
        return -1.0;
    }
    else 
    {
        return (-half_b - sqrt(discriminant)) / a;
    }
}

color ray_color(const ray& r, const hittable& world, int depth)
{
    hit_record rec;

    // If we've exceeded the ray bounce limit, no more light is gathered.
    if (depth <= 0)
        return color(0, 0, 0);
    
    if (world.hit(r, 0.001, Infinity, rec))
    {
        ray scattered;
        color attenuation;
        
        if (rec.mat_ptr->scatter(r, rec, attenuation, scattered))
            return attenuation * ray_color(scattered, world, depth - 1);

        return color(0, 0, 0);
    }

    vec3 unit_direction = unit_vector(r.direction());
    auto t = 0.5 * (unit_direction.y() + 1.0);
    return (1.0 - t) * color(1.0, 1.0, 1.0) + t * color(0.5, 0.7, 1.0);
}

void Render()
{
    // Image
    const auto aspect_ratio = 16.0 / 9.0;
    const int image_width = 400;
    const int image_height = static_cast<int>(image_width / aspect_ratio);
    const int samples_per_pixel = 50;
    const int max_depth = 50;

    // World
    hittable_list world;
    /*world.add(make_shared<sphere>(point3(0, 0, -1), 0.5, nullptr));
    world.add(make_shared<sphere>(point3(0, -100.5, -1), 100, nullptr));*/

    auto material_ground = make_shared<lambertian>(color(0.8, 0.8, 0.0));
    auto material_center = make_shared<lambertian>(color(0.7, 0.3, 0.3));
    auto material_left   = make_shared<metal>(color(0.8, 0.8, 0.8), 0.2);
    auto material_right  = make_shared<metal>(color(0.8, 0.6, 0.2), 0.9);

    world.add(make_shared<sphere>(point3(0.0, -100.5, -1.0), 100.0, material_ground));
    world.add(make_shared<sphere>(point3(0.0, 0.0, -1.0), 0.5, material_center));
    world.add(make_shared<sphere>(point3(-1.0, 0.0, -1.0), 0.5, material_left));
    world.add(make_shared<sphere>(point3(1.0, 0.0, -1.0), 0.5, material_right));

    world.add(make_shared<sphere>(point3(1.0, 1.1, -1.0), 0.5, material_center));

    world.add(make_shared<sphere>(point3(-1.0, 0.9, -1.0), 0.5, material_center));

    // Camera
    camera cam;

    // Write PPM File
    ofstream outputFile;
    outputFile.open("render.ppm");

    outputFile << "P3\n" << image_width << ' ' << image_height << "\n255\n";

    for (int j = image_height - 1; j >= 0; --j) 
    {
        cout << "\rScanlines remaining: " << j << ' ' << std::flush;
        for (int i = 0; i < image_width; ++i) 
        {
            color pixel_color(0, 0, 0);

            // Sampling
            for (int s = 0; s < samples_per_pixel; ++s) 
            {
                auto u = (i + random_double()) / (image_width - 1.0);
                auto v = (j + random_double()) / (image_height - 1.0);
                ray r = cam.get_ray(u, v);
                pixel_color += ray_color(r, world, max_depth);
            }
            
            write_color(outputFile, pixel_color, samples_per_pixel);
        }
    }

    outputFile.close();
    cout << "\nDone.\n";
}

int main()
{
    auto start = std::chrono::high_resolution_clock::now();
    Render();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);

    cout << "Renderen in: " << duration.count() << " seconds" << endl;

    return 0;
}
