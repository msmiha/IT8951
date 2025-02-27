#include "image_cache.h"
#include <stdio.h>

/**
 * loadPreDecodedImage
 * -------------------
 * Loads a pre-decoded image from a cache file.
 */
UBYTE* loadPreDecodedImage(const char *cachePath, UDOUBLE *buffer_size) {
    FILE *fp = fopen(cachePath, "rb");
    if (!fp) {
        return NULL;
    }
    
    // Determine the file size.
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    UBYTE *buffer = (UBYTE *)malloc(size);
    if (!buffer) {
        fclose(fp);
        return NULL;
    }
    
    if (fread(buffer, 1, size, fp) != (size_t)size) {
        free(buffer);
        fclose(fp);
        return NULL;
    }
    
    fclose(fp);
    *buffer_size = (UDOUBLE)size;
    return buffer;
}

/**
 * cachePreDecodedImage
 * --------------------
 * Writes a pre-decoded image buffer to a cache file.
 */
int cachePreDecodedImage(const char *cachePath, UBYTE *buffer, UDOUBLE buffer_size) {
    FILE *fp = fopen(cachePath, "wb");
    if (!fp) {
        return -1;
    }
    
    size_t written = fwrite(buffer, 1, buffer_size, fp);
    fclose(fp);
    
    if (written != buffer_size) {
        return -1;
    }
    
    return 0;
}