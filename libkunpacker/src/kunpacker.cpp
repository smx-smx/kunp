#include "kunpacker.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <locale>
#include <string>

namespace ku {
fs::path kix_path = "/dev/null";
fs::path kbf_path = "/dev/null";

bool isKixFile(const fs::path& path) {
  std::ifstream file;
  file.open(path, std::ios::binary);
  if (!file.is_open()) std::exit(EXIT_FAILURE);

  constexpr std::size_t headSz = sizeof(kixHdr_t) + sizeof(kixNode_t);
  char head[headSz];
  std::memset(&head, 0x00, headSz);

  file.read(reinterpret_cast<char*>(&head), headSz);
  file.close();

  return isKixData((const std::uint8_t*)&head);
}

bool isKixData(const std::uint8_t* data) {
  kixNode_t* root = (kixNode_t*)(data + sizeof(kixHdr_t));
  if (root->type != kixNodeType::DIRECTORY && root->type != kixNodeType::FILE)
    return false;
  if (root->nameLen > 32) return false;
  return true;
}

bool isHeraFile(const fs::path& path) {
  std::ifstream file;
  file.open(path, std::ios::binary);
  if (!file.is_open()) std::exit(EXIT_FAILURE);

  char head[32];
  std::memset(&head, 0x00, 32);

  file.read(reinterpret_cast<char*>(&head), 32);
  file.close();

  return isHeraData((const std::uint8_t*)&head);
}

bool isHeraData(const std::uint8_t* data) {
  for (int i = 0;; i++) {
    if (i + 10 >= 32) return false;
    if (!std::strncmp((char*)(data + i), " \r\n%%%% \r\n", 10)) return true;
  }
  return false;
}

void parseKixBlock(const fs::path& basedir, std::ifstream& kix) {
  printKixBlock(kix);

  kixHdr_t hdr;
  getKixHdr(kix, &hdr);
  // Move input position indicator
  kix.seekg(sizeof(kixHdr_t), std::ios::cur);
  for (std::uint32_t i = 0; i < hdr.numRecords; i++) {
    kixNode_t node;
    std::vector<char> name;
    getKixNode(kix, &node, &name);

    if (node.type == kixNodeType::DIRECTORY) {
      // Save the input position indicator
      auto start = kix.tellg();

      std::cout << "[DBG] parsing directory\n";
      kix.seekg(node.offset, std::ios::beg);
      // std::cout << "[DBG] dirHdr @0x" << std::hex << kix.tellg() << std::dec
      // << "\n";
      struct kixHdr_t dirHdr;
      getKixHdr(kix, &dirHdr);

      std::string name(dirHdr.name);
      std::cout << "[DBG] directory name: '" << name << "'\n";

      fs::path subdir = basedir / fs::path(name);
      std::cout << "[DBG] Create dir: " << subdir << "\n";
      // fs::create_directories(subdir);
      // fs::permissions(subdir, fs::perms::all, fs::perm_options::replace);

      parseKixBlock(subdir, kix);

      // Reset the input position indicator
      kix.seekg(start);
      kix.seekg(sizeof(kixNode_t) - sizeof(std::uint8_t),
                std::ios::cur);  // block doesn't have string size field
      // std::cout << "[DBG] Directory end @0x" << std::hex << kix.tellg() <<
      // std::dec
      // << "\n";
    } else {
      // Extract file entry
      std::cout << "#" << std::to_string(i) << " "
                << "0x" << std::hex << kix.tellg() << std::dec << " ";
      printKixNode(node, name);
      // Move input position indicator
      kix.seekg(sizeof(kixNode_t) + node.nameLen, std::ios::cur);
      // std::cout << "[DBG] start @0x" << std::hex << kix.tellg() << std::dec
      // << "\n";
    }
  }
}

void printKixBlock(std::ifstream& kix) {
  // Save the input position indicator
  auto pos = kix.tellg();
  std::cout << "--- KIX BLOCK 0x" << std::hex << kix.tellg() << " ---\n";
  std::cout << "-- Header 0x" << std::hex << kix.tellg() << " --\n";
  printKixHeader(kix);
  // Move input position indicator
  kix.seekg(sizeof(kixHdr_t), std::ios::cur);
  std::cout << "-- Node 0x" << std::hex << kix.tellg() << " --\n";
  printKixNode(0, kix);
  // Reset the input position indicator
  kix.seekg(pos);
  std::cout << "-----------------------\n";
}

void printKixHeader(std::ifstream& kix) {
  kixHdr_t hdr;
  getKixHdr(kix, &hdr);
  printKixHeader(hdr);
}

void printKixNode(int index, std::ifstream& kix) {
  kixNode_t node;
  std::vector<char> name;
  getKixNode(kix, &node, &name);
  std::cout << "#" << index << " ";
  printKixNode(node, name);
}

std::ostream& operator<<(std::ostream& os, kixNodeType type) {
  switch (type) {
    case kixNodeType::DIRECTORY:
      return os << "Directory";
    case kixNodeType::FILE:
      return os << "File";
  };
  // Should be never reached.
  return os << "Type unknow !";
}

void getKixHdr(std::ifstream& kix, kixHdr_t* hdr) {
  // Save the input position indicator
  auto pos = kix.tellg();
  kix.read(reinterpret_cast<char*>(hdr), sizeof(kixHdr_t));
  // Reset the input position indicator
  kix.seekg(pos);
}
void getKixNode(std::ifstream& kix, kixNode_t* node, std::vector<char>* name) {
  // Save the input position indicator
  auto pos = kix.tellg();
  kix.read(reinterpret_cast<char*>(node), sizeof(kixNode_t));
  name->resize(node->nameLen);
  kix.read(reinterpret_cast<char*>(name->data()), node->nameLen);
  // Reset the input position indicator
  kix.seekg(pos);
}

void printKixHeader(const kixHdr_t& hdr) {
  std::cout << std::dec << "Name: '" << hdr.name << "' "
            << "nRecs: " << std::dec << hdr.numRecords << "\n";
}

void printKixNode(const kixNode_t& node, const std::vector<char>& name) {
  std::cout << std::hex << "memAddr: 0x" << node.memAddr << " "
            << "offset: 0x" << node.offset << " "
            << "(" << std::dec << node.size << "bytes) "
            << "Type: " << node.type << " ";

  std::string filename(name.data(), name.size());
  std::cout << "size: '" << filename.size() << "' ";
  std::cout << "name: '" << filename << "'\n";
}

void parseKixBlock(const fs::path& basedir, std::ifstream& kix,
                   std::ifstream& kbf) {
  // printKixBlock(kix);
  kixHdr_t hdr;
  getKixHdr(kix, &hdr);
  // Move input position indicator
  kix.seekg(sizeof(kixHdr_t), std::ios::cur);
  for (std::uint32_t i = 0; i < hdr.numRecords; i++) {
    kixNode_t node;
    std::vector<char> name;
    getKixNode(kix, &node, &name);

    if (node.type == kixNodeType::DIRECTORY) {
      // Save the input position indicator
      auto start = kix.tellg();

      // std::cout << "[DBG] parsing directory\n";
      kix.seekg(node.offset, std::ios::beg);
      // std::cout << "[DBG] dirHdr @0x" << std::hex << kix.tellg() << std::dec
      // << "\n";
      struct kixHdr_t dirHdr;
      getKixHdr(kix, &dirHdr);

      std::string name(dirHdr.name);
      // std::cout << "[DBG] directory name: '" << name << "'\n";

      fs::path subdir = basedir / fs::path(name);
      std::cout << "Create dir: " << subdir << "\n";
      fs::create_directories(subdir);
      fs::permissions(subdir, fs::perms::all, fs::perm_options::replace);

      parseKixBlock(subdir, kix, kbf);

      // Reset the input position indicator
      kix.seekg(start);
      kix.seekg(sizeof(kixNode_t) - sizeof(std::uint8_t),
                std::ios::cur);  // block doesn't have string size field
      // std::cout << "[DBG] Directory end @0x" << std::hex << kix.tellg() <<
      // std::dec
      // << "\n";
    } else {
      // std::cout
      //	<< "#" << std::to_string(i) << " "
      //	<< "0x" << std::hex << kix.tellg() << std::dec << " ";
      // printKixNode(node, name);
      // Extract file entry
      extractKixNode(basedir, kix, kbf);
      // Move input position indicator
      kix.seekg(sizeof(kixNode_t) + node.nameLen, std::ios::cur);
      // std::cout << "[DBG] start @0x" << std::hex << kix.tellg() << std::dec
      // << "\n";
    }
  }
}

void getKbfNode(std::ifstream& kbf, kbf_node_t* node) {
  // Save the input position indicator
  auto pos = kbf.tellg();
  kbf.read(reinterpret_cast<char*>(node), sizeof(kbf_node_t));
  // Reset the input position indicator
  kbf.seekg(pos);
}

void extractKixNode(const fs::path& basedir, std::ifstream& kix,
                    std::ifstream& kbf) {
  if (fs::file_size(kbf_path) == 0) exit(-1);
  kixNode_t node;
  std::vector<char> name;
  getKixNode(kix, &node, &name);
  if (node.offset + node.size > fs::file_size(kbf_path)) {
    std::cerr << std::hex << "ERROR: cannot extract! out of boundary (0x"
              << node.offset + node.size << " >= 0x" << fs::file_size(kbf_path)
              << ")!\n",
        exit(-2);
  }

  // Get the file entry referenced by this index entry
  kbf.seekg(node.offset);
  kbf_node_t kbf_node;
  getKbfNode(kbf, &kbf_node);

  kbf.ignore(32);
  std::uint8_t data[node.size];
  kbf.read((char*)data, node.size);

  std::string kbf_name(kbf_node.name);
  // std::cout << "[DBG] filename: '" << kbf_name << "'\n";

  fs::path filename = basedir / fs::path(kbf_name);
  std::cout << "Extracting: " << filename << "...\n";

  std::ofstream ofs_out;
  ofs_out.open(filename, std::ios::binary);
  if (!ofs_out.is_open()) {
    std::cerr << "ERROR: cannot open filename '" << filename << '\n';
    std::exit(EXIT_FAILURE);
  }

  ofs_out.write(reinterpret_cast<char*>(data), node.size);  // binary output
  ofs_out.close();
  std::cout << "Extracting: DONE\n";
}
}  // namespace ku
