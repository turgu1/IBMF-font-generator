#include "EPubOpf.hpp"

#include <cstring>

#include "EPubDefs.hpp"
#include "Misc/pugixml.hpp"

void EPubOpf::parse(const std::shared_ptr<uint8_t[]> &buffer, uint32_t length) {

    basePath_ = "";
    extractPath(path_, basePath_);
    // log_i("Base Path: %s", basePath_.c_str());

    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_buffer_inplace(buffer.get(), length);

    if (result) {

        pugi::xml_node package = doc.child("package");
        pugi::xml_node manifest = package.child("manifest");
        pugi::xml_node metadata = package.child("metadata");
        pugi::xml_node spine = package.child("spine");

        title_ = metadata.child("dc:title").value();
        creator_ = metadata.child("dc:creator").value();

        for (auto item : manifest.children("item")) {
            manifest_[item.attribute("id").value()] =
                ManifestItem{.href = item.attribute("href").value(),
                             .mediaType = item.attribute("media-type").value()};
        }

        for (auto itemref : spine.children("itemref")) {
            std::string idref = itemref.attribute("idref").value();

            auto it = manifest_.find(idref);
            if (it != manifest_.end()) {
                spine_.push_back(SpineItem{.item = &it->second});
            } else {
                log_e("Spine idref not found in manifest: %s", idref.c_str());
            }
        }

    } else {
        log_e("Failed to parse EPub opf %s: %s", path_, result.description());
    }
}

void EPubOpf::extractPath(const char *fname, std::string &path) {
    path.clear();
    int i = strlen(fname) - 1;
    while ((i > 0) && (fname[i] != '/')) {
        i--;
    }
    if (i > 0) {
        path.assign(fname, ++i);
    }
}

auto EPubOpf::getSpineIdx(const std::string &href) const -> int {
    for (int spineIdx = 0; spineIdx < spine_.size(); spineIdx++) {
        if (spine_[spineIdx].item->href == href) {
            return spineIdx;
        }
    }
    return 0;
}

auto EPubOpf::exists(const std::string &id) const -> bool {
    log_d("Checking if %s exists in manifest: %d", id.c_str(),
          manifest_.find(id) != manifest_.end());
    return manifest_.find(id) != manifest_.end();
}

auto EPubOpf::getHrefByID(const std::string &id) const -> std::string {
    auto it = manifest_.find(id);
    if (it != manifest_.end()) {
        return it->second.href;
    }
    return "";
}

auto EPubOpf::getIdByHref(const std::string &href) const -> std::string {
    for (auto &item : manifest_) {
        if (item.second.href == href) {
            return item.first;
        }
    }
    return "";
}
