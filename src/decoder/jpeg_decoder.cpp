#include "jpeg_decoder.h"
#include "jpeg_utils.h"

#include <cstdint>
#include <stdexcept>

enum class Marker : uint8_t {
    SOI = 0xD8,

    COM = 0xFE,
    APP0 = 0xE0,
    APP15 = 0xEF,

    DHT = 0xC4,
    DQT = 0xDB,
    SOF0 = 0xC0,
    SOS = 0xDA,

    EOI = 0xD9,
};

JpegDecoder::JpegDecoder(std::istream& input) : reader_(input) {
}

Image JpegDecoder::Decode() {
    Image image;

    ParseSegments();
    image.SetSize(header_.width, header_.height);
    image.SetComment(std::move(comment_));

    Reconstruct(image);
    return image;
}

void JpegDecoder::ParseSegments() {
    if (reader_.ReadByte() != 0xFF) {
        throw std::runtime_error("Missing 0xFF as first byte");
    }
    Marker marker = static_cast<Marker>(reader_.ReadByte());
    if (marker != Marker::SOI) {
        throw std::runtime_error("Missing SOI marker");
    }

    while (true) {
        reader_.ReadByte();
        uint8_t byte = reader_.ReadByte();
        marker = static_cast<Marker>(byte);

        switch (marker) {
            case Marker::SOI:
                throw std::runtime_error("SOI marker dublicate");
            case Marker::COM:
                ParseCOM();
                break;
            case Marker::DHT:
                ParseDHT();
                break;
            case Marker::DQT:
                ParseDQT();
                break;
            case Marker::SOF0:
                ParseSOF0();
                break;
            case Marker::SOS:
                ParseSOS();
                break;
            case Marker::EOI:;
                return;
            default:
                if (Marker::APP0 <= marker && marker <= Marker::APP15) {
                    ParseAPPn();
                } else {
                    throw std::runtime_error("Unsupported marker " + std::format("'{:X}'", byte));
                }
                break;
        }
    }
}

void JpegDecoder::ParseCOM() {
    uint16_t length = reader_.ReadWord();
    if (length < 2) {
        throw std::runtime_error("Error parsing COM: invalid length");
    }

    comment_.reserve(length - 2);
    for (size_t i = 2; i < length; ++i) {
        comment_ += static_cast<char>(reader_.ReadByte());
    }
}

void JpegDecoder::ParseAPPn() {
    uint16_t length = reader_.ReadWord();
    if (length < 2) {
        throw std::runtime_error("Error parsing APPn: invalid length");
    }

    for (size_t i = 2; i < length; ++i) {
        reader_.ReadByte();
    }
}

void JpegDecoder::ParseDHT() {
    uint16_t length = reader_.ReadWord();
    size_t bytes_read = 2;

    std::vector<int> bits(17);

    while (bytes_read < length) {
        uint8_t type = reader_.ReadByte();
        int table_class = type >> 4;
        int table_id = type & 0x0F;

        if (table_class > 1) {
            throw std::runtime_error("Error parsing DHT: invalid table class");
        }
        if (table_id > 1) {
            throw std::runtime_error("Error parsing DHT: invalid table id");
        }

        size_t value_count = 0;
        for (size_t i = 1; i < bits.size(); ++i) {
            bits[i] = reader_.ReadByte();
            value_count += bits[i];
        }
        if (value_count > 256) {
            throw std::runtime_error("Error parsing DHT: too many values");
        }

        std::vector<uint8_t> huff_val(value_count);
        for (size_t i = 0; i < value_count; ++i) {
            huff_val[i] = reader_.ReadByte();
        }
        bytes_read += value_count + 17;

        decoder_.at(table_class).at(table_id).Init(bits, std::move(huff_val));
    }

    if (bytes_read != length) {
        throw std::runtime_error("Error parsing DHT: invalid length");
    }
    dht_processed_ = true;
}

void JpegDecoder::ParseDQT() {
    uint16_t length = reader_.ReadWord();
    size_t bytes_read = 2;

    while (bytes_read < length) {
        uint8_t byte = reader_.ReadByte();
        uint8_t precision = byte >> 4;
        uint8_t table_id = byte & 0x0F;

        if (precision != 0) {
            throw std::runtime_error("Error parsing DQT: invalid precision");
        }
        if (table_id > 3) {
            throw std::runtime_error("Error parsing DQT: invalid table id");
        }

        for (size_t i = 0; i < 64; ++i) {
            uint8_t el = reader_.ReadByte();
            if (el == 0) {
                throw std::runtime_error("Error parsing DQT: invalid quantization table element");
            }
            quant_table_[table_id][i] = el;
        }
        jpeg_utils::ConvertZigZagToLinear(quant_table_[table_id]);
        bytes_read += 65;
    }

    if (bytes_read != length) {
        throw std::runtime_error("Error parsing DQT: invalid length");
    }
    dqt_processed_ = true;
}

void JpegDecoder::ParseSOF0() {
    if (sof0_processed_) {
        throw std::runtime_error("Error parsing SOF0: dublicate marker");
    }

    uint16_t length = reader_.ReadWord();
    uint8_t precision = reader_.ReadByte();
    header_.height = reader_.ReadWord();
    header_.width = reader_.ReadWord();
    uint8_t comp_count = reader_.ReadByte();

    if (length != 8 + 3 * comp_count) {
        throw std::runtime_error("Error parsing SOF0: invalid length");
    }
    if (precision != 8) {
        throw std::runtime_error("Error parsing SOF0: invalid precision");
    }
    if (header_.height == 0) {
        throw std::runtime_error("Error parsing SOF0: invalid height");
    }
    if (header_.width == 0) {
        throw std::runtime_error("Error parsing SOF0: invalid width");
    }
    if (comp_count != 1 && comp_count != 3) {
        throw std::runtime_error("Error parsing SOF0: invalid number of components");
    }

    header_.level_shift = 1 << (precision - 1);
    header_.max_value = (1 << precision) - 1;
    header_.comps.resize(comp_count);
    int h_max = 0;
    int v_max = 0;

    for (auto& comp : header_.comps) {
        comp.id = reader_.ReadByte();
        uint8_t byte = reader_.ReadByte();
        comp.quant_table_id = reader_.ReadByte();
        comp.h_factor = byte >> 4;
        comp.v_factor = byte & 0x0F;

        if (comp.h_factor == 0 || comp.h_factor > 4) {
            throw std::runtime_error("Error parsing SOF0: invalid horizontal sampling factor");
        }
        if (comp.v_factor == 0 || comp.v_factor > 4) {
            throw std::runtime_error("Error parsing SOF0: invalid vertical sampling factor");
        }
        if (comp.quant_table_id > 3) {
            throw std::runtime_error("Error parsing SOF0: invalid quantization table id");
        }

        h_max = std::max(h_max, comp.h_factor);
        v_max = std::max(v_max, comp.v_factor);
    }

    int mcu_width = h_max * 8;
    int mcu_height = v_max * 8;

    header_.max_h_factor = h_max;
    header_.max_v_factor = v_max;
    header_.mcu_cols = (header_.width + mcu_width - 1) / mcu_width;
    header_.mcu_rows = (header_.height + mcu_height - 1) / mcu_height;

    size_t i = 0;
    for (auto& comp : header_.comps) {
        comp.height = header_.mcu_rows * comp.v_factor * 8;
        comp.width = header_.mcu_cols * comp.h_factor * 8;
        components_[i++].resize(comp.height, std::vector<int>(comp.width));
    }

    sof0_processed_ = true;
}

void JpegDecoder::ParseSOS() {
    if (!dht_processed_ || !dqt_processed_ || !sof0_processed_) {
        throw std::runtime_error(
            "Error parsing SOS: reached before processing DHT, DQT and SOF0 segments");
    }

    uint16_t length = reader_.ReadWord();
    uint8_t scan_comps = reader_.ReadByte();

    if (length != 6 + 2 * scan_comps) {
        throw std::runtime_error("Error parsing SOS: invalid length");
    }
    if (scan_comps != 1 && scan_comps != 3) {
        throw std::runtime_error("Error parsing SOS: invalid scan component count");
    }

    ScanHeader scan_header;
    scan_header.comps.resize(scan_comps);

    for (auto& comp : scan_header.comps) {
        comp.id = reader_.ReadByte();
        uint8_t byte = reader_.ReadByte();
        comp.dc_table_id = byte >> 4;
        comp.ac_table_id = byte & 0x0F;

        if (comp.dc_table_id > 1) {
            throw std::runtime_error("Error parsing SOS: invalid table id for DC coefficient");
        }
        if (comp.ac_table_id > 1) {
            throw std::runtime_error("Error parsing SOS: invalid table id for AC coefficient");
        }
    }

    uint8_t ss = reader_.ReadByte();
    uint8_t se = reader_.ReadByte();
    uint8_t a = reader_.ReadByte();
    if (ss != 0 || se != 63 || a != 0) {
        throw std::runtime_error("Error parsing SOS: invalid fields ss, se, ah or al");
    }

    std::array<int, 3> dc_pred = {0, 0, 0};
    size_t mcu_count;
    scan_header.is_interleaved = (scan_comps == 3 && header_.comps.size() == 3);

    if (scan_header.is_interleaved) {
        mcu_count = header_.mcu_cols * header_.mcu_rows;
    } else {
        int comp_index = jpeg_utils::GetComponentIndexById(header_.comps, scan_header.comps[0].id);
        const auto& comp = header_.comps[comp_index];
        mcu_count = ((comp.height + 7) / 8) * ((comp.width + 7) / 8);
    }

    for (size_t i = 0; i < mcu_count; ++i) {
        DecodeMCU(scan_header, dc_pred, i);
    }
    reader_.AlignToByte();
}

void JpegDecoder::Reconstruct(Image& image) {
    int y_cb, x_cb, y_cr, x_cr, cb, cr;

    for (int y = 0; y < header_.height; ++y) {
        for (int x = 0; x < header_.width; ++x) {
            if (header_.comps.size() == 1) {
                cb = header_.level_shift;
                cr = header_.level_shift;
            } else {
                y_cb = (y * header_.comps[1].v_factor) / header_.max_v_factor;
                x_cb = (x * header_.comps[1].h_factor) / header_.max_h_factor;
                y_cr = (y * header_.comps[2].v_factor) / header_.max_v_factor;
                x_cr = (x * header_.comps[2].h_factor) / header_.max_h_factor;

                cb = components_[1][y_cb][x_cb];
                cr = components_[2][y_cr][x_cr];
            }

            RGB pixel = jpeg_utils::ConvertYCbCrToRGB(components_[0][y][x], cb, cr,
                                                      header_.level_shift, header_.max_value);
            image.SetPixel(y, x, pixel);
        }
    }
}

void JpegDecoder::DecodeMCU(const ScanHeader& scan_header, std::array<int, 3>& dc_pred,
                            size_t mcu_index) {
    for (const auto& scan_comp : scan_header.comps) {
        int comp_index = jpeg_utils::GetComponentIndexById(header_.comps, scan_comp.id);
        const auto& comp = header_.comps[comp_index];
        size_t du_count = (scan_header.is_interleaved ? comp.h_factor * comp.v_factor : 1);

        size_t mcu_row_offset = 0;
        size_t mcu_col_offset = 0;
        if (scan_header.is_interleaved) {
            mcu_row_offset = 8 * comp.v_factor * (mcu_index / header_.mcu_cols);
            mcu_col_offset = 8 * comp.h_factor * (mcu_index % header_.mcu_cols);
        } else {
            int du_per_row = (comp.width + 7) / 8;
            mcu_row_offset = 8 * (mcu_index / du_per_row);
            mcu_col_offset = 8 * (mcu_index % du_per_row);
        }

        for (size_t j = 0; j < du_count; ++j) {
            auto du =
                DecodeDataUnit(dc_pred[comp_index], scan_comp.dc_table_id, scan_comp.ac_table_id);
            dc_pred[comp_index] = du[0];
            jpeg_utils::Dequantize(du, quant_table_[comp.quant_table_id]);
            jpeg_utils::ApplyIDCT(du);
            jpeg_utils::ApplyLevelShift(du, header_.level_shift, header_.max_value);

            size_t du_row_offset = mcu_row_offset;
            size_t du_col_offset = mcu_col_offset;
            if (scan_header.is_interleaved) {
                du_row_offset += 8 * (j / comp.h_factor);
                du_col_offset += 8 * (j % comp.h_factor);
            }

            for (size_t y = du_row_offset, idx = 0; y < du_row_offset + 8; ++y) {
                for (size_t x = du_col_offset; x < du_col_offset + 8; ++x, ++idx) {
                    components_[comp_index][y][x] = du[idx];
                }
            }
        }
    }
}

DataUnit JpegDecoder::DecodeDataUnit(int dc_pred, uint8_t dc_table_id, uint8_t ac_table_id) {
    DataUnit zig_zag{};
    uint8_t size = decoder_.at(0).at(dc_table_id).Decode(reader_);
    int dc_diff = jpeg_utils::ReceiveAndExtend(reader_, size);
    size_t pos = 1;

    zig_zag[0] = dc_pred + dc_diff;

    while (pos < zig_zag.size()) {
        uint8_t rs = decoder_.at(1).at(ac_table_id).Decode(reader_);
        uint8_t run_length = rs >> 4;
        size = rs & 0x0F;

        if (size == 0) {
            if (run_length == 0) {
                break;
            }  // EOB
            if (run_length == 15) {
                pos += 16;
                continue;
            }  // ZRL
            throw std::runtime_error("Invalid run length for size category = 0");
        }
        pos += run_length;
        if (pos < zig_zag.size()) {
            zig_zag[pos] = jpeg_utils::ReceiveAndExtend(reader_, size);
        }
        ++pos;
    }

    jpeg_utils::ConvertZigZagToLinear(zig_zag);
    return zig_zag;
}
