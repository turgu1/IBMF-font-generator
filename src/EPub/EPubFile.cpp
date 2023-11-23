#include "EPubFile.hpp"

#include "EPubDefs.hpp"
#include "EPubMetaFile.hpp"

auto EPubFile::findFileOffsetAtCharOffset(const std::string &path, uint32_t charOffset)
    -> uint32_t {

    pugi::xml_document &doc = getXHTMLFile(path);

    struct Walker : pugi::xml_tree_walker {
        auto for_each(pugi::xml_node &node) -> bool override {
            if (node.type() == pugi::xml_node_type::node_pcdata) {
                uint32_t length = strlen(node.value());
                if ((chOffset + length) > charOffset) {
                    offset = epub->getOffset(node.value()) + (charOffset - chOffset);
                    return false;
                }
                chOffset += length;
            }
            return true;
        }

        uint32_t offset;
        uint32_t chOffset;
        uint32_t charOffset;
        EPubFile *epub;
    } walker;

    walker.offset = walker.chOffset = 0;
    walker.charOffset = charOffset;
    walker.epub = this;

    doc.child("html").child("body").traverse(walker);
    // if (readable_->isArticle()) log_w("article");

    return walker.offset;
}

auto EPubFile::findCharOffsetAtFileOffset(const std::string &path, uint32_t fileOffset)
    -> uint32_t {

    pugi::xml_document &doc = getXHTMLFile(path);

    struct Walker : pugi::xml_tree_walker {
        auto for_each(pugi::xml_node &node) -> bool override {
            if (node.type() == pugi::xml_node_type::node_pcdata) {
                uint32_t length = strlen(node.value());
                uint32_t nodeOffset = node.offset_debug();
                if ((nodeOffset + length) >= fileOffset) {
                    uint32_t offset = (fileOffset > nodeOffset) ? fileOffset - nodeOffset : 0;
                    charOffset += offset;
                    return false;
                }
                charOffset += length;
            }
            return true;
        }

        uint32_t charOffset;
        uint32_t fileOffset;
    } walker;

    walker.charOffset = 0;
    walker.fileOffset = fileOffset;

    doc.child("html").child("body").traverse(walker);
    // if (readable_->isArticle()) log_w("article");

    return walker.charOffset;
}

// Returns a pointer to the buffer containing the extracted file from the EPub and it's length in a
// tuple
auto EPubFile::getFile(const std::string &completeFilePath)
    -> std::pair<std::shared_ptr<uint8_t[]>, uint32_t> {

    std::shared_ptr<uint8_t[]> buffer = nullptr;

    EPUB_PROFILE_START(UnzipperOpenFile);
    auto res = epubUnzipper_.openFile(completeFilePath);
    EPUB_PROFILE_END(UnzipperOpenFile);

    if (!res) {
        log_e("Unzipper Failed to open file %s", completeFilePath.c_str());
    } else {
        EPUB_PROFILE_START(UnzipperGetFileSize);
        auto size = epubUnzipper_.getFileSize();
        EPUB_PROFILE_END(UnzipperGetFileSize);

        if (size > 0) {
            buffer = std::shared_ptr<uint8_t[]>(new uint8_t[size]);

            if (buffer != nullptr) {
                EPUB_PROFILE_START(UnzipperReadFromFile);
                uint32_t length = epubUnzipper_.readFile(buffer.get(), size);
                epubUnzipper_.closeFile();
                EPUB_PROFILE_END(UnzipperReadFromFile);
                if (length == size) {
                    // if (readable_->isArticle()) log_w("article");

                    return {std::move(buffer), length};
                }
            } else {
                epubUnzipper_.closeFile();
                log_e("Unable to allocate space for file %s, space required: %" PRIu32,
                      completeFilePath.c_str(), static_cast<uint32_t>(size));
            }
        }
    }

    // if (readable_->isArticle()) log_w("article");

    return {nullptr, 0};
}

auto EPubFile::getXHTMLFile(const std::string &path) -> pugi::xml_document & {
    std::string filePath = opf_->getFullPath(path);
    // log_w("Loading file %s", filePath.c_str());
    uint32_t length;

    if (currentFilePath_ != filePath) {

        // log_w("currentFilePath_: %s, filePath : %s", currentFilePath_.c_str(), filePath.c_str());

        std::tie(currentFileBuffer_, length) = getFile(filePath);

        if (currentFileBuffer_ != nullptr) {
            // Using load_buffer_inplace() is mandatory here as it permits the
            // computation of a node offset location, as string elements are taken
            // directly in the currentFileBuffer instead of being copied.
            // (see the EPubFile::getOffset() method).

            pugi::xml_parse_result result = currentFileDoc_.load_buffer_inplace(
                currentFileBuffer_.get(), length,
                (pugi::parse_default | pugi::parse_ws_pcdata) & ~pugi::parse_escapes);

            if (result) {
                currentFilePath_ = filePath;
            } else {
                log_e("Failed to parse XHTML file %s: %s", filePath.c_str(), result.description());
            }
        } else {
            log_e("Unable to unzip file %s", filePath.c_str());
        }
    }

    // if (readable_->isArticle()) log_w("article");

    return currentFileDoc_;
}

auto EPubFile::open() -> bool {

    if (!epubUnzipper_.open()) {
        log_e("Failed to open ePub file");
        return false;
    }

    EPubMetaFile manifest("META-INF/container.xml");
    auto [manifestBuffer, manifestLength] = getFile("META-INF/container.xml");
    if (manifestBuffer == nullptr) {
        log_e("Failed to open ePub's manifest %s", "META-INF/container.xml");
        return false;
    } else {
        manifest.parse(manifestBuffer, manifestLength);
    }

    // for (auto &path : manifest.rootfilePaths) {
    //     log_i("Rootfile path: %s", path.c_str());
    // }

    if (manifest.rootfilePaths.size() == 0) {
        log_e("No rootfile paths found");
        return false;
    } else if (manifest.rootfilePaths.size() > 1) {
        log_w("Multiple rootfile paths found");
    }

    auto [opfBuffer, length] = getFile(manifest.rootfilePaths[0].c_str());
    if (opfBuffer == nullptr) {
        log_e("Failed to open ePub's opf %s", manifest.rootfilePaths[0].c_str());
        return false;
    } else {
        opf_ = std::make_shared<EPubOpf>(manifest.rootfilePaths[0].c_str());
        opf_->parse(opfBuffer, length);
    }

    // log_d("Total size of all ePub text is %d", opf_->getTotalSize());
    fileOpen_ = true;
    return true;
}

void EPubFile::close() {
    epubUnzipper_.close();
    fileOpen_ = false;
}
