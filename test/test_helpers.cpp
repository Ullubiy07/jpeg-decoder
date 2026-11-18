#include "test_helpers.h"
#include "decoder.h"

#include <cstddef>
#include <cstdio>
#include <jpeglib.h>
#include <png.h>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cmath>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace {

const std::filesystem::path kImages =
    std::filesystem::path(__FILE__).parent_path() / "images";

Image ReadJpg(const std::filesystem::path& path) {
    const auto filename = path.string();
    const auto closeFile = [](FILE* file) { std::fclose(file); };
    std::unique_ptr<FILE, decltype(closeFile)> file(
        std::fopen(filename.c_str(), "rb"), closeFile);
    REQUIRE(file != nullptr);

    jpeg_decompress_struct reader{};
    jpeg_error_mgr errors{};
    reader.err = jpeg_std_error(&errors);
    jpeg_create_decompress(&reader);
    jpeg_stdio_src(&reader, file.get());
    jpeg_read_header(&reader, TRUE);
    reader.out_color_space = JCS_RGB;
    reader.do_fancy_upsampling = FALSE;
    reader.dct_method = JDCT_ISLOW;
    jpeg_start_decompress(&reader);

    Image expected(reader.output_width, reader.output_height);
    std::vector<JSAMPLE> row(reader.output_width * 3);
    while (reader.output_scanline < reader.output_height) {
        const auto y = reader.output_scanline;
        auto* data = row.data();
        jpeg_read_scanlines(&reader, &data, 1);
        for (std::size_t x = 0; x < expected.Width(); ++x) {
            expected.SetPixel(y, x, {row[x * 3], row[x * 3 + 1], row[x * 3 + 2]});
        }
    }
    jpeg_finish_decompress(&reader);
    jpeg_destroy_decompress(&reader);
    return expected;
}

void Compare(const Image& actual, const Image& expected) {
    REQUIRE(actual.Width() == expected.Width());
    REQUIRE(actual.Height() == expected.Height());
    REQUIRE(actual.Width() > 0);
    REQUIRE(actual.Height() > 0);

    double squaredError = 0;
    for (std::size_t y = 0; y < actual.Height(); ++y) {
        for (std::size_t x = 0; x < actual.Width(); ++x) {
            const RGB actualPixel = actual.GetPixel(y, x);
            const RGB expectedPixel = expected.GetPixel(y, x);
            const std::array channels{actualPixel.r, actualPixel.g, actualPixel.b};
            const std::array targets{expectedPixel.r, expectedPixel.g, expectedPixel.b};
            for (std::size_t c = 0; c < channels.size(); ++c) {
                REQUIRE(channels[c] >= 0);
                REQUIRE(channels[c] <= 255);
                const double delta = channels[c] - targets[c];
                squaredError += delta * delta;
            }
        }
    }
    const double rmse = std::sqrt(squaredError /
        (static_cast<double>(actual.Width()) * actual.Height() * 3));
    INFO("RGB root mean square error: " << rmse);
    CHECK(rmse <= 3.0);
}

void WritePng(const std::filesystem::path& path, const Image& image) {
    std::vector<png_byte> pixels;
    pixels.reserve(image.Width() * image.Height() * 3);
    for (std::size_t y = 0; y < image.Height(); ++y) {
        for (std::size_t x = 0; x < image.Width(); ++x) {
            const RGB pixel = image.GetPixel(y, x);
            pixels.push_back(static_cast<png_byte>(pixel.r));
            pixels.push_back(static_cast<png_byte>(pixel.g));
            pixels.push_back(static_cast<png_byte>(pixel.b));
        }
    }

    png_image output{};
    output.version = PNG_IMAGE_VERSION;
    output.width = static_cast<png_uint_32>(image.Width());
    output.height = static_cast<png_uint_32>(image.Height());
    output.format = PNG_FORMAT_RGB;
    const auto filename = path.string();
    const bool saved = png_image_write_to_file(
        &output, filename.c_str(), 0, pixels.data(), 0, nullptr);
    const std::string error = output.message;
    png_image_free(&output);
    INFO("PNG output: " << filename << ": " << error);
    REQUIRE(saved);
}

}  // namespace

void CheckImage(std::string_view filename, std::string_view comment) {
    const auto path = kImages / filename;
    INFO("Input: " << path.string());
    REQUIRE(std::filesystem::is_regular_file(path));
    const Image actual = Decode(path);
    CHECK(actual.GetComment() == comment);
    Compare(actual, ReadJpg(path));

    auto output = path;
    output.replace_extension("png");
    WritePng(output, actual);
}

void CheckInvalidImage(std::string_view filename) {
    const auto path = kImages / "bad" / filename;
    INFO("Invalid input: " << path.string());
    REQUIRE(std::filesystem::is_regular_file(path));
    REQUIRE_THROWS_AS(Decode(path), std::exception);
}

void CheckMissingImage(std::string_view filename) {
    const auto path = kImages / filename;
    REQUIRE_FALSE(std::filesystem::exists(path));
    REQUIRE_THROWS_AS(Decode(path), std::exception);
}
