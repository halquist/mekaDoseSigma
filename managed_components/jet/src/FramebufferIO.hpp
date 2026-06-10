#pragma once

#include "JetConfig.hpp"
#include <cstdint>

#ifndef FRAMEBUFFER_RGB565_BYTE_SWAP
#define FRAMEBUFFER_RGB565_BYTE_SWAP 1
#endif

/// Pack logical RGB565 into the byte order expected by the GC9A01 SPI blit.
static inline uint16_t fbPack(uint16_t rgb565) {
#if FRAMEBUFFER_RGB565_BYTE_SWAP
    return static_cast<uint16_t>((rgb565 >> 8) | (rgb565 << 8));
#else
    return rgb565;
#endif
}

/// Unpack a stored framebuffer pixel back to logical RGB565 (for blending).
static inline uint16_t fbUnpack(uint16_t stored) {
    return fbPack(stored);
}
