#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "kunpacker.hpp"

int
main(int argc, char** argv) {
	std::cout << "Kallisto Index File (KIX) | Kallisto Binary File (KBF) Unpacker\n";
	if (argc == 1) {
		std::cerr << "Usage:\n";
		std::cerr << "* Read KIX file: " << argv[0] << "[file.kix]\n";
		std::cerr << "* Extract KBF file: " << argv[0] << "[file.kix] [file.kbf]\n";
		std::exit(EXIT_SUCCESS);
	}
	bool extract = false;

	// Read second file
	std::ifstream ifs_kbf;
	if (argc >= 3) {
		ku::kbf_path = argv[2];
		ifs_kbf.open(ku::kbf_path, std::ios::in | std::ios::binary);
		if (!ifs_kbf.is_open()) {
			std::cerr << "ERROR: cannot open KBF file '" << ku::kbf_path << "'\n";
			std::exit(EXIT_FAILURE);
		}

		// Display file size
		try {
			auto kbf_size = fs::file_size(ku::kbf_path);
			std::cout << "File size = " << kbf_size << "bytes\n";
		} catch (fs::filesystem_error& e) {
			std::cerr << e.what() << '\n';
			std::exit(EXIT_FAILURE);
		}
		extract = true;
	}

	// Read first file
	std::ifstream ifs_kix;
	if (argc >= 2) {
		ku::kix_path = argv[1];
		if (ku::isKixFile(ku::kix_path)) {
			std::cout << ku::kix_path << " => KIX File Detected!\n";
			ifs_kix.open(ku::kix_path, std::ios::in | std::ios::binary);
			if (!ifs_kix.is_open()) {
				std::cerr << "ERROR: cannot open KIX file '" << ku::kix_path << "'\n";
				std::exit(EXIT_FAILURE);
			}

			// Display file size
			try {
				auto kix_size = fs::file_size(ku::kix_path);
				std::cout << "File size = " << kix_size << "bytes\n";
			} catch (fs::filesystem_error& e) {
				std::cerr << e.what() << '\n';
				std::exit(EXIT_FAILURE);
			}

			// Parse KIX [KBF] file(s)
			try {
				std::filesystem::path base_path = fs::current_path();
				std::cout << "Current dir = " << base_path << "\n";

				if (extract)
					ku::parseKixBlock(base_path, ifs_kix, ifs_kbf);
				else
					ku::parseKixBlock(base_path, ifs_kix);
			} catch (fs::filesystem_error& e) {
				std::cerr << e.what() << '\n';
				std::exit(EXIT_FAILURE);
			}
		} else {
			if (ku::isHeraFile(ku::kix_path)) {
				std::cout << ku::kix_path << " => Hera File Detected !\n";
				std::exit(EXIT_FAILURE);
			} else {
				std::cout << ku::kix_path << " => Unknown/Raw File Detected !\n";
				std::exit(EXIT_FAILURE);
			}
		}
	}

	return EXIT_FAILURE;
}
