// clang-format off
#include <setjmp.h>
#include <stdio.h>
#include <jpeglib.h>
// clang-format on

#include <array>
#include <fstream>
#include <vector>

#include "ppm_image.h"

using namespace std;

namespace img_lib {
namespace {
struct ErrorManager {
    jpeg_error_mgr pub;
    jmp_buf setjmp_buffer;
};

void OnJpegError(j_common_ptr cinfo) {
    auto* err = reinterpret_cast<ErrorManager*>(cinfo->err);
    (*cinfo->err->output_message)(cinfo);
    longjmp(err->setjmp_buffer, 1);
}

FILE* OpenFileForRead(const Path& file) {
#ifdef _MSC_VER
    return _wfopen(file.wstring().c_str(), L"rb");
#else
    return fopen(file.string().c_str(), "rb");
#endif
}

FILE* OpenFileForWrite(const Path& file) {
#ifdef _MSC_VER
    return _wfopen(file.wstring().c_str(), L"wb");
#else
    return fopen(file.string().c_str(), "wb");
#endif
}

void StoreScanline(const JSAMPLE* row, int y, Image& out_image) {
    Color* line = out_image.GetLine(y);
    for (int x = 0; x < out_image.GetWidth(); ++x) {
        const JSAMPLE* pixel = row + x * 3;
        line[x] =
            Color{static_cast<byte>(pixel[0]), static_cast<byte>(pixel[1]),
                  static_cast<byte>(pixel[2]), byte{255}};
    }
}

}  // namespace

bool SaveJPEG(const Path& file, const Image& image) {
    jpeg_compress_struct cinfo;
    ErrorManager jerr;

    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = OnJpegError;

    FILE* outfile = nullptr;
    bool compress_created = false;

    if (setjmp(jerr.setjmp_buffer)) {
        if (compress_created) {
            jpeg_destroy_compress(&cinfo);
        }
        if (outfile != nullptr) {
            fclose(outfile);
        }
        return false;
    }

    jpeg_create_compress(&cinfo);
    compress_created = true;

    outfile = OpenFileForWrite(file);
    if (outfile == nullptr) {
        jpeg_destroy_compress(&cinfo);
        return false;
    }

    jpeg_stdio_dest(&cinfo, outfile);

    const int width = image.GetWidth();
    const int height = image.GetHeight();

    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;

    jpeg_set_defaults(&cinfo);
    jpeg_start_compress(&cinfo, TRUE);

    const int row_stride = width * 3;
    vector<JSAMPLE> row(row_stride);

    while (cinfo.next_scanline < cinfo.image_height) {
        const int y = static_cast<int>(cinfo.next_scanline);
        const Color* line = image.GetLine(y);

        for (int x = 0; x < width; ++x) {
            row[x * 3 + 0] = static_cast<unsigned char>(line[x].r);
            row[x * 3 + 1] = static_cast<unsigned char>(line[x].g);
            row[x * 3 + 2] = static_cast<unsigned char>(line[x].b);
        }

        JSAMPROW row_pointer = row.data();
        jpeg_write_scanlines(&cinfo, &row_pointer, 1);
    }

    jpeg_finish_compress(&cinfo);
    fclose(outfile);
    jpeg_destroy_compress(&cinfo);

    return true;
}

Image LoadJPEG(const Path& file) {
    jpeg_decompress_struct cinfo;
    ErrorManager jerr;

    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = OnJpegError;

    FILE* infile = nullptr;
    bool decompress_created = false;

    if (setjmp(jerr.setjmp_buffer)) {
        if (decompress_created) {
            jpeg_destroy_decompress(&cinfo);
        }
        if (infile != nullptr) {
            fclose(infile);
        }
        return {};
    }

    infile = OpenFileForRead(file);
    if (infile == nullptr) {
        return {};
    }

    jpeg_create_decompress(&cinfo);
    decompress_created = true;
    jpeg_stdio_src(&cinfo, infile);

    jpeg_read_header(&cinfo, TRUE);

    cinfo.out_color_space = JCS_RGB;
    cinfo.output_components = 3;

    jpeg_start_decompress(&cinfo);

    const int row_stride = cinfo.output_width * cinfo.output_components;
    JSAMPARRAY buffer = (*cinfo.mem->alloc_sarray)(
        reinterpret_cast<j_common_ptr>(&cinfo), JPOOL_IMAGE, row_stride, 1);

    Image result(cinfo.output_width, cinfo.output_height, Color::Black());

    while (cinfo.output_scanline < cinfo.output_height) {
        const int y = static_cast<int>(cinfo.output_scanline);
        jpeg_read_scanlines(&cinfo, buffer, 1);
        StoreScanline(buffer[0], y, result);
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(infile);

    return result;
}

}  // namespace img_lib