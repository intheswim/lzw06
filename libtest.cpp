#include <cstring>
#include <cstdlib>
#include <cstdio>
#include "export.h"

#include <chrono>
#include <iostream>

int main ()
{
    const char inputFile[] = "sample.txt";
    const char compressedFile[] = "sample.lzw";
    const char outputFile[] = "sample_copy.txt";

    std::chrono::high_resolution_clock::time_point start;

    start = std::chrono::high_resolution_clock::now();

    int ret = Compress (inputFile, compressedFile, VERBOSE_OUTPUT);

    printf ("Compression : %s.\n", ret ? "Successful" : "Failed");

    if (!ret)
        return EXIT_FAILURE;

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);    

    std::cout << duration.count() << " microsecs\n";

    start = std::chrono::high_resolution_clock::now();

    ret = Decompress (compressedFile, outputFile, OVERWRITE_FLAG | VERBOSE_OUTPUT);

    printf ("Decompression : %s.\n", ret ? "Successful" : "Failed");

    if (!ret)
        return EXIT_FAILURE;

    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);    

    std::cout << duration.count() << " microsecs\n";

    return EXIT_SUCCESS;

}
