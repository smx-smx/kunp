#include <cstdlib>
#include <fstream>
#include <filesystem>
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

  // Read first file
  std::ifstream ifs_kix;
  if (argc >= 2) {
    std::filesystem::path kix_path = argv[1];
    if (ku::isKixFile(kix_path)) {
      std::cout << kix_path << " => KIX File Detected!\n";

      ifs_kix.open(kix_path, std::ios::in | std::ios::binary);
      if (!ifs_kix.is_open()) {
        std::cerr << "ERROR: cannot open KIX file '" << kix_path << '\n';
        std::exit(EXIT_FAILURE);
      }

      try {
        auto kix_size = fs::file_size(kix_path);
        std::cout << "File size = " << kix_size << "bytes\n";
      } catch(fs::filesystem_error& e) {
        std::cerr << e.what() << '\n';
        std::exit(EXIT_FAILURE);
      }
    } else {
      if (ku::isHeraFile(kix_path)) {
        std::cout << kix_path << " => Hera File Detected!\n";
        std::exit(EXIT_FAILURE);
      } else {
        std::cout << kix_path << " => Unknown/Raw\n";
        std::exit(EXIT_FAILURE);
      }
    }
  }

  // Read second file
  std::ifstream ifs_kbf;
  if (argc >= 3) {
    std::filesystem::path kbf_path = argv[2];
    ifs_kbf.open(kbf_path, std::ios::in | std::ios::binary);
    if (!ifs_kbf.is_open()) {
      std::cerr << "ERROR: cannot open KBF file '" << kbf_path << '\n';
      std::exit(EXIT_FAILURE);
    }

    try {
      auto kbf_size = fs::file_size(kbf_path);
      std::cout << "File size = " << kbf_size << "bytes\n";
    } catch(fs::filesystem_error& e) {
      std::cerr << e.what() << '\n';
      std::exit(EXIT_FAILURE);
    }
    extract = true;
  }

  try {
    std::filesystem::path base_path = fs::current_path();
    std::cout << "Current dir = " << base_path << "\n";

    if(extract)
      ku::parseKixBlock(base_path, ifs_kix, ifs_kbf);
    else
      ku::parseKixBlock(base_path, ifs_kix);
  } catch(fs::filesystem_error& e) {
    std::cerr << e.what() << '\n';
    std::exit(EXIT_FAILURE);
  }

  return EXIT_FAILURE;
}
