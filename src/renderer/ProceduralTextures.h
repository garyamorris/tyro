#pragma once
#include <cstdint>
#include <vector>

namespace tyro {

// Each generator returns RGBA8 pixel data laid out top-to-bottom (matches
// stb_image's flipped-on-load convention so we can reuse Texture::loadFromMemory
// directly). All produce tileable 256x256 images by default.

std::vector<std::uint8_t> makeCheckerTex (int size = 256, int cells = 8);
std::vector<std::uint8_t> makeBrickTex   (int size = 256);
std::vector<std::uint8_t> makeWoodTex    (int size = 256);
std::vector<std::uint8_t> makeMarbleTex  (int size = 256);
std::vector<std::uint8_t> makeNoiseTex   (int size = 256);
std::vector<std::uint8_t> makeHexTex     (int size = 256);

} // namespace tyro
