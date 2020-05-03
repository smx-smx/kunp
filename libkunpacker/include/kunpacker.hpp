#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace ku {
extern fs::path kix_path;
extern fs::path kbf_path;

bool isKixFile(const fs::path& path);
bool isKixData(const std::uint8_t* data);

bool isHeraFile(const fs::path& path);
bool isHeraData(const std::uint8_t* data);

void parseKixBlock(const fs::path& basedir, std::ifstream& kix);
void parseKixBlock(const fs::path& basedir, std::ifstream& kix,
                   std::ifstream& kbf);

void printKixBlock(std::ifstream& kix);
void printKixHeader(std::ifstream& kix);
void printKixNode(int index, std::ifstream& kix);

struct __attribute__((packed)) kixHdr_t {
 public:
  char name[32];
  std::uint32_t numRecords;
};

enum class kixNodeType : uint8_t { DIRECTORY = 0, FILE = 1 };
std::ostream& operator<<(std::ostream& os, kixNodeType type);

struct __attribute__((packed)) kixNode_t {
 public:
  kixNodeType type;
  std::uint32_t memAddr;
  std::uint32_t offset;
  std::uint32_t size;
};

struct __attribute__((packed)) kixFileNode_t : public kixNode_t {
  std::uint8_t nameLen;  // not present for KIX_BLOCK
  char name[];
};

void getKixHdr(std::ifstream& kix, kixHdr_t* hdr);
void getKixNode(std::ifstream& kix, kixFileNode_t* node,
                std::vector<char>* name);
void printKixHeader(const kixHdr_t& hdr);
void printKixNode(const kixNode_t& node, const std::vector<char>& name);

void extractKixNode(const fs::path& basedir, std::ifstream& kix,
                    std::ifstream& kbf);

struct __attribute__((packed)) kbf_node_t {
  char name[32];
  std::uint8_t data[];
};

void getKbfNode(std::ifstream& kbf, kbf_node_t* node);
}  // namespace ku
