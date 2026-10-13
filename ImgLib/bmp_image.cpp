#include "bmp_image.h"

#include <array>
#include <fstream>
#include <string_view>

#include "pack_defines.h"

using namespace std;

namespace img_lib {
namespace {
constexpr uint16_t BMP_SIGNATURE = 0x4D42;
constexpr uint32_t BMP_FILE_HEADER_SIZE = 14;
constexpr uint32_t BMP_INFO_HEADER_SIZE = 40;
constexpr uint32_t BMP_DATA_OFFSET =
    BMP_FILE_HEADER_SIZE + BMP_INFO_HEADER_SIZE;
constexpr uint16_t BMP_PLANES = 1;
constexpr uint16_t BMP_BITS_PER_PIXEL = 24;
constexpr uint32_t BMP_COMPRESSION_NONE = 0;
constexpr int32_t BMP_PELS_PER_METER = 11811;
constexpr int32_t BMP_CLR_USED = 0;
constexpr int32_t BMP_CLR_IMPORTANT = 0x1000000;

static int GetBMPStride(int w) {
    return 4 * ((w * 3 + 3) / 4);
}
}  // namespace

PACKED_STRUCT_BEGIN BitmapFileHeader {
    uint16_t bf_type;
    uint32_t bf_size;
    uint32_t bf_reserved;
    uint32_t bf_off_bits;
}
PACKED_STRUCT_END

PACKED_STRUCT_BEGIN BitmapInfoHeader {
    uint32_t bi_size;
    int32_t bi_width;
    int32_t bi_height;
    uint16_t bi_planes;
    uint16_t bi_bit_count;
    uint32_t bi_compression;
    uint32_t bi_size_image;
    int32_t bi_x_pels_per_meter;
    int32_t bi_y_pels_per_meter;
    int32_t bi_clr_used;
    int32_t bi_clr_important;
}
PACKED_STRUCT_END

// напишите эту функцию
bool SaveBMP(const Path& file, const Image& image) {
    const int w = image.GetWidth();
    const int h = image.GetHeight();

    if (w <= 0 || h <= 0) {
        return false;
    }

    const int stride = GetBMPStride(w);
    const uint32_t data_size =
        static_cast<uint32_t>(stride) * static_cast<uint32_t>(h);

    BitmapFileHeader file_header{};
    file_header.bf_type = BMP_SIGNATURE;
    file_header.bf_size = BMP_DATA_OFFSET + data_size;
    file_header.bf_reserved = 0;
    file_header.bf_off_bits = BMP_DATA_OFFSET;

    BitmapInfoHeader info_header{};
    info_header.bi_size = BMP_INFO_HEADER_SIZE;
    info_header.bi_width = w;
    info_header.bi_height = h;
    info_header.bi_planes = BMP_PLANES;
    info_header.bi_bit_count = BMP_BITS_PER_PIXEL;
    info_header.bi_compression = BMP_COMPRESSION_NONE;
    info_header.bi_size_image = data_size;
    info_header.bi_x_pels_per_meter = BMP_PELS_PER_METER;
    info_header.bi_y_pels_per_meter = BMP_PELS_PER_METER;
    info_header.bi_clr_used = BMP_CLR_USED;
    info_header.bi_clr_important = BMP_CLR_IMPORTANT;

    ofstream out(file, ios::binary);
    if (!out) {
        return false;
    }

    out.write(reinterpret_cast<const char*>(&file_header), sizeof(file_header));
    out.write(reinterpret_cast<const char*>(&info_header), sizeof(info_header));

    if (!out) {
        return false;
    }

    vector<char> row(stride, 0);

    for (int y = h - 1; y >= 0; --y) {
        const Color* line = image.GetLine(y);

        for (int x = 0; x < w; ++x) {
            row[x * 3 + 0] = static_cast<char>(line[x].b);
            row[x * 3 + 1] = static_cast<char>(line[x].g);
            row[x * 3 + 2] = static_cast<char>(line[x].r);
        }

        out.write(row.data(), stride);
        if (!out) {
            return false;
        }
    }
    return out.good();
}

Image LoadBMP(const Path& file) {
    ifstream in(file, ios::binary);
    if (!in) {
        return {};
    }

    BitmapFileHeader file_header{};
    BitmapInfoHeader info_header{};

    in.read(reinterpret_cast<char*>(&file_header), sizeof(file_header));
    in.read(reinterpret_cast<char*>(&info_header), sizeof(info_header));

    if (file_header.bf_type != BMP_SIGNATURE ||
        file_header.bf_off_bits != BMP_DATA_OFFSET ||
        info_header.bi_size != BMP_INFO_HEADER_SIZE ||
        info_header.bi_planes != BMP_PLANES ||
        info_header.bi_bit_count != BMP_BITS_PER_PIXEL ||
        info_header.bi_compression != BMP_COMPRESSION_NONE ||
        info_header.bi_width <= 0 || info_header.bi_height == 0) {
        return {};
    }
    const int w = info_header.bi_width;

    const bool top_down = info_header.bi_height < 0;
    const int h = top_down ? -info_header.bi_height : info_header.bi_height;

    const int stride = GetBMPStride(w);
    Image result(w, h, Color::Black());
    vector<char> row(stride);

    for (int y = 0; y < h; ++y) {
        in.read(row.data(), stride);
        if (!in) {
            return {};
        }
        const int image_y = top_down ? y : (h - 1 - y);
        Color* line = result.GetLine(image_y);

        for (int x = 0; x < w; ++x) {
            line[x].r =
                static_cast<byte>(static_cast<unsigned char>(row[x * 3 + 2]));
            line[x].g =
                static_cast<byte>(static_cast<unsigned char>(row[x * 3 + 1]));
            line[x].b =
                static_cast<byte>(static_cast<unsigned char>(row[x * 3 + 0]));
            line[x].a = byte{255};
        }
    }
    return result;
}

}  // namespace img_lib