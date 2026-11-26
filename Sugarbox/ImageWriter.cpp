#include "ImageWriter.h"

#include <cstdio>
#include <cstring>

#include "zlib.h"

namespace
{
   void PutUint32(std::vector<unsigned char>& out, unsigned int value)
   {
      out.push_back(static_cast<unsigned char>(value >> 24));
      out.push_back(static_cast<unsigned char>(value >> 16));
      out.push_back(static_cast<unsigned char>(value >> 8));
      out.push_back(static_cast<unsigned char>(value));
   }

   // Length, type, data, CRC of type + data
   void PutChunk(std::vector<unsigned char>& out, const char* type, const unsigned char* data, size_t size)
   {
      PutUint32(out, static_cast<unsigned int>(size));
      const size_t type_offset = out.size();
      out.insert(out.end(), type, type + 4);
      if (size > 0) out.insert(out.end(), data, data + size);
      const uLong crc = crc32(0L, out.data() + type_offset, static_cast<uInt>(size + 4));
      PutUint32(out, static_cast<unsigned int>(crc));
   }
}

namespace ImageWriter
{
   bool EncodePng(const unsigned char* rgba, int width, int height, std::vector<unsigned char>& png)
   {
      if (rgba == nullptr || width <= 0 || height <= 0) return false;

      // Each row : filter type 0 (none), then the pixels
      const size_t row_size = static_cast<size_t>(width) * 4;
      std::vector<unsigned char> raw((row_size + 1) * height);
      for (int y = 0; y < height; y++)
      {
         raw[y * (row_size + 1)] = 0;
         memcpy(&raw[y * (row_size + 1) + 1], rgba + y * row_size, row_size);
      }

      uLongf compressed_size = compressBound(static_cast<uLong>(raw.size()));
      std::vector<unsigned char> compressed(compressed_size);
      if (compress2(compressed.data(), &compressed_size, raw.data(), static_cast<uLong>(raw.size()), Z_BEST_SPEED) != Z_OK)
      {
         return false;
      }

      static const unsigned char signature[] = { 0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A };
      png.assign(signature, signature + sizeof(signature));

      std::vector<unsigned char> header;
      PutUint32(header, static_cast<unsigned int>(width));
      PutUint32(header, static_cast<unsigned int>(height));
      header.push_back(8);   // bits per channel
      header.push_back(6);   // RGBA
      header.push_back(0);   // deflate
      header.push_back(0);   // standard filters
      header.push_back(0);   // not interlaced
      PutChunk(png, "IHDR", header.data(), header.size());
      PutChunk(png, "IDAT", compressed.data(), compressed_size);
      PutChunk(png, "IEND", nullptr, 0);
      return true;
   }

   bool SavePng(const char* path, const unsigned char* rgba, int width, int height)
   {
      std::vector<unsigned char> png;
      if (!EncodePng(rgba, width, height, png)) return false;

      FILE* file = fopen(path, "wb");
      if (file == nullptr) return false;
      const bool written = fwrite(png.data(), 1, png.size(), file) == png.size();
      return fclose(file) == 0 && written;
   }

   std::string Base64(const std::vector<unsigned char>& data)
   {
      static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
      std::string out;
      out.reserve((data.size() + 2) / 3 * 4);
      size_t i = 0;
      for (; i + 2 < data.size(); i += 3)
      {
         const unsigned int value = (data[i] << 16) | (data[i + 1] << 8) | data[i + 2];
         out += alphabet[(value >> 18) & 0x3F];
         out += alphabet[(value >> 12) & 0x3F];
         out += alphabet[(value >> 6) & 0x3F];
         out += alphabet[value & 0x3F];
      }
      if (i < data.size())
      {
         unsigned int value = data[i] << 16;
         if (i + 1 < data.size()) value |= data[i + 1] << 8;
         out += alphabet[(value >> 18) & 0x3F];
         out += alphabet[(value >> 12) & 0x3F];
         out += (i + 1 < data.size()) ? alphabet[(value >> 6) & 0x3F] : '=';
         out += '=';
      }
      return out;
   }
}
