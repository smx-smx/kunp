#pragma once

#include <cstdint>
#include <fstream>
#include <filesystem>
#include <string>
//#include <sys/mman.h>

namespace fs = std::filesystem;

namespace ku {
bool isKixFile(const fs::path& path);
bool isKixData(const uint8_t* data);

bool isHeraFile(const fs::path& path);
bool isHeraData(const uint8_t* data);

extern bool extract;

void parseKixBlock(const fs::path& basedir, std::ifstream& kix);


void parseKixBlock(const fs::path& basedir, std::ifstream& kix, std::ifstream& kbf);

/*
struct __attribute__((packed)) kbf_node_t {
	char name[32];
	uint8_t data[];
};

struct map_t {
	std::uintptr_t start;
	std::uintptr_t end;
	std::size_t size;
};

long int parse_kix_block(char* basedir, kix_hdr_t* hdr, const map_t& kix_map);

void map_new(map_t* map, void* start, std::size_t size);
*/
} // namespace ku
