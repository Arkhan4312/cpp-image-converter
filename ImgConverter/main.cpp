#include <bmp_image.h>
#include <img_lib.h>
#include <jpeg_image.h>
#include <ppm_image.h>

#include <filesystem>
#include <iostream>
#include <string_view>

using namespace std;

namespace {

using img_lib::Image;
using img_lib::Path;

struct FormatHandler {
    Image (*load)(const Path&);
    bool (*save)(const Path&, const Image&);
};

const FormatHandler* FindFormatHandler(const Path& path) {
    static const FormatHandler ppm_handler{img_lib::LoadPPM, img_lib::SavePPM};
    static const FormatHandler jpeg_handler{img_lib::LoadJPEG,
                                            img_lib::SaveJPEG};
    static const FormatHandler bmp_handler{img_lib::LoadBMP, img_lib::SaveBMP};

    const string ext = path.extension().string();

    if (ext == ".ppm") {
        return &ppm_handler;
    }
    if (ext == ".jpg" || ext == ".jpeg") {
        return &jpeg_handler;
    }
    if (ext == ".bmp") {
        return &bmp_handler;
    }
    return nullptr;
}

int PrintUsageAndReturn(const char* program, int code) {
    cerr << "Usage: "sv << program << " <in_file> <out_file>"sv << endl;
    return code;
}
}  // namespace

int main(int argc, const char** argv) {
    if (argc != 3) {
        return PrintUsageAndReturn(argv[0], 1);
    }

    const Path in_path = argv[1];
    const Path out_path = argv[2];

    const FormatHandler* in_handler = FindFormatHandler(in_path);
    if (!in_handler) {
        cerr << "Unknown format of the input file"sv << endl;
        return 2;
    }

    const FormatHandler* out_handler = FindFormatHandler(out_path);
    if (!out_handler) {
        cerr << "Unknown format of the output file"sv << endl;
        return 3;
    }

    const Image image = in_handler->load(in_path);
    if (!image) {
        cerr << "Loading failed"sv << endl;
        return 4;
    }

    if (!out_handler->save(out_path, image)) {
        cerr << "Saving failed"sv << endl;
        return 5;
    }

    cout << "Successfully converted"sv << endl;
    return 0;
}