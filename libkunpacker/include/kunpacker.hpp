#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
//#include <sys/mman.h>

namespace fs = std::filesystem;

namespace ku {
extern fs::path kix_path;
extern fs::path kbf_path;

bool isKixFile(const fs::path& path);
bool isKixData(const uint8_t* data);

bool isHeraFile(const fs::path& path);
bool isHeraData(const uint8_t* data);

void parseKixBlock(const fs::path& basedir, std::ifstream& kix);
void parseKixBlock(const fs::path& basedir, std::ifstream& kix, std::ifstream& kbf);

void printKixBlock(std::ifstream& kix);
void printKixHeader(std::ifstream& kix);
void printKixNode(int index, std::ifstream& kix);

struct __attribute__((packed)) kix_hdr_t {
	char name[32];
	std::uint32_t numRecords;
};

struct __attribute__((packed)) kix_node_t {
	std::uint8_t type;
	std::uint32_t memAddr;
	std::uint32_t offset;
	std::uint32_t size;
	std::uint8_t nameLen; // not present for KIX_BLOCK
	char name[];
};

void getKixHdr(std::ifstream& kix, kix_hdr_t* hdr);
void getKixNode(std::ifstream& kix, kix_node_t* node, std::vector<char>* name);
void printKixHeader(const kix_hdr_t& hdr);
void printKixNode(const kix_node_t& node, const std::vector<char>& name);

void extractKixNode(const fs::path& basedir, std::ifstream& kix, std::ifstream& kbf);

struct __attribute__((packed)) kbf_node_t {
	char name[32];
	uint8_t data[];
};

void getKbfNode(std::ifstream& kbf, kbf_node_t* node);

/*
     struct map_t {
     std::uintptr_t start;
     std::uintptr_t end;
     std::size_t size;
     };

     long int parse_kix_block(char* basedir, kix_hdr_t* hdr, const map_t& kix_map);

     void map_new(map_t* map, void* start, std::size_t size);
     */
} // namespace ku
