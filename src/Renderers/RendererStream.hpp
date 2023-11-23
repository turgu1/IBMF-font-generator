#pragma once

#include <cinttypes>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>

#include "Misc/StringUtil.hpp"
#include "Models/DocType.hpp"

class RendererStream {
private:
    DocType docType_{DocType::NONE};

protected:
    RendererStream(DocType docType) : docType_(docType) {}
    RendererStream() = default;

public:
    virtual ~RendererStream() = default;

    [[nodiscard]] auto isFor(DocType docType) const -> bool { return docType == docType_; }
    [[nodiscard]] auto getDocType() const -> DocType { return docType_; }

    static auto parseDocType(const char *path) -> DocType {
        if (HasSuffix(path, ".txt") || HasSuffix(path, ".txt.gz")) {
            return DocType::TXT;
        }
        if (HasSuffix(path, ".epub")) {
            return DocType::EPUB;
        }
        return DocType::NONE;
    }

    virtual auto getNextByte() -> uint8_t {
        if (size() > position()) {
            return static_cast<uint8_t>(read());
        } else {
            return 0;
        }
    }

    virtual auto available() -> int { return size() - position(); };
    virtual auto readBytes(uint8_t *buf, int size) -> int {
        // implementation of byte-by-byte read for a block,
        // only used if RendererStream subclass does not override this
        // currently only compressed streams use and override this method
        int ch = 0;
        for (int i = 0; i < size; i++) {
            ch = read();
            if (ch < 0) {
                return i;
            }

            buf[i] = ch;
        }

        return size;
    };
    virtual auto read() -> int = 0;
    virtual auto seek(uint32_t pos) -> bool = 0;
    virtual auto position() -> size_t = 0;
    virtual auto size() -> size_t = 0;
};
