#pragma once

#include "jpeg_decoder.h"

#include <cstdint>
#include <vector>

namespace jpeg_utils {

int ReceiveAndExtend(JpegReader& reader, uint8_t size);

int GetComponentIndexById(const std::vector<FrameComponent>& comps, uint8_t id);

void ApplyIDCT(DataUnit& data_unit);
void ApplyLevelShift(DataUnit& data_unit, int level_shift, int max_val);

void ConvertZigZagToLinear(DataUnit& zig_zag);
RGB ConvertYCbCrToRGB(int y, int cb, int cr, int level_shift, int max_val);

void Dequantize(DataUnit& data_unit, const DataUnit& quant_table);

}  // namespace jpeg_utils
