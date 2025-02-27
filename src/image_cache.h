#ifndef IMAGE_CACHE_H
#define IMAGE_CACHE_H

#include "../lib/Config/DEV_Config.h"  // Ensure this header defines UBYTE, UWORD, UDOUBLE, etc.
#include <stdlib.h>

/**
 * loadPreDecodedImage
 * -------------------
 * Loads a pre-decoded image from a cache file.
 *
 * @param cachePath: Path to the cached raw image file.
 * @param buffer_size: Pointer to a UDOUBLE that will receive the size in bytes.
 *
 * @return: Pointer to the allocated buffer containing the image data,
 *          or NULL on failure.
 */
UBYTE* loadPreDecodedImage(const char *cachePath, UDOUBLE *buffer_size);

/**
 * cachePreDecodedImage
 * --------------------
 * Writes a pre-decoded image buffer to a cache file.
 *
 * @param cachePath: Path where the image data should be saved.
 * @param buffer: Pointer to the image data buffer.
 * @param buffer_size: Size of the buffer in bytes.
 *
 * @return: 0 on success, -1 on failure.
 */
int cachePreDecodedImage(const char *cachePath, UBYTE *buffer, UDOUBLE buffer_size);

#endif // IMAGE_CACHE_H
