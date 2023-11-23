#pragma once

#include <cinttypes>
#include <memory>
#include <vector>

#include "../Unicode/UBlocks.hpp"

// clang-format off
//
// The following definitions are used by all parts of the driver.
//
// The IBMF font files have the current format:
//
// 1. LATIN and UTF32 formats:
//
//  At Offset 0:
//  +--------------------+
//  |                    |  Preamble (6 bytes)
//  |                    |
//  +--------------------+
//  |                    |  Pixel sizes (one byte per face pt size present padded to 32 bits
//  |                    |  from the start) (not used by this driver)
//  +--------------------+
//  |                    |  FaceHeader offset vector 
//  |                    |  (32 bit offset for each face)
//  +--------------------+
//  |                    |  For FontFormat 1 (FontFormat::UTF32 only): the table that contains corresponding
//  |                    |  values between Unicode CodePoints and their internal GlyphCode.
//  |                    |  (content already well aligned to 32 bits frontiers)
//  +--------------------+
//
//  +--------------------+               <------------+
//  |                    |  FaceHeader                |
//  |                    |  (32 bits aligned)         |
//  |                    |                            |
//  +--------------------+                            |
//  |                    |  Glyphs' pixels indexes    |
//  |                    |  in the Pixels Pool        |
//  |                    |  (32bits each)             |
//  +--------------------+                            |
//  |                    |  GlyphsInfo                |
//  |                    |  Array (16 bits aligned)   |
//  |                    |                            |  Repeat for
//  +--------------------+                            |> each face
//  |                    |                            |  part of the
//  |                    |  Pixels Pool               |  font
//  |                    |  (No alignement, all bytes)|
//  |                    |                            |
//  +--------------------+                            |
//  |                    |  Filler (32bits padding)   | <- Takes into account both
//  +--------------------+                            |    GlyphsInfo and PixelsPool
//  |                    |                            |
//  |                    |  LigKerSteps               |
//  |                    |  (2 x 16 bits each step)   |
//  |                    |                            |
//  +--------------------+               <------------+
//             .
//             .
//             .
//
// 2. BACKUP Format:
//
// The BACKUP format is used to keep a copy of glyphs that have been modified by hand.
// This is to allow for the retrieval of changes made when there is a need to
// re-import a font.
//
// Only one backup per glyph is kept in the backup *font*.
//
// It is used by the application as another instance of an IBMF font.
//
// The application is responsible of opening and saving the backup file and to call the
// saveGlyph() method when it is required to save a glyph. The importBackup
// method can be used to retrieve glyphs present in the backup file to incorporate
// them in the current opened ibmf font. Care must be taken by the application to insure
// synchronisation of both opened file to be of the same imported font.
//
// When importing the backup glyphs to the current ibmf font, only the glyphs present in the
// ibmf font will be updated.
//
// Basically:
//
// - The number of faces may be different from the ibmf font as it contains only the
//   faces for which a glyph has been modified
// - In the FaceHeader, the glyphCount can be different for each face present as it
//   reflects the number of modified glyphs present in this backup
// - The GlyphInfo is replaced with BackupGlyphInfo that contains the codePoint
//   associated with the glyph.
// - The information kept for a glyph that was modified are: The pixels bitmap, the
//   glyphInfo metrics and the lig/kern table.
//
// - The createBackup() class method must be called to generate a new backup file if none
//   is available. A new instance of the backup *font* will then be generated without any
//   face in it.
// - The saveGlyph() method is called to save a glyph information to the backup instance.
// - The save() method is called to save the backup *font* to disk.
// - The load() method is called to load a backup  *font* from disk.
//
//  At Offset 0:
//  +--------------------+
//  |                    |  Preamble (6 bytes)
//  |                    |
//  +--------------------+
//  |                    |  Pixel sizes (one byte per face pt size present padded to 32 bits
//  |                    |  from the start) (not used by this driver)
//  +--------------------+
//  |                    |  FaceHeader offset vector
//  |                    |  (32 bit offset for each face)
//  +--------------------+
//
//  +--------------------+               <------------+
//  |                    |  FaceHeader                |
//  |                    |  (32 bits aligned)         |
//  |                    |                            |
//  +--------------------+                            |
//  |                    |  Glyphs' pixels indexes    |
//  |                    |  in the Pixels Pool        |
//  |                    |  (32bits each)             |
//  +--------------------+                            |
//  |                    |  GlyphsBackupInfo          |
//  |                    |  Array (16 bits aligned)   |
//  |                    |                            |  Repeat for
//  +--------------------+                            |> each face
//  |                    |                            |  part of the
//  |                    |  Pixels Pool               |  font
//  |                    |  (No alignement, all bytes)|
//  |                    |                            |
//  +--------------------+                            |
//  |                    |  Filler (32bits padding)   | <- Takes into account both
//  +--------------------+                            |    GlyphsInfo and PixelsPool
//  |                    |                            |
//  |                    |  LigKerSteps               |
//  |                    |  (2 x 16 bits each step)   |
//  |                    |                            |
//  +--------------------+               <------------+
//             .
//             .
//             .
//
//
// clang-format on

namespace ibmf_defs {

#ifdef DEBUG_IBMF
const constexpr int DEBUG = DEBUG_IBMF;
#else
const constexpr int DEBUG = 0;
#endif

const constexpr uint8_t IBMF_VERSION = 4;

// The followings have to be adjusted depending on the screen
// software/hardware/firmware' pixels polarity/color/shading/gray-scale
// At least, one of BLACK... or WHITE... must be 0. If not, some changes are
// required in the code.

// const constexpr uint8_t BLACK_ONE_BIT = 0;
// const constexpr uint8_t WHITE_ONE_BIT = 1;

// const constexpr uint8_t BLACK_EIGHT_BITS = 0;
// const constexpr uint8_t WHITE_EIGHT_BITS = 0xFF;

const constexpr uint8_t BLACK_ONE_BIT = 1;
const constexpr uint8_t WHITE_ONE_BIT = 0;

const constexpr uint8_t BLACK_EIGHT_BITS = 0xFF;
const constexpr uint8_t WHITE_EIGHT_BITS = 0x00;

enum FontFormat : uint8_t { LATIN = 0, UTF32 = 1, BACKUP = 7 };
enum class PixelResolution : uint8_t { ONE_BIT, EIGHT_BITS };

const constexpr PixelResolution resolution = PixelResolution::EIGHT_BITS;

struct Dim {
    uint8_t width;
    uint8_t height;
    Dim(uint8_t w, uint8_t h) : width(w), height(h) {}
    Dim() : width(0), height(0) {}
    bool operator==(const Dim &other) const {
        return (width == other.width) && (height == other.height);
    }
};

struct Pos {
    int8_t x;
    int8_t y;
    Pos(int8_t xpos, int8_t ypos) : x(xpos), y(ypos) {}
    Pos() : x(0), y(0) {}
    bool operator==(const Pos &other) const { return (x == other.x) && (y == other.y); }
};

typedef uint8_t *MemoryPtr;
typedef std::vector<uint8_t> Pixels;
typedef Pixels *PixelsPtr;
typedef uint16_t GlyphCode;
typedef std::vector<char32_t> CharCodes;

const constexpr GlyphCode NO_GLYPH_CODE = 0x7FFF;
const constexpr GlyphCode SPACE_CODE = 0x7FFE;

// RLE (Run Length Encoded) Bitmap. To get something to show, they have
// to be processed through the RLEExtractor class.
// Dim contains the expected width and height once the bitmap has been
// decompressed. The length is the pixels array size in bytes.

struct RLEBitmap {
    Pixels pixels;
    Dim dim;
    uint16_t length;
    void clear() {
        pixels.clear();
        dim = Dim(0, 0);
        length = 0;
    }
};
typedef std::shared_ptr<RLEBitmap> RLEBitmapPtr;

// Uncompressed Bitmap.

struct Bitmap {
    Pixels pixels;
    Dim dim;
    Bitmap() { clear(); }
    void clear() {
        pixels.clear();
        dim = Dim(0, 0);
    }
    Bitmap(Pixels &thePixels, Dim &theDim) {
        pixels = thePixels;
        dim = theDim;
    }
    bool operator==(const Bitmap &other) const {
        if ((pixels.size() != other.pixels.size()) || !(dim == other.dim)) return false;
        for (int idx = 0; idx < pixels.size(); idx++) {
            if (pixels.at(idx) != other.pixels.at(idx)) return false;
        }
        return true;
    }
};
typedef std::shared_ptr<Bitmap> BitmapPtr;

#pragma pack(push, 1)

typedef int16_t FIX16;
typedef int16_t FIX14;

struct Preamble {
    char marker[4];
    uint8_t faceCount;
    struct {
        uint8_t version : 5;
        FontFormat fontFormat : 3;
    } bits;
};
typedef std::shared_ptr<Preamble> PreamblePtr;

struct FaceHeader {
    uint8_t pointSize;         // In points (pt) a point is 1 / 72.27 of an inch
    uint8_t lineHeight;        // In pixels
    uint16_t dpi;              // Pixels per inch
    FIX16 xHeight;             // Hight of character 'x' in pixels
    FIX16 emSize;              // Hight of character 'M' in pixels
    FIX16 slantCorrection;     // When an italic face
    uint8_t descenderHeight;   // The height of the descending below the origin
    uint8_t spaceSize;         // Size of a space character in pixels
    uint16_t glyphCount;       // Must be the same for all face (Except for BACKUP format)
    uint16_t ligKernStepCount; // Length of the Ligature/Kerning table
    uint32_t pixelsPoolSize;   // Size of the Pixels Pool
};

// typedef FaceHeader *FaceHeaderPtr;
typedef std::shared_ptr<FaceHeader> FaceHeaderPtr;
typedef uint8_t (*PixelsPoolTempPtr)[]; // Temporary pointer
typedef uint32_t PixelPoolIndex;
typedef PixelPoolIndex (*GlyphsPixelPoolIndexesTempPtr)[]; // One for each glyph

// clang-format off
//
// The lig kern array contains instructions (struct LibKernStep) in a simple programming
// language that explains what to do for special letter pairs. The information in squared
// brackets relates to fields that are part of the LibKernStep struct. Each entry in this
// array is a lig kern command of four bytes:
//
// - first byte: skip byte, indicates that this is the final program step if the byte
//                          is 128 or more, otherwise the next step is obtained by
//                          skipping this number of intervening steps [nextStepRelative].
// - second byte: next char, if next character follows the current character, then
//                           perform the operation and stop, otherwise continue.
// - third byte: op byte, indicates a ligature step if less than 128, a kern step otherwise.
// - fourth byte: remainder.
//
// In a kern step [isAKern == true], an additional space equal to kern located at
// [(displHigh << 8) + displLow] in the kern array is inserted between the current
// character and [nextChar]. This amount is often negative, so that the characters
// are brought closer together by kerning; but it might be positive.
//
// There are eight kinds of ligature steps [isAKern == false], having op byte codes
// [aOp bOp cOp] where 0 ≤ aOp ≤ bOp + cOp and 0 ≤ bOp, cOp ≤ 1.
//
// The character whose code is [replacementChar] is inserted between the current
// character and [nextChar]; then the current character is deleted if bOp = 0, and
// [nextChar] is deleted if cOp = 0; then we pass over aOp characters to reach the next
// current character (which may have a ligature/kerning program of its own).
//
// If the very first instruction of a character’s lig kern program has [whole > 128],
// the program actually begins in location [(displHigh << 8) + displLow]. This feature
// allows access to large lig kern arrays, because the first instruction must otherwise
// appear in a location ≤ 255.
//
// Any instruction with [whole > 128] in the lig kern array must have
// [(displHigh << 8) + displLow] < the size of the array. If such an instruction is
// encountered during normal program execution, it denotes an unconditional halt; no
// ligature or kerning command is performed.
//
// (The following usage has been extracted from the lig/kern array as not being used outside
//  of a TeX generated document)
//
// If the very first instruction of the lig kern array has [whole == 0xFF], the
// [nextChar] byte is the so-called right boundary character of this font; the value
// of [nextChar] need not lie between char codes boundaries.
//
// If the very last instruction of the lig kern array has [whole == 0xFF], there is
// a special ligature/kerning program for a left boundary character, beginning at location
// [(displHigh << 8) + displLow] . The interpretation is that TEX puts implicit boundary
// characters before and after each consecutive string of characters from the same font.
// These implicit characters do not appear in the output, but they can affect ligatures
// and kerning.
//
// -----
//
// Here is the optimized version considering larger characters table
// and that some fields are not being used (BEWARE: a little-endian format):
//
//           Byte 2                   Byte 1
// +------------------------+------------------------+
// |Stop|               Next Char                    |
// +------------------------+------------------------+
//
//
//           Byte 4                   Byte 3
// +------------------------+------------------------+
// |Kern|             Replacement Char               |  <- isAKern (Kern in the diagram) is false
// +------------------------+------------------------+
// |Kern|GoTo|      Displacement in FIX14            |  <- isAKern is true and GoTo is false => Kerning value
// +------------------------+------------------------+
// |Kern|GoTo|          Displacement                 |  <- isAkern and GoTo are true
// +------------------------+------------------------+
//
// Up to 32765 different glyph codes can be managed through this format.
// Kerning displacements reduced to 14 bits is not a big issue: kernings are
// usually small numbers. FIX14 and FIX16 are using 6 bits for the fraction. Their
// remains 8 bits for FIX14 and 10 bits for FIX16, that is more than enough...
//
// This is NOW the format in use with this driver and other support apps.
//
// clang-format on

union Nxt {
    struct {
        GlyphCode nextGlyphCode : 15;
        bool stop : 1;
    } data;
    struct {
        uint16_t val;
    } whole;
};

union ReplDisp {
    struct {
        GlyphCode replGlyphCode : 15;
        bool isAKern : 1;
    } repl;
    struct {
        FIX14 kerningValue : 14;
        bool isAGoTo : 1;
        bool isAKern : 1;
    } kern;
    struct {
        uint16_t displacement : 14;
        bool isAGoTo : 1;
        bool isAKern : 1;
    } goTo;
    struct {
        uint16_t val;
    } whole;
};

struct LigKernStep {
    Nxt a;
    ReplDisp b;
};

struct RLEMetrics {
    uint8_t dynF : 4;
    uint8_t firstIsBlack : 1;
    uint8_t beforeAddedOptKern : 2;
    uint8_t afterAddedOptKern : 1;
    //
    bool operator==(const RLEMetrics &other) const {
        return (dynF == other.dynF) && (firstIsBlack == other.firstIsBlack) &&
               (beforeAddedOptKern == other.beforeAddedOptKern) &&
               (afterAddedOptKern == other.afterAddedOptKern);
    }
};

struct GlyphInfo {
    uint8_t bitmapWidth;     // Width of bitmap once decompressed
    uint8_t bitmapHeight;    // Height of bitmap once decompressed
    int8_t horizontalOffset; // Horizontal offset from the orign
    int8_t verticalOffset;   // Vertical offset from the origin
    uint16_t packetLength;   // Length of the compressed bitmap
    FIX16 advance;           // Normal advance to the next glyph position in line
    RLEMetrics rleMetrics;   // RLE Compression information
    uint8_t ligKernPgmIndex; // = 255 if none, Index in the ligature/kern array
    GlyphCode mainCode;      // Main composite (or not) glyphCode for kerning matching algo
    //
    bool operator==(const GlyphInfo &other) const {
        return (bitmapWidth == other.bitmapWidth) && (bitmapHeight == other.bitmapHeight) &&
               (horizontalOffset == other.horizontalOffset) &&
               (verticalOffset == other.verticalOffset) && (packetLength == other.packetLength) &&
               (advance == other.advance) && (rleMetrics == other.rleMetrics) &&
               (mainCode == other.mainCode);
    }
};

typedef std::shared_ptr<GlyphInfo> GlyphInfoPtr;

struct BackupGlyphInfo {
    uint8_t bitmapWidth;     // Width of bitmap once decompressed
    uint8_t bitmapHeight;    // Height of bitmap once decompressed
    int8_t horizontalOffset; // Horizontal offset from the orign
    int8_t verticalOffset;   // Vertical offset from the origin
    uint16_t packetLength;   // Length of the compressed bitmap
    FIX16 advance;           // Normal advance to the next glyph position in line
    RLEMetrics rleMetrics;   // RLE Compression information
    int16_t ligCount;
    int16_t kernCount;
    char32_t mainCodePoint; // Main composite (or not) codePoint for kerning matching algo
    char32_t codePoint;     // CodePoint associated with the glyph (for BACKUP Format only)
};

typedef std::shared_ptr<BackupGlyphInfo> BackupGlyphInfoPtr;

// clang-format off
// 
// For FontFormat 1 (FontFormat::UTF32), there is a table that contains
// corresponding values between Unicode CodePoints and their internal GlyphCode.
// The GlyphCode is the index in the glyph table for the character.
//
// This table is in two parts:
//
// - Unicode plane information for the 4 planes supported
//   by the driver,
// - The list of bundle of code points that are part of each plane.
//   A bundle identifies the first codePoint and the number of consecutive codepoints
//   that are part of the bundle.
//
// For more information about the planes, please consult the followint Wikipedia page:
//
//     https://en.wikipedia.org/wiki/Plane_(Unicode)
//
// clang-format on

struct Plane {
    uint16_t codePointBundlesIdx; // Index of the plane in the CodePointBundles table
    uint16_t entriesCount;        // The number of entries in the CodePointBungles table
    GlyphCode firstGlyphCode;     // glyphCode corresponding to the first codePoint in
                                  // the bundles
};

struct CodePointBundle {
    char16_t firstCodePoint; // The first UTF16 codePoint of the bundle
    char16_t lastCodePoint;  // The last UTF16 codePoint of the bundle
};

typedef Plane Planes[4];
typedef CodePointBundle (*CodePointBundlesPtr)[];
typedef Plane (*PlanesPtr)[];

#pragma pack(pop)

struct GlyphMetrics {
    int16_t xoff, yoff;
    int16_t advance;
    int16_t lineHeight;
    int16_t ligatureAndKernPgmIndex;
    void clear() {
        xoff = yoff = 0;
        advance = lineHeight = 0;
        ligatureAndKernPgmIndex = 255;
    }
};

struct Glyph {
    GlyphMetrics metrics;
    Bitmap bitmap;
    uint8_t pointSize;
    void clear() {
        metrics.clear();
        bitmap.clear();
        pointSize = 0;
    }
};

struct GlyphKernStep {
    uint16_t nextGlyphCode;
    FIX16 kern;
    bool operator==(const GlyphKernStep &other) const {
        return (nextGlyphCode == other.nextGlyphCode) && (kern == other.kern);
    }
};
typedef std::vector<GlyphKernStep> GlyphKernSteps;

struct GlyphLigStep {
    uint16_t nextGlyphCode;
    uint16_t replacementGlyphCode;
    //
    bool operator==(const GlyphLigStep &other) const {
        return (nextGlyphCode == other.nextGlyphCode) &&
               (replacementGlyphCode == other.replacementGlyphCode);
    }
};
typedef std::vector<GlyphLigStep> GlyphLigSteps;

struct GlyphLigKern {
    GlyphLigSteps ligSteps;
    GlyphKernSteps kernSteps;
    //
    bool operator==(const GlyphLigKern &other) const {
        if ((ligSteps.size() != other.ligSteps.size()) ||
            (kernSteps.size() != other.kernSteps.size())) {
            return false;
        }

        int idx = 0;
        for (auto &l : ligSteps) {
            if (!(l == other.ligSteps[idx])) {
                return false;
            }
            idx += 1;
        }

        idx = 0;
        for (auto &k : kernSteps) {
            if (!(k == other.kernSteps[idx])) {
                return false;
            }
            idx += 1;
        }
        return true;
    }
};
typedef std::shared_ptr<GlyphLigKern> GlyphLigKernPtr;

#pragma pack(push, 1)
struct BackupGlyphKernStep {
    char32_t nextCodePoint;
    FIX16 kern;
};
typedef std::vector<BackupGlyphKernStep> BackupGlyphKernSteps;

struct BackupGlyphLigStep {
    char32_t nextCodePoint;
    char32_t replacementCodePoint;
};
typedef std::vector<BackupGlyphLigStep> BackupGlyphLigSteps;

struct BackupGlyphLigKern {
    BackupGlyphLigSteps ligSteps;
    BackupGlyphKernSteps kernSteps;
};
typedef std::shared_ptr<BackupGlyphLigKern> BackupGlyphLigKernPtr;
#pragma pack(pop)

// These are the structure required to create a new font
// from some parameters. For now, it is used to create UTF32
// font format files.

struct CharSelection {
    std::string filename; // Filename to import from
    SelectedBlockIndexesPtr selectedBlockIndexes;
};
typedef std::vector<CharSelection> CharSelections;
typedef std::shared_ptr<CharSelections> CharSelectionsPtr;

struct FontParameters {
    int dpi;
    bool pt8;
    bool pt9;
    bool pt10;
    bool pt12;
    bool pt14;
    bool pt17;
    bool pt24;
    bool pt48;
    std::string filename;
    CharSelectionsPtr charSelections;
    bool withKerning;
};
typedef std::shared_ptr<FontParameters> FontParametersPtr;

// Ligature table. Used to create entries in a new font defintition.
// Of course, the three letters must be present in the resulting font to have
// that ligature added to the font.

const struct Ligature {
    char32_t firstChar;
    char32_t nextChar;
    char32_t replacement;
} ligatures[] = {
    {0x0066, 0x0066, 0xFB00}, // f, f, ﬀ
    {0x0066, 0x0069, 0xFB01}, // f, i, ﬁ
    {0x0066, 0x006C, 0xFB02}, // f, l, ﬂ
    {0xFB00, 0x0069, 0xFB03}, // ﬀ ,i, ﬃ
    {0xFB00, 0x006C, 0xFB04}, // ﬀ ,l, ﬄ
    {0x0069, 0x006A, 0x0133}, // i, j, ĳ
    {0x0049, 0x004A, 0x0132}, // I, J, Ĳ
    {0x003C, 0x003C, 0x00AB}, // <, <, «
    {0x003E, 0x003E, 0x00BB}, // >, >, »
    {0x003F, 0x2018, 0x00BF}, // ?, ‘, ¿
    {0x0021, 0x2018, 0x00A1}, // !, ‘, ¡
    {0x2018, 0x2018, 0x201C}, // ‘, ‘, “
    {0x2019, 0x2019, 0x201D}, // ’, ’, ”
    {0x002C, 0x002C, 0x201E}, // , , „
    {0x2013, 0x002D, 0x2014}, // –, -, —
    {0x002D, 0x002D, 0x2013}, // -, -, –
};

const constexpr char32_t ZERO_WIDTH_CODEPOINT = 0xFEFF; // U+0FEFF
const constexpr char32_t UNKNOWN_CODEPOINT = 0xE05E;    // U+E05E This is part of the Sol Font.

} // namespace ibmf_defs
