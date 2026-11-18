#include "test_helpers.h"

#include <catch2/catch_test_macros.hpp>
#include <string>

TEST_CASE("Small JFIF, 4:2:0", "[baseline]") {
    CheckImage("small.jpg", ":)");
}

TEST_CASE("JFIF, 4:4:4", "[baseline]") {
    CheckImage("lenna.jpg");
}

TEST_CASE("Low-quality JFIF, 4:2:2", "[baseline]") {
    CheckImage("bad_quality.jpg", "so quality");
}

TEST_CASE("Tiny JFIF, 4:4:4", "[baseline]") {
    CheckImage("tiny.jpg");
}

TEST_CASE("EXIF, 4:2:2", "[baseline]") {
    CheckImage("chroma_halfed.jpg");
}

TEST_CASE("Grayscale EXIF", "[baseline]") {
    CheckImage("grayscale.jpg");
}

TEST_CASE("JFIF and EXIF, 4:2:0", "[baseline]") {
    CheckImage("test.jpg");
}

TEST_CASE("EXIF, 4:4:4", "[baseline]") {
    CheckImage("colors.jpg");
}

TEST_CASE("Photoshop JPEG, 4:4:4", "[baseline]") {
    CheckImage("save_for_web.jpg");
}

TEST_CASE("Reject malformed JPEG files", "[baseline][invalid]") {
    for (int number = 1; number <= 24; ++number) {
        DYNAMIC_SECTION("bad" << number << ".jpg") {
            CheckInvalidImage("bad" + std::to_string(number) + ".jpg");
        }
    }
}

TEST_CASE("Reject a missing file", "[baseline][invalid]") {
    CheckMissingImage("does_not_exist.jpg");
}

TEST_CASE("Reject an empty file", "[baseline][invalid]") {
    CheckInvalidImage("empty.jpg");
}

TEST_CASE("Reject a non-JPEG file", "[baseline][invalid]") {
    CheckInvalidImage("not_jpeg.jpg");
}

TEST_CASE("Reject a damaged SOI marker", "[baseline][invalid]") {
    CheckInvalidImage("damaged_soi.jpg");
}

TEST_CASE("Reject a truncated header", "[baseline][invalid]") {
    CheckInvalidImage("truncated_header.jpg");
}

TEST_CASE("Reject truncated scan data", "[baseline][invalid]") {
    CheckInvalidImage("truncated_scan.jpg");
}
