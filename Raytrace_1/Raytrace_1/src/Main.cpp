#include "Definitions.h"
#include "Ray.h"
#include "HittableList.h"
#include "Sphere.h"
#include "Color.h"
#include "Camera.h"
#include "Lambertian.h"
#include "Metal.h"
#include "Dielectric.h"

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

hittable_list random_scene() 
{
    hittable_list world;

    auto ground_material = make_shared<lambertian>(color(0.5, 0.5, 0.5));
    world.add(make_shared<sphere>(point3(0, -1000, 0), 1000, ground_material));

    int balls = 11;

    for (int a = -balls; a < balls; a++) 
    {
        for (int b = -balls; b < balls; b++) 
        {
            auto choose_mat = random_double();
            point3 center(a + 0.9 * random_double(), 0.2, b + 0.9 * random_double());

            if ((center - point3(4, 0.2, 0)).length() > 0.9)
            {
                shared_ptr<material> sphere_material;

                if (choose_mat < 0.8) 
                {
                    // diffuse
                    auto albedo = color::random() * color::random();
                    sphere_material = make_shared<lambertian>(albedo);
                    world.add(make_shared<sphere>(center, 0.2, sphere_material));
                }
                else if (choose_mat < 0.95) 
                {
                    // metal
                    auto albedo = color::random(0.5, 1);
                    auto fuzz = random_double(0, 0.5);
                    sphere_material = make_shared<metal>(albedo, fuzz);
                    world.add(make_shared<sphere>(center, 0.2, sphere_material));
                }
                else 
                {
                    // glass
                    sphere_material = make_shared<dielectric>(1.5);
                    world.add(make_shared<sphere>(center, 0.2, sphere_material));
                }
            }
        }
    }

    auto material1 = make_shared<dielectric>(1.5);
    world.add(make_shared<sphere>(point3(0, 1, 0), 1.0, material1));

    auto material2 = make_shared<lambertian>(color(0.4, 0.2, 0.1));
    world.add(make_shared<sphere>(point3(-4, 1, 0), 1.0, material2));

    auto material3 = make_shared<metal>(color(0.8, 0.8, 0.8), 0.0);
    world.add(make_shared<sphere>(point3(4, 1, 0), 1.0, material3));

    return world;
}

void Render()
{
    // Image
    const auto aspect_ratio = 16.0 / 9.0;
    const int image_width = 1280;
    const int image_height = static_cast<int>(image_width / aspect_ratio);
    const int samples_per_pixel = 500;
    const int max_depth = 50;

    // World
    auto world = random_scene();
    /*world.add(make_shared<sphere>(point3(0, 0, -1), 0.5, nullptr));
    world.add(make_shared<sphere>(point3(0, -100.5, -1), 100, nullptr));*/

    /*auto material_ground = make_shared<lambertian>(color(0.3, 0.3, 0.3));
    auto material_center = make_shared<lambertian>(color(0.7, 0.3, 0.3));
    auto material_blue   = make_shared<lambertian>(color(0.1, 0.1, 0.8));
    auto material_green   = make_shared<lambertian>(color(0.1, 0.8, 0.1));
    auto material_metal_1   = make_shared<metal>(color(0.8, 0.8, 0.8), 0.2);
    auto material_metal_2   = make_shared<metal>(color(0.9, 0.1, 0.1), 0.1);
    auto material_right  = make_shared<metal>(color(0.8, 0.6, 0.2), 0.9);
    auto material_glass_1  = make_shared<dielectric>(1.5);
    auto material_glass_2  = make_shared<dielectric>(2.5);

    world.add(make_shared<sphere>(point3(0.0, -100.5, -1.0), 100.0, material_ground));
    
    world.add(make_shared<sphere>(point3(0.0, 0.0, -1.0), 0.5, material_center));
    world.add(make_shared<sphere>(point3(-1.0, 0.0, -1.0), 0.5, material_metal_1));
    world.add(make_shared<sphere>(point3(1.0, 0.0, -1.0), 0.5, material_glass_1));
    world.add(make_shared<sphere>(point3(0.5, -0.25, 0.0), 0.25, material_glass_2));
    world.add(make_shared<sphere>(point3(0.5, 1.0, -1.0), 0.5, material_blue));
    world.add(make_shared<sphere>(point3(-1.0, 0.0, -2.0), 0.5, material_green));
    world.add(make_shared<sphere>(point3(-2.0, 0.0, -1.0), 0.5, material_glass_2));*/


    // Camera
    point3 lookfrom(13, 2, 3);
    point3 lookat(0, 0, 0);
    vec3 vup(0, 1, 0);
    auto dist_to_focus = 10.0;
    auto aperture = 0.1;
    auto vFov = 20;
    camera cam(lookfrom, lookat, vup, vFov, aspect_ratio, aperture, dist_to_focus);

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
