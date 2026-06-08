#include "raylib.h"

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
#include <vector>

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

    auto ground_material = make_unique<lambertian>(color(0.5, 0.5, 0.5));
    world.add(make_unique<sphere>(point3(0, -1000, 0), 1000, std::move(ground_material)));

    int balls = 11;

    for (int a = -balls; a < balls; a++) 
    {
        for (int b = -balls; b < balls; b++) 
        {
            auto choose_mat = random_double();
            point3 center(a + 0.9 * random_double(), 0.2, b + 0.9 * random_double());

            if ((center - point3(4, 0.2, 0)).length() > 0.9)
            {
                if (choose_mat < 0.8)
                {
                    // diffuse
                    auto albedo = color::random() * color::random();
                    world.add(make_unique<sphere>(center, 0.2, make_unique<lambertian>(albedo)));
                }
                else if (choose_mat < 0.95)
                {
                    // metal
                    auto albedo = color::random(0.5, 1);
                    auto fuzz = random_double(0, 0.5);
                    world.add(make_unique<sphere>(center, 0.2, make_unique<metal>(albedo, fuzz)));
                }
                else
                {
                    // glass
                    world.add(make_unique<sphere>(center, 0.2, make_unique<dielectric>(1.5)));
                }
            }
        }
    }

    world.add(make_unique<sphere>(point3(0, 1, 0), 1.0, make_unique<dielectric>(1.5)));
    world.add(make_unique<sphere>(point3(-4, 1, 0), 1.0, make_unique<lambertian>(color(0.4, 0.2, 0.1))));
    world.add(make_unique<sphere>(point3(4, 1, 0), 1.0, make_unique<metal>(color(0.8, 0.8, 0.8), 0.0)));

    return world;
}

// Convert an accumulated sample color into a displayable / PPM 8-bit color.
// Same math as the original write_color(): average samples, gamma-correct (gamma=2.0), clamp.
Color to_display_color(const color& pixel_color, int samples_per_pixel)
{
    auto r = pixel_color.x();
    auto g = pixel_color.y();
    auto b = pixel_color.z();

    auto scale = 1.0 / samples_per_pixel;
    r = sqrt(scale * r);
    g = sqrt(scale * g);
    b = sqrt(scale * b);

    return Color{
        static_cast<unsigned char>(256 * clamp(r, 0.0, 0.999)),
        static_cast<unsigned char>(256 * clamp(g, 0.0, 0.999)),
        static_cast<unsigned char>(256 * clamp(b, 0.0, 0.999)),
        255
    };
}

// Write the finished image to a PPM file (top scanline first, matching the original output).
void write_ppm(const char* path, const std::vector<Color>& fb, int width, int height)
{
    ofstream out(path);
    out << "P3\n" << width << ' ' << height << "\n255\n";
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
        {
            const Color& c = fb[y * width + x];
            out << static_cast<int>(c.r) << ' '
                << static_cast<int>(c.g) << ' '
                << static_cast<int>(c.b) << '\n';
        }
}

void Render()
{
    // Image
    const auto aspect_ratio = 16.0 / 9.0;
    const int image_width = 320;
    const int image_height = static_cast<int>(image_width / aspect_ratio);
    const int samples_per_pixel = 1;
    const int max_depth = 1;

    // World
    auto world = random_scene();

    // Camera
    point3 lookfrom(13, 2, 3);
    point3 lookat(0, 0, 0);
    vec3 vup(0, 1, 0);
    auto dist_to_focus = 10.0;
    auto aperture = 0.1;
    auto vFov = 20;
    camera cam(lookfrom, lookat, vup, vFov, aspect_ratio, aperture, dist_to_focus);

    // Realtime preview window (scaled up so the small render is comfortably visible)
    const int scale = 4;
    InitWindow(image_width * scale, image_height * scale, "RaytraceWeekend - realtime (raylib)");
    SetTargetFPS(60);

    // Persistent off-screen buffer we "put pixels" into as each scanline finishes.
    RenderTexture2D target = LoadRenderTexture(image_width, image_height);
    BeginTextureMode(target);
        ClearBackground(BLACK);
    EndTextureMode();

    // CPU-side copy of the image (screen order, row 0 = top) used to write the PPM at the end.
    std::vector<Color> framebuffer(image_width * image_height, Color{ 0, 0, 0, 255 });

    int rows_done = 0;          // completed scanlines (0 = top of image)
    bool saved = false;
    auto start = std::chrono::high_resolution_clock::now();

    while (!WindowShouldClose())
    {
        // --- Trace one full scanline per frame so the window stays responsive ---
        if (rows_done < image_height)
        {
            const int j = (image_height - 1) - rows_done;   // raytracer row (bottom = 0)
            const int screen_y = rows_done;                 // screen / PPM row (top = 0)

            BeginTextureMode(target);
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

                Color c = to_display_color(pixel_color, samples_per_pixel);
                framebuffer[screen_y * image_width + i] = c;
                DrawPixel(i, screen_y, c);          // put pixel into the preview buffer
            }
            EndTextureMode();

            ++rows_done;

            if (rows_done == image_height && !saved)
            {
                auto end = std::chrono::high_resolution_clock::now();
                auto secs = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
                write_ppm("render.ppm", framebuffer, image_width, image_height);
                cout << "Rendered in " << secs << " seconds. Saved render.ppm" << endl;
                saved = true;
            }
        }

        // --- Present the buffer, scaled up (negative source height flips the RenderTexture upright) ---
        BeginDrawing();
            ClearBackground(BLACK);
            DrawTexturePro(
                target.texture,
                Rectangle{ 0, 0, (float)target.texture.width, -(float)target.texture.height },
                Rectangle{ 0, 0, (float)(image_width * scale), (float)(image_height * scale) },
                Vector2{ 0, 0 }, 0.0f, WHITE);

            if (!saved)
                DrawText(TextFormat("Tracing scanline %d / %d", rows_done, image_height), 10, 10, 20, RAYWHITE);
            else
                DrawText("Done - saved render.ppm", 10, 10, 20, GREEN);
        EndDrawing();
    }

    UnloadRenderTexture(target);
    CloseWindow();
}

int main()
{
    Render();
    return 0;
}
