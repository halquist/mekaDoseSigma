// JetConfig.hpp - Configuration for Seeed Studio XIAO ESP32-S3 with Round Display
// Display: 240x240 RGB565, GC9A01 controller

#define RENDER_TILE_BUFFER 0
#define TILE_WIDTH 32
#define TILE_HEIGHT 32

#define FAST_Z 1
#define LAZY_Z 0

// Store pixels in SPI wire byte order so the display blit needs no per-frame swap.
#define FRAMEBUFFER_RGB565_BYTE_SWAP 1

#define SCREEN_DOOR_ALPHA 0
#define SKIP_ZERO_AREA_TRIANGLES 1
#define NOISE_ALPHA 0

#define Z_BUFFERING 0
#define SORT_TRIANGLES 1
#define SORT_SCENE_OBJECTS 1
#define SORT_SCENE_REVERSE 1

#define DEPTH_ALPHA_BLEND 0

#define TEXTURE_MAPPING 0
#define PERSPECTIVE_CORRECT_TEXTURES 0
#define BILINEAR_FILTER 0

#define LIGHTING 1
#define Z_BRIGHTNESS 0

#define FLOAT_CAMERA_ANGLES 1
#define FLOAT_SIN_CACHE_SCALE 10
#define FLOAT_TAN_CACHE_SCALE 1

#define POSTFX_CRT 0
#define POSTFX_CELLSHADING 0
#define POSTFX_ANTIALIASING 0
#define POSTFX_BLOOM 0
#define POSTFX_MOTION_BLUR 0
#define POSTFX_CHROMATIC 0
#define POSTFX_PIXELATE 0

#define DEBUG_OVERDRAW 0

#define CRT_SCANLINE_INTENSITY 48
#define MOTION_BLUR_STRENGTH 50
#define CHROMATIC_OFFSET 2
#define PIXELATE_SIZE 4
#define CELLSHADING_CELL_BITS 4

#define zBrightFar (1600 * 8)
#define zBrightNear (200 * 8)
#define zBrightScale 48

#define depthFogFar (2048 * 8)
#define depthFogNear (1024 * 8)

#define HALF_WIDTH_BUFFERS 0
#define FIELD_BUFFERS 0
#define CHECKERBOARD_MODE 0
#define CHECKERBOARD_RECONSTRUCTION 0

#define MAX_PICK_QUERIES 0

#if defined(ESP_PLATFORM)
#define ESP32
#endif
