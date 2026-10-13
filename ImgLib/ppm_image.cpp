#include "ppm_image.h"

#include <fstream>
#include <string>
#include <string_view>
#include <vector>
using namespace std;

namespace img_lib {

namespace {

constexpr string_view PPM_SIGNATURE = "P6"sv;
constexpr int PPM_MAX_COLOR = 255;
constexpr int CHANNELS_COUNT = 3;

}  // namespace

bool SavePPM(const Path& file, const Image& image) {
    const int w = image.GetWidth();
    const int h = image.GetHeight();
    if (w <= 0 || h <= 0) {
        return false;
    }

    ofstream out(file, ios::binary);
    if (!out) {
        return false;
    }

    out << PPM_SIGNATURE << '\n'
        << w << ' ' << h << '\n'
        << PPM_MAX_COLOR << '\n';
    if (!out) {
        return false;
    }

    const int row_size = w * CHANNELS_COUNT;
    vector<char> row(row_size);

    for (int y = 0; y < h; ++y) {
        const Color* line = image.GetLine(y);
        for (int x = 0; x < w; ++x) {
            row[x * CHANNELS_COUNT + 0] = static_cast<char>(line[x].r);
            row[x * CHANNELS_COUNT + 1] = static_cast<char>(line[x].g);
            row[x * CHANNELS_COUNT + 2] = static_cast<char>(line[x].b);
        }

        out.write(row.data(), row_size);
        if (!out) {
            return false;
        }
    }

    return out.good();
}

Image LoadPPM(const Path& file) {
    ifstream in(file, ios::binary);
    if (!in) {
        return {};
    }
    string signature;
    int w = 0;
    int h = 0;
    int color_max = 0;

    in >> signature >> w >> h >> color_max;
    if (!in) {
        return {};
    }

    if (signature != PPM_SIGNATURE || color_max != PPM_MAX_COLOR || w <= 0 ||
        h <= 0) {
        return {};
    }

    if (in.get() != '\n') {
        return {};
    }
    Image result(w, h, Color::Black());

    const int row_size = w * CHANNELS_COUNT;
    vector<char> row(row_size);

    for (int y = 0; y < h; ++y) {
        in.read(row.data(), row_size);
        if (!in) {
            return {};
        }
        Color* line = result.GetLine(y);
        for (int x = 0; x < w; ++x) {
            line[x].r = static_cast<byte>(
                static_cast<unsigned char>(row[x * CHANNELS_COUNT + 0]));
            line[x].g = static_cast<byte>(
                static_cast<unsigned char>(row[x * CHANNELS_COUNT + 1]));
            line[x].b = static_cast<byte>(
                static_cast<unsigned char>(row[x * CHANNELS_COUNT + 2]));
            line[x].a = byte{255};
        }
    }
    return result;
}

}  // namespace img_lib