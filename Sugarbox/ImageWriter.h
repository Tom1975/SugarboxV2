#pragma once

#include <string>
#include <vector>

// Images of the emulated screen, without any UI library : PNG (zlib), and base64 to send them
namespace ImageWriter
{
   // RGBA8888 pixels, rows from the top -> PNG file content
   bool EncodePng(const unsigned char* rgba, int width, int height, std::vector<unsigned char>& png);
   bool SavePng(const char* path, const unsigned char* rgba, int width, int height);

   std::string Base64(const std::vector<unsigned char>& data);
}
