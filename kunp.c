/*
 * Copyright 2018 Smx
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "kallisto.h"

typedef struct {
	uintptr_t start;
	uintptr_t end;
	size_t size;
} map_t;
	
map_t kix_map, kbf_map;

bool extract = false;

void map_new(map_t *map, void *start, size_t size){
	map->start = (uintptr_t)start;
	map->size = size;
	map->end = map->start + size;
}

void print_kix_entry(int index, kix_node_t *node){
	printf("#%-4d @0x%x->   0x%08x 0x%08x (%08zu bytes)    %d    %*.*s\n", 
		index, (uintptr_t)node - kix_map.start,
		node->memAddr, node->offset, node->size,
		node->type,
		0, node->nameLen, node->name
	);
}

int isHeraData(uint8_t *data){
	int i, steps=0;
	char ch;
	for(i=0; ;i++){
		if(i+10 >= 32)
			return false;
		if(!strncmp((char *)(data+i), " \r\n%%%% \r\n", 10))
			return i;
	}
	return -1;
}

int isHeraFile(const char *path){
	FILE *file = fopen(path, "rb");
	if(!file)
		return -1;
	char head[32];
	memset(&head, 0x00, 32);
	
	int read = fread(&head, 1, 32, file);
	fclose(file);
	
	return isHeraData((uint8_t *)&head);
}

int extractHeraFile(const char *path, int startOff){
	int err = 0;
	
	FILE *hera = fopen(path, "rb");
	if(!hera){
		err = -1;
		goto exit_s0;
	}
	
	struct stat statBuf;
	if(fstat(fileno(hera), &statBuf) < 0){
		perror("ftsat");
		err = -3;
		goto exit_s1;
	}
	
	//char *pathHdr, *pathData;
	char *pathData;
	//asprintf(&pathHdr, "%s.hdr", path);
	asprintf(&pathData, "%s.bin", path);
	
	//FILE *heraHdr = fopen(pathHdr, "wb");
	FILE *heraData = fopen(pathData, "wb");
	if(!heraData){
		err = -2;
		goto exit_s2;
	}
	
	fseek(hera, startOff, SEEK_SET);
	
	char *buf = calloc(1, statBuf.st_size);
	fread(buf, 1, statBuf.st_size - startOff, hera);
	
	fwrite(buf, 1, statBuf.st_size - startOff, heraData);
	
	fflush(heraData); fclose(heraData);
	
	free(buf);
	
	exit_s2:
		free(pathData);
	exit_s1:
		fclose(hera);
	exit_s0:
		return err;
}

bool isKixData(uint8_t *data){
	kix_node_t *root = (kix_node_t *)(data + sizeof(kix_hdr_t));
	if(root->type != KIX_BLOCK && root->type != KIX_ENTRY)
		return false;
	if(root->nameLen > 32)
		return false;
	return true;
}

int isKixFile(const char *path){
	FILE *file = fopen(path, "rb");
	if(!file)
		return -1;
	size_t headSz = sizeof(kix_hdr_t) + sizeof(kix_node_t);
	char head[headSz];
	memset(&head, 0x00, headSz);
	
	fread(&head, 1, headSz, file);
	fclose(file);
	
	return isKixData((uint8_t *)&head);
}

int extract_kix_entry(kix_node_t *node){
	if(kbf_map.start == 0)
		return -1;
	if(node->offset + node->size > kbf_map.size){
		fprintf(stderr, "ERROR: cannot extract! out of boundary (0x%08X >= 0x%08X)!\n", node->offset + node->size, kbf_map.size);
		return -2;
	}
	kbf_node_t *data = (kbf_node_t *)(kbf_map.start + node->offset);
	printf("Extracting to '%.32s'\n", data->name);
	char tmp[33];
	memset(&tmp, 0x00, 33);
	strncpy(tmp, data->name, 33);
	FILE *out = fopen(tmp, "wb");
	if(!out){
		fprintf(stderr, "ERROR: cannot open output file '%s' for writing\n", data->name);
		return -3;
	}
	int written = fwrite(data->data, 1, node->size, out);
	
	fflush(out); fclose(out);
	
	if(written != node->size){
		fprintf(stderr, "WARNING: Expected %zu bytes, but written %zu\n", node->size, written);
		return -4;
	}
}

void print_kix_block(kix_hdr_t *hdr){
	printf("--- KIX BLOCK @0x%x ---\n", (uintptr_t)hdr - kix_map.start);
	printf("Name: %.32s\n", hdr->name);
	printf("nRecs: %d\n", hdr->numRecords);
	print_kix_entry(0, (kix_node_t *)((uintptr_t)hdr + sizeof(*hdr)));
	printf("-----------------------\n");
}

long int parse_kix_block(kix_hdr_t *hdr){
	uintptr_t start = (uintptr_t)hdr;

	print_kix_block(hdr);
	start += sizeof(*hdr);
	
	int i, parsed = 0;
	for(i=0; i<hdr->numRecords; i++){
		kix_node_t *node = (kix_node_t *)start;
		start += sizeof(*node);
		parsed += sizeof(*node);
		if(node->type == KIX_BLOCK){
			printf("[DBG] parsing block\n");
			parsed += parse_kix_block((kix_hdr_t *)(kix_map.start + node->offset));
			parsed--; start--; //block doesn't have string size field
		} else {
			print_kix_entry(i, node);
			if(extract){
				extract_kix_entry(node);
			}
			start += node->nameLen;
			parsed += node->nameLen;
		}
	}
	return parsed;
}

int main(int argc, char **argv){
	printf("Kallisto Index File (KIX) | Kallisto Binary File (KBF) Unpacker\n");
	printf("Copyright (C) 2015 Smx\n\n");
	if(argc < 2){
		fprintf(stderr, "Usage: %s [file.kix] [file.kbf]\n", argv[0]);
		return EXIT_FAILURE;
	}
		
	int fd_kix, fd_kbf;
	void *map, *map2;
	struct stat kix_statBuf, kbf_statBuf;
	
	if(argc > 2){
		extract = true;
		if((fd_kbf = open(argv[2], O_RDONLY)) < 0){
			fprintf(stderr, "ERROR: cannot open KBF file '%s' (%s)\n", argv[1], strerror(errno));
			return EXIT_FAILURE;
		}
		if(stat(argv[2], &kbf_statBuf) < 0){
			perror("fstat failed");
			return EXIT_FAILURE;
		}
		if((map2 = mmap(0, kix_statBuf.st_size, PROT_READ, MAP_SHARED, fd_kbf, 0)) == MAP_FAILED){
			perror("mmap");
			return EXIT_FAILURE;
		}
		map_new(&kbf_map, map2, kbf_statBuf.st_size);
	}
	
	int pos;
	
	if(isKixFile(argv[1])){
		printf("'%-50s' => KIX File Detected!\n", argv[1]);
		if(stat(argv[1], &kix_statBuf) < 0){
			perror("fstat failed");
			return EXIT_FAILURE;
		}
		
		if((fd_kix=open(argv[1], O_RDONLY)) < 0){
			fprintf(stderr, "ERROR: cannot open KIX file '%s' (%s)\n", argv[1], strerror(errno));
			return EXIT_FAILURE;
		}
		
		if((map = mmap(0, kix_statBuf.st_size, PROT_READ, MAP_SHARED, fd_kix, 0)) == MAP_FAILED){
			perror("mmap");
			return EXIT_FAILURE;
		}
		
		map_new(&kix_map, map, kix_statBuf.st_size);
		
		int parsed = parse_kix_block((kix_hdr_t *)(kix_map.start));
		printf("[DBG] Done!, parsed %d bytes\n", parsed);
		
		munmap(map, kix_statBuf.st_size);
		close(fd_kix);
		if(extract){
			munmap(map2, kbf_statBuf.st_size);
			close(fd_kbf);
		}
	} else if((pos=isHeraFile(argv[1])) > -1){
		printf("'%-50s' => Hera File Detected!\n", argv[1]);
	} else {
		printf("'%-50s' => Unknown/Raw\n", argv[1]);
	}
	return EXIT_SUCCESS;
}