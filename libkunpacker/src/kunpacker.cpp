#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <iostream>

#include <kunpacker.hpp>

namespace ku {
  constexpr auto KIX_BLOCK = 0;
  constexpr auto KIX_ENTRY = 1;
  bool extract = false;

  struct __attribute__((packed)) kix_hdr_t {
    char name[32];
    uint32_t numRecords;
  };

  struct __attribute__((packed)) kix_node_t {
    std::uint8_t type;
    std::uint32_t memAddr;
    std::uint32_t offset;
    std::uint32_t size;
    std::uint8_t nameLen;
    char name[];
  };

  bool
    isKixFile(const fs::path& path) {
      std::ifstream file;
      file.open(path, std::ios::in | std::ios::binary);
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
      file.open(path, std::ios::in | std::ios::binary);
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
    printKixEntry(int index, struct kix_node_t* node, std::ifstream& kix) {
      kix.read(reinterpret_cast<char*>(&node), sizeof(kix_node_t));
      std::cout << "#" << index << std::hex
        << " @0x" << kix.tellg() << "->"
        << "0x" << node->memAddr << " "
        << "0x" << node->offset << " ("
        << std::dec << node->size << "bytes) "
        << node->type << " " << 0 << " "
        << node->nameLen << " " << node->name << "\n";
    }

  void
    printKixBlock(struct kix_hdr_t* hdr, struct kix_node_t* node, std::ifstream& kix) {
      std::cout << "--- KIX BLOCK 0x" << std::hex << kix.tellg() << " ---\n";
      kix.read(reinterpret_cast<char*>(hdr), sizeof(kix_hdr_t));
      std::cout << "Name: " << hdr->name << "\n";
      std::cout << "nRecs: " << std::dec << hdr->numRecords << "\n";
      printKixEntry(0, node, kix);
      std::cout << "-----------------------\n";
    }

  void
    parseKixBlock(const fs::path& basedir, std::ifstream& kix) {
      auto pos = kix.tellg();
      struct kix_hdr_t hdr;
      struct kix_node_t node;
      printKixBlock(&hdr, &node, kix);

      for (int i = 0; i < hdr.numRecords; i++) {
        // nested KIX headers indicate directories
        if (node.type == KIX_BLOCK) {
          std::cout << "[DBG] parsing directory\n";
          kix.seekg(pos + node.offset);
          struct kix_hdr_t dirent;
          kix.read(reinterpret_cast<char*>(&dirent), sizeof(kix_hdr_t));

          kix_hdr_t* dirent = (kix_hdr_t*)(kix_map.start + node.offset);
          char* subdir;
          asprintf(&subdir, "%s/%.32s", basedir, dirent.name);

          mkdir(subdir, (mode_t)0777);

          parseKixBlock(subdir, kix);
          free(subdir);
        }
        else {
          // Extract file entry
          printKixEntry(i, node, kix);
          pos += node->nameLen;
        }
      }
    }

  void
    parseKixBlock(const fs::path& basedir, std::ifstream& kix, std::ifstream& kbf) {
      struct kix_hdr_t hdr;
      printKixBlock(&hdr, kix_map);
      /*
      int i, parsed = 0;
      for (i = 0; i < hdr.numRecords; i++) {
        kix_node_t* node = (kix_node_t*)start;
        start += sizeof(*node);
        parsed += sizeof(*node);
        // nested KIX headers indicate directories
        if (node->type == KIX_BLOCK) {
          std::cout << "[DBG] parsing directory\n";
          kix_hdr_t* dirent = (kix_hdr_t*)(kix_map.start + node->offset);
          char* subdir;
          asprintf(&subdir, "%s/%.32s", basedir, dirent->name);

          mkdir(subdir, (mode_t)0777);

          parsed += parse_kix_block(subdir, dirent);
          parsed--;
          start--; // block doesn't have string size field

          free(subdir);
        } else {
          // Extract file entry
          printKixEntry(i, node, kix);
          if (extract) {
            extract_kix_entry(basedir, node);
          }
          start += node->nameLen;
          parsed += node->nameLen;
        }
      }
      */
    }

  /*
     map_t kix_map, kbf_map;

     void
     map_new(map_t* map, void* start, std::size_t size) {
     map->start = (uintptr_t)start;
     map->size  = size;
     map->end   = map->start + size;
     }

     void
     print_kix_entry(int index, kix_node_t* node, const map_t& kix_map) {
     printf("#%-4d @0x%x->   0x%08x 0x%08x (%08zu bytes)    %d    %*.*s\n",
     index,
     (uintptr_t)node - kix_map.start,
     node->memAddr,
     node->offset,
     node->size,
     node->type,
     0,
     node->nameLen,
     node->name);
     }

     int
     extractHeraFile(const std::filesystem::path& path, int startOff) {
     int err = 0;

     FILE* hera = fopen(path, "rb");
     if (!hera) {
     err = -1;
     goto exit_s0;
     }

     struct stat statBuf;
     if (fstat(fileno(hera), &statBuf) < 0) {
     perror("ftsat");
     err = -3;
     goto exit_s1;
     }

// char *pathHdr, *pathData;
char* pathData;
// asprintf(&pathHdr, "%s.hdr", path);
asprintf(&pathData, "%s.bin", path);

// FILE *heraHdr = fopen(pathHdr, "wb");
FILE* heraData = fopen(pathData, "wb");
if (!heraData) {
err = -2;
goto exit_s2;
}

fseek(hera, startOff, SEEK_SET);

char* buf = calloc(1, statBuf.st_size);
fread(buf, 1, statBuf.st_size - startOff, hera);

fwrite(buf, 1, statBuf.st_size - startOff, heraData);

fflush(heraData);
fclose(heraData);

free(buf);

exit_s2:
free(pathData);
exit_s1:
fclose(hera);
exit_s0:
return err;
}

int
extract_kix_entry(char* basedir, kix_node_t* node) {
  if (kbf_map.start == 0) return -1;
  if (node->offset + node->size > kbf_map.size) {
    fprintf(stderr,
        "ERROR: cannot extract! out of boundary (0x%08X >= 0x%08X)!\n",
        node->offset + node->size,
        kbf_map.size);
    return -2;
  }

  // Get the file entry referenced by this index entry
  kbf_node_t* data = (kbf_node_t*)(kbf_map.start + node->offset);

  char* filePath;
  asprintf(&filePath, "%s/%.32s", basedir, data->name);
  printf("Extracting to '%s'\n", filePath);

  FILE* out = fopen(filePath, "wb");
  if (!out) {
    fprintf(stderr, "ERROR: cannot open output file '%s' for writing\n", filePath);
    free(filePath);
    return -3;
  }

  free(filePath);

  int written = fwrite(data->data, 1, node->size, out);

  fflush(out);
  fclose(out);

  if (written != node->size) {
    fprintf(
        stderr, "WARNING: Expected %zu bytes, but written %zu\n", node->size, written);
    return -4;
  }
}

void
print_kix_block(kix_hdr_t* hdr, const map_t& kix_map) {
  printf("--- KIX BLOCK @0x%x ---\n", (uintptr_t)hdr - kix_map.start);
  printf("Name: %.32s\n", hdr->name);
  printf("nRecs: %d\n", hdr->numRecords);
  print_kix_entry(0, (kix_node_t*)((uintptr_t)hdr + sizeof(*hdr)));
  printf("-----------------------\n");
}

long int
parse_kix_block(char* basedir, kix_hdr_t* hdr, const map_t& kix_map) {
  uintptr_t start = (uintptr_t)hdr;

  print_kix_block(hdr, kix_map);
  start += sizeof(*hdr);

  int i, parsed = 0;
  for (i = 0; i < hdr->numRecords; i++) {
    kix_node_t* node = (kix_node_t*)start;
    start += sizeof(*node);
    parsed += sizeof(*node);
    // nested KIX headers indicate directories
    if (node->type == KIX_BLOCK) {
      printf("[DBG] parsing directory\n");
      kix_hdr_t* dirent = (kix_hdr_t*)(kix_map.start + node->offset);
      char* subdir;
      asprintf(&subdir, "%s/%.32s", basedir, dirent->name);

      mkdir(subdir, (mode_t)0777);

      parsed += parse_kix_block(subdir, dirent);
      parsed--;
      start--; // block doesn't have string size field

      free(subdir);
    } else {
      // Extract file entry
      print_kix_entry(i, node);
      if (extract) {
        extract_kix_entry(basedir, node);
      }
      start += node->nameLen;
      parsed += node->nameLen;
    }
  }
  return parsed;
}
*/
} // namespace ku
