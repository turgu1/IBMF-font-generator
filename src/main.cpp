#include <iostream>
#include <map>
#include <memory>

#include "EPub/EPubFile.hpp"
#include "IBMF/IBMFHexImport.hpp"
#include "IBMF/UTF8Iterator.hpp"

using CharsList = std::map<char32_t, uint32_t>;

std::shared_ptr<EPubFile> ePubFile;
CharsList charsList;

UBlocks myUBlocks;
IBMFHexImport ibmfHexImport;

void ParseFile(pugi::xml_document &doc) {

  struct Walker : pugi::xml_tree_walker {
    CharsList *list;

    auto for_each(pugi::xml_node &node) -> bool override {
      if (node.type() == pugi::xml_node_type::node_pcdata) {
        const std::string data = node.value();
        auto iter = UTF8Iterator(data);
        while (iter != data.end()) {
          char32_t ch = *iter++;
          if ((ch > ' ') && (ch != 0xA0) &&
              !((ch >= 0x2000) && (ch <= 0x200F)) && (ch != 0x202F) &&
              (ch != ibmf_defs::ZERO_WIDTH_CODEPOINT) &&
              (ch != ibmf_defs::UNKNOWN_CODEPOINT) &&
              !((ch >= 0xFFF0) && (ch <= 0xFFFF))) {
            auto entry = list->find(ch);
            if (entry != list->end()) {
              entry->second++;
            } else {
              (*list)[ch] = 1;
            }
          }
        }
      }
      return true;
    }

  } walker;

  walker.list = &charsList;
  doc.traverse(walker);
}

void ShowCharsList() {
  int i = 0;
  for (auto &entry : charsList) {
    std::cout << std::hex << entry.first << std::dec << ":" << entry.second
              << " ";
    i++;
    if ((i % 10) == 0) {
      std::cout << std::endl;
    }
  }
  if ((i % 10) != 0) {
    std::cout << std::endl;
  }
}

void BuildUBlocks() {
  uint32_t count = 0;
  uint32_t first = -1;
  uint32_t code = -1;
  uint32_t last = -1;
  for (auto &entry : charsList) {
    if (entry.first != code + 1) {
      if (code == -1) {
        first = code = entry.first;
      } else {
        std::cout << std::hex << first << " .. " << code << std::dec
                  << std::endl;
        myUBlocks.push_back(UBlockDef{first, code, ""});
        count += 1;
        code = last = first = entry.first;
      }
    } else {
      last = code = entry.first;
    }
  }
  if (last != -1) {
    std::cout << std::hex << first << " .. " << last << std::dec << std::endl;
    myUBlocks.push_back(UBlockDef{first, last, ""});
    count += 1;
  }
  std::cout << "Cluster Count : " << count << std::endl;

  std::cout << std::endl << "MyUBlocks:" << std::endl << std::endl;

  for (auto &ub : myUBlocks) {
    std::cout << std::hex << ub.first_ << " .. " << ub.last_ << std::dec
              << std::endl;
  }
  std::cout << "[The End]" << std::endl;
}

auto LoadXHTMLAt(Idx fileIdx) -> pugi::xml_document & {
  const EPubOpf::SpineItem &spineItem = ePubFile->getSpine(fileIdx);
  return ePubFile->getXHTMLFile(spineItem.item->href);
}

auto ScanDocument() -> bool {
  for (Idx fileIdx = 0; fileIdx < ePubFile->getSpineCount(); fileIdx++) {
    pugi::xml_document &doc = LoadXHTMLAt(fileIdx);

    if (doc) {
      ParseFile(doc);
    } else {
      return false;
    }
  }

  return true;
}

void Usage(const char *path) {
  std::cout << "Usage: " << path << " <HEX Font Path> <EPub file path>"
            << std::endl;
}

auto main(int argc, char **argv) -> int {

  int status = 0;

  // if (argc != 3) {
  //     Usage(argv[0]);
  //     return -1;
  // }

  ePubFile =
      std::make_shared<EPubFile>(argc == 3 ? argv[2] : "./V1010490321.epub");

  if (ePubFile->isOpen()) {
    log_i("File %s is open", argc == 3 ? argv[2] : "./V1010490321.epub");
    if (ScanDocument()) {
      log_i("Scan completed! Characters Count: %" PRIu32,
            (uint32_t)charsList.size());
      ShowCharsList();
      BuildUBlocks();
      ibmfHexImport.loadHex(argc == 3 ? argv[2] : "./unifont-15.1.04.hex",
                            myUBlocks);

      std::fstream out;
      out.open("font.ibmf", std::ios::out);

      if (out.is_open()) {
        ibmfHexImport.save(out);
        out.close();
      } else {
        std::cout << "Unable to open font.ibmf" << std::endl;
      }
    } else {
      log_e("Unable to complete document scan");
    }
  } else {
    log_e("Unable to open file %s", argc == 3 ? argv[2] : "./V1010490321.epub");
    status = -2;
  }

  return status;
}