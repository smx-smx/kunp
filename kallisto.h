/*
 * Copyright 2018 Smx
 */
#ifndef __KALLISTO_H_
#define __KALLISTO_H_

#include <stdint.h>

#define KIX_BLOCK 0
#define KIX_ENTRY 1

typedef struct __attribute__((packed)) {
	uint8_t type;
	uint32_t memAddr;
	uint32_t offset;
	uint32_t size;
	uint8_t nameLen;
	char name[];
} kix_node_t;

typedef struct __attribute__((packed)) {
	char name[32];
	uint32_t numRecords;
} kix_hdr_t;

typedef struct __attribute__((packed)) {
	char name[32];
	uint8_t data[];
} kbf_node_t;

#endif