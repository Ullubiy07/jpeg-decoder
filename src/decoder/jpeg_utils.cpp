#include "jpeg_utils.h"

#include <cmath>
#include <fftw3.h>

namespace {

int Extend(int value, uint8_t precision) {
    if (precision == 0) {
        return 0;
    }
    if (precision > 11) {
        throw std::invalid_argument("Invalid precision argument");
    }

    uint16_t vp = 1 << (precision - 1);
    if (value < vp) {
        value += (~0u << precision) + 1;
    }
    return value;
}

int Receive(JpegReader& reader, uint8_t size) {
    if (size > 11) {
        throw std::invalid_argument("Invalid size argument");
    }

    unsigned int value = 0;
    for (size_t i = 0; i < size; ++i) {
        value = (value << 1) | reader.ReadBit();
    }
    return static_cast<int>(value);
}

}  // namespace

namespace jpeg_utils {

int ReceiveAndExtend(JpegReader& reader, uint8_t size) {
    return Extend(Receive(reader, size), size);
}

int GetComponentIndexById(const std::vector<FrameComponent>& comps, uint8_t id) {
    for (size_t i = 0; i < comps.size(); ++i) {
        if (comps[i].id == id) {
            return i;
        }
    }
    throw std::runtime_error("Component not found");
}

void ApplyIDCT(DataUnit& data_unit) {
    static double* buf = reinterpret_cast<double*>(fftw_malloc(64 * sizeof(double)));

    static fftw_plan plan =
        fftw_plan_r2r_2d(8, 8, buf, buf, FFTW_REDFT01, FFTW_REDFT01, FFTW_ESTIMATE);

    static const double kEdgeFact = 16.0 * std::sqrt(2);
    static const double kWeight[64] = {
        32.0,      kEdgeFact, kEdgeFact, kEdgeFact, kEdgeFact, kEdgeFact, kEdgeFact, 32.0,
        kEdgeFact, 16.0,      16.0,      16.0,      16.0,      16.0,      16.0,      kEdgeFact,
        kEdgeFact, 16.0,      16.0,      16.0,      16.0,      16.0,      16.0,      kEdgeFact,
        kEdgeFact, 16.0,      16.0,      16.0,      16.0,      16.0,      16.0,      kEdgeFact,
        kEdgeFact, 16.0,      16.0,      16.0,      16.0,      16.0,      16.0,      kEdgeFact,
        kEdgeFact, 16.0,      16.0,      16.0,      16.0,      16.0,      16.0,      kEdgeFact,
        kEdgeFact, 16.0,      16.0,      16.0,      16.0,      16.0,      16.0,      kEdgeFact,
        32.0,      kEdgeFact, kEdgeFact, kEdgeFact, kEdgeFact, kEdgeFact, kEdgeFact, 32.0,
    };

    for (size_t i = 0; i < 64; ++i) {
        buf[i] = data_unit[i] * kWeight[i];
    }
    fftw_execute(plan);

    for (size_t i = 0; i < data_unit.size(); ++i) {
        data_unit[i] = static_cast<int>(buf[i] / 256.0);
    }
}

void ApplyLevelShift(DataUnit& data_unit, int level_shift, int max_val) {
    for (size_t i = 0; i < data_unit.size(); ++i) {
        data_unit[i] = std::min(std::max(data_unit[i] + level_shift, 0), max_val);
    }
}

RGB ConvertYCbCrToRGB(int y, int cb, int cr, int level_shift, int max_val) {
    RGB pixel;

    pixel.r = y + 1.402f * (cr - level_shift) + 0.5f;
    pixel.g = y - 0.34414f * (cb - level_shift) - 0.71414f * (cr - level_shift) + 0.5f;
    pixel.b = y + 1.772f * (cb - level_shift) + 0.5f;

    pixel.r = std::min(std::max(pixel.r, 0), max_val);
    pixel.g = std::min(std::max(pixel.g, 0), max_val);
    pixel.b = std::min(std::max(pixel.b, 0), max_val);

    return pixel;
}

void ConvertZigZagToLinear(DataUnit& zig_zag) {
    DataUnit temp = zig_zag;
    static const int kZigZagOrder[64] = {
        0,  1,  5,  6,  14, 15, 27, 28, 2,  4,  7,  13, 16, 26, 29, 42, 3,  8,  12, 17, 25, 30,
        41, 43, 9,  11, 18, 24, 31, 40, 44, 53, 10, 19, 23, 32, 39, 45, 52, 54, 20, 22, 33, 38,
        46, 51, 55, 60, 21, 34, 37, 47, 50, 56, 59, 61, 35, 36, 48, 49, 57, 58, 62, 63};

    for (size_t i = 0; i < 64; ++i) {
        zig_zag[i] = temp[kZigZagOrder[i]];
    }
}

void Dequantize(DataUnit& data_unit, const DataUnit& quant_table) {
    for (size_t i = 0; i < data_unit.size(); ++i) {
        data_unit[i] *= quant_table[i];
    }
}

}  // namespace jpeg_utils
