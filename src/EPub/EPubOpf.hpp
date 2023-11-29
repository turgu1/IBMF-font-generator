#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "EPubDefs.hpp"

class EPubOpf {
public:
  EPubOpf(const char *path) : path_(path) {}

  void parse(const std::shared_ptr<uint8_t[]> &buffer, uint32_t length);

  struct ManifestItem {
    std::string href;
    std::string mediaType;
  };

  struct SpineItem {
    ManifestItem *item;
  };

  using ManifestMap = std::unordered_map<std::string, ManifestItem>;

  [[nodiscard]] auto getSpine(Idx idx) const -> const SpineItem & { return spine_[idx]; }
  [[nodiscard]] auto getManifestMap() const -> const ManifestMap & { return manifest_; }

  [[nodiscard]] auto getFullPath(const std::string &fileName) const -> std::string {
    return basePath_ + fileName;
  }

  [[nodiscard]] inline auto getSpineCount() const -> uint16_t { return spine_.size(); }
  [[nodiscard]] inline auto getManifestCount() const -> uint16_t { return manifest_.size(); }

  void extractPath(const char *fname, std::string &path);

  auto getSpineIdx(const std::string &href) const -> int;

  auto exists(const std::string &id) const -> bool;

  auto getHrefByID(const std::string &id) const -> std::string;
  auto getIdByHref(const std::string &href) const -> std::string;

private:
  const char *path_;

  std::string basePath_;

  using ManifestIterator = std::unordered_map<std::string, ManifestItem>::iterator;
  using SpineVector      = std::vector<SpineItem>;

  std::string title_;
  std::string creator_;
  ManifestMap manifest_;
  SpineVector spine_;
};
