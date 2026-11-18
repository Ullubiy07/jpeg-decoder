/*
JPEG image decoder.

Implements the ISO/IEC 10918-1 : 1993(E) / ITU-T.81 technical specification.
Supported modes: baseline sequential
*/

#pragma once

#include "huffman.h"
#include "jpeg_reader.h"

#include "image.h"

#include <array>
#include <cstdint>
#include <vector>

struct FrameComponent {
    int id;
    int h_factor;  // number of horizontal 8*8 blocks in each MCU
    int v_factor;  // number of vertical 8*8 blocks in each MCU
    int quant_table_id;

    int width;
    int height;
};

struct ScanComponent {
    int id;
    int dc_table_id;
    int ac_table_id;
};

struct FrameHeader {
    std::vector<FrameComponent> comps;
    int height;
    int width;

    int level_shift;
    int max_value;
    int mcu_cols;
    int mcu_rows;
    int max_h_factor;
    int max_v_factor;
};

struct ScanHeader {
    std::vector<ScanComponent> comps;
    bool is_interleaved;
};

using DataUnit = std::array<int, 64>;

class JpegDecoder {
public:
    JpegDecoder(std::istream& input);
    Image Decode();

private:
    void ParseSegments();
    void ParseCOM();
    void ParseAPPn();
    void ParseDHT();
    void ParseDQT();
    void ParseSOF0();
    void ParseSOS();

    void DecodeMCU(const ScanHeader& scan_header, std::array<int, 3>& dc_pred, size_t mcu_index);
    DataUnit DecodeDataUnit(int dc_pred, uint8_t dc_table_id, uint8_t ac_table_id);

    void Reconstruct(Image& image);

    JpegReader reader_;
    FrameHeader header_;

    std::array<DataUnit, 4> quant_table_;
    std::array<std::vector<std::vector<int>>, 3> components_;
    std::array<std::array<HuffmanDecoder<uint8_t>, 2>, 2> decoder_;
    std::string comment_;

    bool dht_processed_ = false;
    bool dqt_processed_ = false;
    bool sof0_processed_ = false;
};
