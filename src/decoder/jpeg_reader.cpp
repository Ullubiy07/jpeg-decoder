#include "jpeg_reader.h"

#include <stdexcept>

JpegReader::JpegReader(std::istream& input) : input_(input) {
}

int JpegReader::ReadBit() {
    if (cnt_ == 0) {
        byte_ = NextByte();
        cnt_ = 8;

        if (byte_ == 0xFF && NextByte() != 0x00) {
            throw std::runtime_error("Reader error: unexpected marker in bitstream");
        }
    }

    int bit = (byte_ >> 7);
    --cnt_;
    byte_ <<= 1;

    return bit;
}

uint8_t JpegReader::NextByte() {
    int byte = input_.get();

    if (input_.eof()) {
        throw std::runtime_error("Reader error: unexpected EOF in input stream");
    } else if (input_.fail()) {
        throw std::runtime_error("Reader error: unexpected read error in input stream");
    }

    return static_cast<uint8_t>(byte);
}

uint8_t JpegReader::ReadByte() {
    if (cnt_ != 0) {
        throw std::runtime_error("Reader error: unaligned byte read");
    }
    return NextByte();
}

uint16_t JpegReader::ReadWord() {
    uint8_t left_byte = ReadByte();
    uint8_t right_byte = ReadByte();
    return (static_cast<uint16_t>(left_byte) << 8) | right_byte;
}

void JpegReader::AlignToByte() {
    byte_ = 0;
    cnt_ = 0;
}
