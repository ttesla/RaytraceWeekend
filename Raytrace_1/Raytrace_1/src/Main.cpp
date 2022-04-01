#include <iostream>
#include <fstream>

#include "color.h"
#include "vec3.h"

using namespace std;

void WriteToFile()
{
    const int image_width = 256;
    const int image_height = 256;

    ofstream outputFile;
    outputFile.open("render.ppm");

    outputFile << "P3\n" << image_width << ' ' << image_height << "\n255\n";

    for (int j = image_height - 1; j >= 0; --j) 
    {
        std::cerr << "\rScanlines remaining: " << j << ' ' << std::flush;
        for (int i = 0; i < image_width; ++i) 
        {
            color pixel_color(double(i) / (image_width - 1), double(j) / (image_height - 1), 0.25);
            write_color(outputFile, pixel_color);
        }
    }

    outputFile.close();
    std::cerr << "\nDone.\n";
}

int main()
{
    WriteToFile();
    return 0;
}
