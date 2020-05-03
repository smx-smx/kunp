/**
 * MIT License
 * Copyright(C) 2020 Stefano Moioli <smxdev4@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 *all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 **/
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "kallisto.h"
#include "mfile.h"

#ifdef _WIN32
#define MKDIR(x) mkdir(x)
#else
#define MKDIR(x) mkdir(x, (mode_t)0777)
#endif

class HeraUnpacker {
 private:
  MFILE* kix = nullptr;
  MFILE* kbf = nullptr;

  static bool isKixData(uint8_t* data) {
    kix_node_t* root = (kix_node_t*)(data + sizeof(kix_node_t));
    return (root->type == kixNodeType::Directory);
  }

  static bool isKixFile(MFILE* mf) {
    size_t headSz = sizeof(kix_hdr_t) + sizeof(kix_node_t);
    if (msize(mf) < headSz) {
      return false;
    }

    uint8_t* head = mdata(mf, uint8_t);
    return isKixData(head);
  }

  int extract_kix_entry(const char* basedir, kix_node_t* node) {
    uint8_t* kbf_start = mdata(kbf, uint8_t);

    size_t kbf_size = msize(kbf);
    if (node->offset + node->size > kbf_size) {
      std::cerr << util::ssprintf(
          "ERROR: cannot extract! out of boundary (0x%08X >= 0x%08X)!\n",
          node->offset + node->size, kbf_size);
      return -2;
    }

    // Get the file entry referenced by this index entry
    kbf_node_t* data = (kbf_node_t*)(kbf_start + node->offset);

    std::string filePath = util::ssprintf("%s/%.32s", basedir, data->name);
    std::cout << util::ssprintf("Extracting to '%s'\n", filePath.c_str());

    std::ofstream out(filePath, std::ofstream::binary);
    if (!out.is_open()) {
      std::cerr << util::ssprintf(
          "ERROR: cannot open output file '%s' for writing\n",
          filePath.c_str());
      return -3;
    }
    out.write((char*)data->data, node->size);
    off_t written = out.tellp();
    if (written != node->size) {
      std::cerr << util::ssprintf(
          "WARNING: Expected %zu bytes, but written %zu\n", node->size,
          written);
      return -4;
    }

    return 0;
  }

  long int parse_kix_block(const char* basedir, kix_hdr_t* hdr) {
    uintptr_t start = (uintptr_t)hdr;

    hdr->print(kix);
    std::cout << "-----------------------\n";
    start += sizeof(*hdr);

    long int parsed = sizeof(*hdr);
    for (uint i = 0; i < hdr->numRecords; i++) {
      kix_node_t* node = (kix_node_t*)start;

      // nested KIX headers indicate directories
      switch (node->type) {
        case kixNodeType::Directory: {
          start += sizeof(*node);
          parsed += sizeof(*node);

          node->print(i, kix);
          kix_hdr_t* dirent = (kix_hdr_t*)(mdata(kix, uint8_t) + node->offset);
          std::string subdir =
              util::ssprintf("%s/%.32s", basedir, dirent->name);

          MKDIR(subdir.c_str());

          std::cout << '\n';
          parsed += parse_kix_block(subdir.c_str(), dirent);
          break;
        }
        case kixNodeType::File: {
          kix_filenode_t* file = (kix_filenode_t*)start;
          start += sizeof(*file);
          parsed += sizeof(*file);

          // Extract file entry
          file->print(i, kix);
          extract_kix_entry(basedir, file);

          start += file->nameLen;
          parsed += file->nameLen;
          break;
        }
        default:
          throw util::ssprintf("Invalid block type '%d'", node->type);
      }
    }
    return parsed;
  }

 public:
  HeraUnpacker(const char* kixPath, const char* kbfPath) {
    this->kix = mopen(kixPath, O_RDONLY);
    if (!this->kix) {
      throw std::invalid_argument(
          util::ssprintf("Cannot open KIX file '%s' for reading", kixPath));
    }
    if (!isKixFile(this->kix)) {
      mclose(this->kix);
      this->kix = nullptr;
      throw std::invalid_argument(
          util::ssprintf("'%s' is not a valid KIX file", kixPath));
    }

    this->kbf = mopen(kbfPath, O_RDONLY);
    if (!this->kbf) {
      throw std::invalid_argument(
          util::ssprintf("Cannot open KBF file '%s' for reading", kbfPath));
    }
  }

  size_t kix_size() { return msize(kix); }
  size_t kbf_size() { return msize(kbf); }

  long int parse_kix_block(const char* basedir) {
    return parse_kix_block(basedir, mdata(kix, kix_hdr_t));
  }

  ~HeraUnpacker() {
    if (this->kix) {
      mclose(this->kix);
    }
    if (this->kbf) {
      mclose(this->kbf);
    }
  }
};

int main(int argc, char** argv) {
  std::cout
      << "Kallisto Index File (KIX) | Kallisto Binary File (KBF) Unpacker\n";
  std::cout << "Copyright (C) 2020 Smx\n\n";
  if (argc < 3) {
    std::cerr << util::ssprintf("Usage: %s [file.kix] [file.kbf]\n", argv[0]);
    return EXIT_SUCCESS;
  }

  std::filesystem::path cwd = std::filesystem::current_path();
  std::filesystem::path kixName =
      std::filesystem::path(argv[1]).filename().replace_extension("");

  std::string targetDirPath = (cwd / kixName).string();

  const char* targetDir = targetDirPath.c_str();
  MKDIR(targetDir);

  int parsed = 0;
  try {
    HeraUnpacker unp(argv[1], argv[2]);
    parsed = unp.parse_kix_block(targetDir);
    std::cout << util::ssprintf("Done!, parsed %zu/%zu bytes\n", parsed,
                                unp.kix_size());
    return EXIT_SUCCESS;
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
}
