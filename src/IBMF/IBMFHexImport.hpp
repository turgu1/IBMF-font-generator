#pragma once

#include <fstream>
#include <iostream>

#include "IBMFFontMod.hpp"

class IBMFHexImport : public IBMFFontMod {
public:
  IBMFHexImport() : IBMFFontMod() {}

  auto charSelected(char32_t ch, UBlocks &uBlocks, uint32_t firstBytes) const
      -> bool;
  auto prepareCodePlanes(std::fstream &in, UBlocks &uBlocks) -> int;
  auto readCodePoint(std::fstream &in, char32_t &codePoint,
                     uint32_t &firstBytes) -> bool;
  auto readOneGlyph(std::fstream &in, char32_t &codePoint, BitmapPtr bitmap,
                    int8_t &vOffset, uint16_t &advance) -> GlyphCode;
  auto loadHex(std::string filename, UBlocks &uBlocks) -> bool;
};

typedef std::shared_ptr<IBMFHexImport> IBMFHexImportPtr;
