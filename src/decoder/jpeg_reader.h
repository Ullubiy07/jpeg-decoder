#pragma once

#include "huffman.h"

#include <iostream>
#include <cstdint>

class JpegReader : public IBitReader {
public:
    JpegReader(std::istream& input);
    int ReadBit() override;

    uint8_t ReadByte();
    uint16_t ReadWord();
    void AlignToByte();

private:
    uint8_t NextByte();

    std::istream& input_;
    int cnt_ = 0;
    uint8_t byte_ = 0;
};
