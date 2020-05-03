/*
 * Copyright 2018 Smx
 */
#pragma once

#include <cstdint>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

#include "mfile.h"

namespace util {
template <typename... Args>
std::string ssprintf(const char *format, Args... args) {
  int length = std::snprintf(nullptr, 0, format, args...);
  assert(length >= 0);

  std::vector<char> buf(length + 1);
  std::snprintf(buf.data(), length + 1, format, args...);

  std::string str(buf.data());
  return str;
}
}  // namespace util

enum class kixNodeType : uint8_t { Directory = 0, File = 1 };

struct __attribute__((packed)) kix_node_t {
  kixNodeType type;
  std::uint32_t memAddr;
  std::uint32_t offset;
  std::uint32_t size;

  void print(int index, MFILE *kix) {
    std::cout << util::ssprintf(
        "#%-4d @0x%x->   0x%08x 0x%08x (%08zu bytes)    %c    \n", index,
        moff(kix, this), memAddr, offset, size,
        type == kixNodeType::Directory ? 'd' : 'f');
  }
};

struct __attribute__((packed)) kix_filenode_t : kix_node_t {
  std::uint8_t nameLen;
  char name[];

  void print(int index, MFILE *kix) {
    std::cout << util::ssprintf(
        "#%-4d @0x%x->   0x%08x 0x%08x (%08zu bytes)    %c    %*.*s\n", index,
        moff(kix, this), memAddr, offset, size, 'f', 0, nameLen, name);
  }
};

struct __attribute__((packed)) kix_hdr_t {
  char name[32];
  std::uint32_t numRecords;

  void print(MFILE *kix) {
    std::cout << util::ssprintf("--- KIX BLOCK @0x%x ---\n", moff(kix, this));
    std::cout << util::ssprintf("Name: %.32s\n", name);
    std::cout << util::ssprintf("nRecs: %d\n", numRecords);
  }
};

typedef struct __attribute__((packed)) {
  char name[32];
  uint8_t data[];
} kbf_node_t;
