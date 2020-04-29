#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <locale>
#include <string>

#include "kunpacker.hpp"

namespace ku {
fs::path kix_path = "/dev/null";
fs::path kbf_path = "/dev/null";

constexpr auto KIX_BLOCK = 0;
constexpr auto KIX_ENTRY = 1;

bool
isKixFile(const fs::path& path) {
	std::ifstream file;
	file.open(path, std::ios::binary);
	if (!file.is_open()) std::exit(EXIT_FAILURE);

	constexpr std::size_t headSz = sizeof(kix_hdr_t) + sizeof(kix_node_t);
	char head[headSz];
	std::memset(&head, 0x00, headSz);

	file.read(reinterpret_cast<char*>(&head), headSz);
	file.close();

	return isKixData((const uint8_t*)&head);
}

bool
isKixData(const uint8_t* data) {
	kix_node_t* root = (kix_node_t*)(data + sizeof(kix_hdr_t));
	if (root->type != KIX_BLOCK && root->type != KIX_ENTRY) return false;
	if (root->nameLen > 32) return false;
	return true;
}

bool
isHeraFile(const fs::path& path) {
	std::ifstream file;
	file.open(path, std::ios::binary);
	if (!file.is_open()) std::exit(EXIT_FAILURE);

	char head[32];
	std::memset(&head, 0x00, 32);

	file.read(reinterpret_cast<char*>(&head), 32);
	file.close();

	return isHeraData((const uint8_t*)&head);
}

bool
isHeraData(const uint8_t* data) {
	int i, steps = 0;
	char ch;
	for (i = 0;; i++) {
		if (i + 10 >= 32) return false;
		if (!std::strncmp((char*)(data + i), " \r\n%%%% \r\n", 10)) return true;
	}
	return false;
}

void
parseKixBlock(const fs::path& basedir, std::ifstream& kix) {
	printKixBlock(kix);

	kix_hdr_t hdr;
	getKixHdr(kix, &hdr);
	// Move input position indicator
	kix.seekg(sizeof(kix_hdr_t), std::ios::cur);
	for (int i = 0; i < hdr.numRecords; i++) {
		kix_node_t node;
		getKixNode(kix, &node);

		// nested KIX headers indicate directories
		if (node.type == KIX_BLOCK) {
			// Save the input position indicator
			auto start = kix.tellg();

			std::cout << "[DBG] parsing directory\n";
			kix.seekg(node.offset, std::ios::beg);
			// std::cout << "[DBG] dirHdr @0x" << std::hex << kix.tellg() << std::dec << "\n";
			struct kix_hdr_t dirHdr;
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
			kix.seekg(sizeof(kix_node_t) - sizeof(std::uint8_t),
			          std::ios::cur); // block doesn't have string size field
			// std::cout << "[DBG] KIX_BLOCK end @0x" << std::hex << kix.tellg() << std::dec
			// << "\n";
		} else {
			// Extract file entry
			std::cout << "#" << std::to_string(i) << " "
			          << "0x" << std::hex << kix.tellg() << std::dec << " ";
			printKixNode(node);
			// Move input position indicator
			kix.seekg(sizeof(kix_node_t) + node.nameLen, std::ios::cur);
			// std::cout << "[DBG] start @0x" << std::hex << kix.tellg() << std::dec << "\n";
		}
	}
}

void
printKixBlock(std::ifstream& kix) {
	// Save the input position indicator
	auto pos = kix.tellg();
	std::cout << "--- KIX BLOCK 0x" << std::hex << kix.tellg() << " ---\n";
	std::cout << "-- Header 0x" << std::hex << kix.tellg() << " --\n";
	printKixHeader(kix);
	// Move input position indicator
	kix.seekg(sizeof(kix_hdr_t), std::ios::cur);
	std::cout << "-- Node 0x" << std::hex << kix.tellg() << " --\n";
	printKixNode(0, kix);
	// Reset the input position indicator
	kix.seekg(pos);
	std::cout << "-----------------------\n";
}

void
printKixHeader(std::ifstream& kix) {
	kix_hdr_t hdr;
	getKixHdr(kix, &hdr);
	printKixHeader(hdr);
}

void
printKixNode(int index, std::ifstream& kix) {
	kix_node_t node;
	getKixNode(kix, &node);
	std::cout << "#" << index << " ";
	printKixNode(node);
}

void
getKixHdr(std::ifstream& kix, kix_hdr_t* hdr) {
	// Save the input position indicator
	auto pos = kix.tellg();
	kix.read(reinterpret_cast<char*>(hdr), sizeof(kix_hdr_t));
	// Reset the input position indicator
	kix.seekg(pos);
}
void
getKixNode(std::ifstream& kix, kix_node_t* node) {
	// Save the input position indicator
	auto pos = kix.tellg();
	kix.read(reinterpret_cast<char*>(node), sizeof(kix_node_t));
	// Reset the input position indicator
	kix.seekg(pos);
}

void
printKixHeader(const kix_hdr_t& hdr) {
	std::cout << std::dec << "Name: '" << hdr.name << "' "
	          << "nRecs: " << std::dec << hdr.numRecords << "\n";
}

void
printKixNode(const kix_node_t& node) {
	std::cout << std::hex << "memAddr: 0x" << node.memAddr << " "
	          << "offset: 0x" << node.offset << " "
	          << "(" << std::dec << node.size << "bytes) "
	          << "Type: " << std::to_string(node.type) << " ";

	std::string name(node.name, node.nameLen);
	std::cout << "name: '" << name.c_str() << "'\n";
}

void
parseKixBlock(const fs::path& basedir, std::ifstream& kix, std::ifstream& kbf) {
	// printKixBlock(kix);
	kix_hdr_t hdr;
	getKixHdr(kix, &hdr);
	// Move input position indicator
	kix.seekg(sizeof(kix_hdr_t), std::ios::cur);
	for (int i = 0; i < hdr.numRecords; i++) {
		kix_node_t node;
		getKixNode(kix, &node);

		// nested KIX headers indicate directories
		if (node.type == KIX_BLOCK) {
			// Save the input position indicator
			auto start = kix.tellg();

			// std::cout << "[DBG] parsing directory\n";
			kix.seekg(node.offset, std::ios::beg);
			// std::cout << "[DBG] dirHdr @0x" << std::hex << kix.tellg() << std::dec << "\n";
			struct kix_hdr_t dirHdr;
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
			kix.seekg(sizeof(kix_node_t) - sizeof(std::uint8_t),
			          std::ios::cur); // block doesn't have string size field
			// std::cout << "[DBG] KIX_BLOCK end @0x" << std::hex << kix.tellg() << std::dec
			// << "\n";
		} else {
			// std::cout
			//	<< "#" << std::to_string(i) << " "
			//	<< "0x" << std::hex << kix.tellg() << std::dec << " ";
			// printKixNode(node);
			// Extract file entry
			extractKixNode(basedir, kix, kbf);
			// Move input position indicator
			kix.seekg(sizeof(kix_node_t) + node.nameLen, std::ios::cur);
			// std::cout << "[DBG] start @0x" << std::hex << kix.tellg() << std::dec << "\n";
		}
	}
}

void
getKbfNode(std::ifstream& kbf, kbf_node_t* node) {
	// Save the input position indicator
	auto pos = kbf.tellg();
	kbf.read(reinterpret_cast<char*>(node), sizeof(kbf_node_t));
	// Reset the input position indicator
	kbf.seekg(pos);
}

void
extractKixNode(const fs::path& basedir, std::ifstream& kix, std::ifstream& kbf) {
	if (fs::file_size(kbf_path) == 0) exit(-1);
	kix_node_t node;
	getKixNode(kix, &node);
	if (node.offset + node.size > fs::file_size(kbf_path)) {
		std::cerr << std::hex << "ERROR: cannot extract! out of boundary (0x"
		          << node.offset + node.size << " >= 0x" << fs::file_size(kbf_path)
		          << ")!\n",
		  exit(-2);
	}

	// Get the file entry referenced by this index entry
	kbf.seekg(node.offset);
	kbf_node_t data;
	getKbfNode(kbf, &data);

	std::string name(data.name);
	// std::cout << "[DBG] filename: '" << name << "'\n";

	fs::path filename = basedir / fs::path(name);
	std::cout << "Extracting: " << filename << "...\n";

	std::ofstream ofs_out;
	ofs_out.open(filename, std::ios::binary);
	if (!ofs_out.is_open()) {
		std::cerr << "ERROR: cannot open filename '" << filename << '\n';
		std::exit(EXIT_FAILURE);
	}

	ofs_out.write(reinterpret_cast<char*>(&(data.data)), node.size); // binary output
	ofs_out.close();
	std::cout << "Extracting: DONE\n";
}
} // namespace ku
