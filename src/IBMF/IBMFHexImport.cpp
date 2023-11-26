#include "IBMFHexImport.hpp"

#include <iomanip>
#include <map>

enum Position { NONE, LEFT, RIGHT, CENTER };

using PositionList = std::map<char32_t, Position>;

PositionList positionList = {
  { U'\U00002025', NONE  },
  { U'\U00002014', NONE  },
  { U'\U00002013', NONE  },
  { U'\U0000005F', NONE  },
  { U'\U0000005F', NONE  },
  { U'\U00000028', NONE  },
  { U'\U00000029', NONE  },
  { U'\U0000007B', NONE  },
  { U'\U0000007D', NONE  },
  { U'\U00003014', RIGHT },
  { U'\U00003015', LEFT  },
  { U'\U00003010', RIGHT },
  { U'\U00003011', LEFT  },
  { U'\U0000300A', RIGHT },
  { U'\U0000300B', LEFT  },
  { U'\U00003008', RIGHT },
  { U'\U00003009', LEFT  },
  { U'\U0000300C', RIGHT },
  { U'\U0000300D', LEFT  },
  { U'\U0000300E', RIGHT },
  { U'\U0000300F', LEFT  },
  { U'\U0000FE51', LEFT  },
  { U'\U0000005B', NONE  },
  { U'\U0000005D', NONE  },
  { U'\U0000203E', NONE  },
  { U'\U0000203E', NONE  },
  { U'\U0000203E', NONE  },
  { U'\U0000203E', NONE  },
  { U'\U0000005F', NONE  },
  { U'\U0000005F', NONE  },
  { U'\U0000005F', NONE  }
};

auto IBMFHexImport::readCodePoint(std::fstream &in, char32_t &codePoint,
                                  uint32_t &firstBytes) -> bool {

  auto hex = [](char a) -> uint8_t {
    return ((a >= '0') && (a <= '9')) ? a - '0' : a - 'A' + 10;
  };

  if (in.eof())
    return false;

  int i;
  in >> std::hex >> i;

  char ch;
  in >> ch;

  if (ch == ':') {
    codePoint = static_cast<char32_t>(i);
    char b1, b2, b3, b4;
    in >> b1 >> b2 >> b3 >> b4;
    firstBytes = (hex(b1) << 12) + (hex(b2) << 8) + (hex(b3) << 4) + hex(b4);
    in >> b1 >> b2 >> b3 >> b4;
    firstBytes = (firstBytes << 16) + (hex(b1) << 12) + (hex(b2) << 8) +
                 (hex(b3) << 4) + hex(b4);
    while (!in.eof() && (in.get() != '\n'))
      ;
    return true;
  }
  return false;
}

auto IBMFHexImport::readOneGlyph(std::fstream &in, char32_t &codePoint,
                                 BitmapPtr bitmap, int8_t &hOffset, int8_t &vOffset,
                                 uint16_t &advance) -> GlyphCode {
  int i;
  auto hex = [](char a) -> uint8_t {
    return ((a >= '0') && (a <= '9')) ? a - '0' : a - 'A' + 10;
  };

  in >> std::hex >> i;

  char ch;
  in >> ch;
  if (ch == ':') {
    codePoint = static_cast<char32_t>(i);
    GlyphCode glyphCode = toGlyphCode(codePoint);
    if (glyphCode == NO_GLYPH_CODE)
      goto noCode;

    std::vector<uint8_t> bytes;
    while (!in.eof() && (in.get() != '\n')) {
      in.unget();
      char b1, b2;
      in >> b1 >> b2;
      bytes.push_back((hex(b1) << 4) + hex(b2));
    }

    int byteWidth = (bytes.size() == 16) ? 1 : 2;
    int byteHeight = 16;

    advance = (byteWidth == 2) ? 16 : 8;

    if ((byteWidth * byteHeight) == bytes.size()) {
      int firstRow, lastRow, firstCol, lastCol;
      if (byteWidth == 1) {
        for (firstRow = 0; firstRow < 16; firstRow++) {
          if (bytes[firstRow] != 0)
            break;
        }
        if (firstRow >= 16)
          goto spaceCode;
        for (lastRow = 15; lastRow >= 0; lastRow--) {
          if (bytes[lastRow] != 0)
            break;
        }
        if (lastRow < 0)
          goto spaceCode; // Not really usefull...
      } else {
        for (firstRow = 0; firstRow < 16; firstRow++) {
          if ((bytes[firstRow << 1] != 0) || (bytes[(firstRow << 1) + 1] != 0))
            break;
        }
        if (firstRow >= 16)
          goto spaceCode;
        for (lastRow = 15; lastRow >= 0; lastRow--) {
          if ((bytes[lastRow << 1] != 0) || (bytes[(lastRow << 1) + 1] != 0))
            break;
        }
        if (lastRow < 0)
          goto spaceCode; // Not really usefull...
      }

      if (byteWidth == 1) {
        uint8_t mask = 0x80;
        firstCol = 0;
        for (int j = 0; j < 7; j++) {
          for (int i = firstRow; i <= lastRow; i++) {
            if (bytes[i] & mask)
              goto end1;
          }
          mask >>= 1;
          firstCol += 1;
        }
      end1:
        mask = 0x01;
        lastCol = 7;
        for (int j = 0; j < 7; j++) {
          for (int i = firstRow; i <= lastRow; i++) {
            if (bytes[i] & mask)
              goto end2;
          }
          mask <<= 1;
          lastCol -= 1;
        }
      } else {
        uint8_t mask = 0x80;
        firstCol = 0;
        for (int j = 0; j < 15; j++) {
          for (int i = firstRow; i <= lastRow; i++) {
            if (bytes[(i << 1) + (j >> 3)] & mask)
              goto end3;
          }
          mask >>= 1;
          if (mask == 0)
            mask = 0x80;
          firstCol += 1;
        }
      end3:
        mask = 0x01;
        lastCol = 15;
        for (int j = 15; j >= 0; j--) {
          for (int i = firstRow; i <= lastRow; i++) {
            if (bytes[(i << 1) + (j >> 3)] & mask)
              goto end4;
          }
          mask <<= 1;
          if (mask == 0)
            mask = 0x01;
          lastCol -= 1;
        }
      }

    end2:
    end4:
      bitmap->dim = Dim(lastCol - firstCol + 1, lastRow - firstRow + 1);
      vOffset = 14 - firstRow;

      uint8_t *buff = bytes.data() + (firstRow * byteWidth);
      for (int row = firstRow; row <= lastRow; row++) {
        uint8_t mask = 0x80 >> (firstCol & 7);
        for (int col = firstCol; col <= lastCol; col++) {
          uint8_t pixel = ((buff[col >> 3] & mask) == 0) ? 0 : 0xFF;
          bitmap->pixels.push_back(pixel);
          mask >>= 1;
          if (mask == 0)
            mask = 0x80;
        }
        buff += byteWidth;
      }

    } else {
      std::cout << "GNU Unifont Read Error!!!" << std::endl;
      return NO_GLYPH_CODE;
    }

    auto posit = positionList.find(codePoint);
    if ((posit != positionList.end()) && (posit->second == RIGHT)) {
      hOffset = - (advance - bitmap->dim.width - 1);
    } else {
      hOffset = 0;
    }

    return glyphCode;
  } else {

  noCode:
    while (!in.eof() && (in.get() != '\n'))
      ;
    return NO_GLYPH_CODE;

  spaceCode:
    bitmap->dim = Dim(0, 0);
    bitmap->pixels.clear();
    vOffset = 0;
    hOffset = 0;
    return SPACE_CODE;
  }

  return true;
}

// Returns true if the received character is part of the control characters nor
// one of the space characters as defined in Unicode.
auto IBMFHexImport::charSelected(char32_t ch, UBlocks &uBlocks,
                                 uint32_t firstBytes) const -> bool {
  // Don't populate with space and non-break-space characters
  if ((ch >= 0x0021) && (ch != 0x00A0) && ((ch < 0x02000) || (ch > 0x200F)) &&
      ((ch < 0x02028) || (ch > 0x202F)) && ((ch < 0x0205F) || (ch > 0x206F)) &&
      (firstBytes != 0xAAAA0001) /* && (firstBytes != 0x00007FFE) */) {
    for (auto &uBlockDef : uBlocks) {
      if ((ch >= uBlockDef.first_) && (ch <= uBlockDef.last_)) {
        return true;
      }
    }
  }
  return false;
}

auto IBMFHexImport::prepareCodePlanes(std::fstream &in, UBlocks &uBlocks)
    -> int {

  uint16_t glyphCode = 0;

  for (int i = 0; i < 4; i++) {
    planes_.push_back(Plane({0, 0, 0}));
  }

  bool firstSelected = false;
  char16_t currCodePoint = 0;
  int currPlaneIdx = 0;
  int currCodePointBundleIdx = 0;
  char32_t codePoint;
  uint32_t firstBytes;

  while (readCodePoint(in, codePoint, firstBytes)) {
    if (charSelected(codePoint, uBlocks, firstBytes)) {
      int planeIdx = codePoint >> 16;
      if (planeIdx < 4) { // Only the first 4 planes are managed
        char16_t u16 = static_cast<char16_t>(codePoint & 0x0000FFFF);
        if (!firstSelected) {
          currCodePoint = u16;
          currPlaneIdx = planeIdx;
          planes_[planeIdx] = Plane{
              .codePointBundlesIdx =
                  static_cast<uint16_t>(codePointBundles_.size()),
              .entriesCount = 1,
              .firstGlyphCode = 0,
          };
          currCodePointBundleIdx = codePointBundles_.size();
          CodePointBundle bundle =
              CodePointBundle({.firstCodePoint = u16, .lastCodePoint = u16});
          codePointBundles_.push_back(bundle);
          firstSelected = true;
        } else {
          if (planeIdx != currPlaneIdx) {
            for (int idx = planeIdx + 1; idx <= currPlaneIdx; idx++) {
              planes_[idx].codePointBundlesIdx = codePointBundles_.size();
            }
            planes_[planeIdx] =
                Plane{.codePointBundlesIdx =
                          static_cast<uint16_t>(codePointBundles_.size()),
                      .entriesCount = 1,
                      .firstGlyphCode = glyphCode};
            currCodePointBundleIdx = codePointBundles_.size();
            CodePointBundle bundle =
                CodePointBundle({.firstCodePoint = u16, .lastCodePoint = u16});
            codePointBundles_.push_back(bundle);
            currCodePoint = u16;
            currPlaneIdx = planeIdx;
          } else {
            if (u16 == (currCodePoint + 1)) {
              codePointBundles_[currCodePointBundleIdx].lastCodePoint = u16;
            } else {
              currCodePointBundleIdx = codePointBundles_.size();
              CodePointBundle bundle = CodePointBundle(
                  {.firstCodePoint = u16, .lastCodePoint = u16});
              codePointBundles_.push_back(bundle);
              planes_[currPlaneIdx].entriesCount += 1;
            }
            currCodePoint = u16;
          }
        }
      }
      glyphCode += 1;
    }
  }
  // Completes the info of planes not used
  for (int idx = currPlaneIdx + 1; idx < 4; idx++) {
    planes_[idx].codePointBundlesIdx = codePointBundles_.size();
    planes_[idx].firstGlyphCode = glyphCode;
  }

  return glyphCode;
}

auto IBMFHexImport::loadHex(std::string filename, UBlocks &uBlocs) -> bool {

  clear();

  // ----- Preamble -----

  // clang-format off
  preamble_ = {
      .marker    = {'I', 'B', 'M', 'F'},
      .faceCount = 1,
      .bits      = {.version = IBMF_VERSION, .fontFormat = FontFormat::UTF32}
  };
  // clang-format on

  std::fstream in;
  in.open(filename, std::ios::in);

  if (in.is_open()) {

    int glyphCount = prepareCodePlanes(in, uBlocs);

    if (glyphCount <= 0) {
      in.close();
      return false;
    }

    FacePtr face = FacePtr(new Face);

    in.close();
    in.open(filename, std::ios::in);

    while (!in.eof()) {
      char32_t codePoint;
      auto bitmap = BitmapPtr(new Bitmap());
      int8_t hOffset, vOffset;
      GlyphCode glyphCode;
      uint16_t advance;

      if ((glyphCode = readOneGlyph(in, codePoint, bitmap, hOffset, vOffset, advance)) !=
          NO_GLYPH_CODE) {

        face->bitmaps.push_back(bitmap);

        GlyphLigKernPtr glyphLigKern = GlyphLigKernPtr(new GlyphLigKern);

        // Create ligatures for the glyph if available
        // Ensure that both next and replacement glyph codes are present in the
        // resulting IBMF font
        char32_t firstChar = getUTF32(glyphCode);
        for (auto &ligature : ligatures) {
          if (ligature.firstChar == firstChar) {
            GlyphCode nextGlyphCode = toGlyphCode(ligature.nextChar);
            GlyphCode replacementGlyphCode = toGlyphCode(ligature.replacement);
            if ((nextGlyphCode != NO_GLYPH_CODE) &&
                (replacementGlyphCode != NO_GLYPH_CODE)) {
              glyphLigKern->ligSteps.push_back(
                  GlyphLigStep{.nextGlyphCode = nextGlyphCode,
                               .replacementGlyphCode = replacementGlyphCode});
            }
          }
        }

        face->glyphsLigKern.push_back(glyphLigKern);

        // ----- Glyph Info -----

        GlyphInfoPtr glyphInfo = GlyphInfoPtr(new GlyphInfo(GlyphInfo{
            .bitmapWidth = static_cast<uint8_t>(bitmap->dim.width),
            .bitmapHeight = static_cast<uint8_t>(bitmap->dim.height),
            .horizontalOffset = static_cast<int8_t>(hOffset),
            .verticalOffset = static_cast<int8_t>(vOffset),
            .packetLength =
                static_cast<uint16_t>(bitmap->dim.width * bitmap->dim.height),
            .advance = static_cast<FIX16>(
                ((codePoint < 0x2E80) ||
                         ((codePoint >= 0xA000) && (codePoint < 0xFE10)) ||
                         ((codePoint >= 0xFE70) && (codePoint < 0xFF00))
                     ? (bitmap->dim.width + 1)
                     : advance)
                << 6),
            .rleMetrics = RLEMetrics{.dynF = 0,
                                     .firstIsBlack = false,
                                     .beforeAddedOptKern = 0,
                                     .afterAddedOptKern = 0},
            .ligKernPgmIndex = 0, // completed at save time
            .mainCode = glyphCode // No composite management (for now)
        }));

        face->glyphs.push_back(glyphInfo);
      }
    }

    in.close();

    // ----- Face Header -----

    face->header = FaceHeaderPtr(new FaceHeader({
        .pointSize = 10,
        .lineHeight = static_cast<uint8_t>(16),
        .dpi = static_cast<uint16_t>(75),
        .xHeight = static_cast<FIX16>(8 << 6),
        .emSize = static_cast<FIX16>(10 << 6),
        .slantCorrection = 0, // not available for FreeType
        .descenderHeight = static_cast<uint8_t>(2),
        .spaceSize = 5,
        .glyphCount = static_cast<uint16_t>(glyphCount),
        .ligKernStepCount = 0, // will be set at save time
        .pixelsPoolSize = 0,   // will be set at save time
    }));
    faces_.push_back(std::move(face));

    return true;
  }

  return false;
}
