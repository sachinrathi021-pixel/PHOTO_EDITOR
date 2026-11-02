// stb_impl.cpp - Implements stb_image and stb_image_write in exactly one translation unit.
// These libraries are used ONLY for decoding/encoding PNG/JPG files.
// All image-processing operations are implemented manually.

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
