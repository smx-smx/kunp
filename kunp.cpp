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
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
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
#include <iostream>
#include <string>
#include <vector>

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <libgen.h>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "mfile.h"
#include "kallisto.h"

namespace util
{
	template< typename... Args >
	std::string ssprintf(const char *format, Args... args) {
		int length = std::snprintf(nullptr, 0, format, args...);
		assert(length >= 0);

		std::vector<char> buf(length + 1);
		std::snprintf(buf.data(), length + 1, format, args...);

		std::string str(buf.data());
		return str;
	}
}

/**
 * basename and dirname might modify the source path.
 * they also return a pointer to static memory that might be overwritten in subsequent calls
 */
char *my_dirname(const char *path){
	char *cpy = strdup(path);
	char *ret = dirname(cpy);
	ret = strdup(ret);
	free(cpy);
	return ret;
}

class HeraUnpacker {
	private:
	MFILE *kix;
	MFILE *kbf;

	static bool isKixData(uint8_t *data){
		kix_node_t *root = (kix_node_t *)(data + sizeof(kix_hdr_t));
		if(root->type != KIX_BLOCK && root->type != KIX_ENTRY)
			return false;
		if(root->nameLen > 32)
			return false;
		return true;
	}

	static bool isKixFile(MFILE *mf){
		size_t headSz = sizeof(kix_hdr_t) + sizeof(kix_node_t);
		if(msize(mf) < headSz){
			return false;
		}

		uint8_t *head = mdata(mf, uint8_t);
		return isKixData(head);
	}

	void print_kix_entry(int index, kix_node_t *node){
		std::cout << util::ssprintf("#%-4d @0x%x->   0x%08x 0x%08x (%08zu bytes)    %d    %*.*s\n", 
			index,
			(uintptr_t)node - (uintptr_t)mdata(kix, uint8_t),
			node->memAddr, node->offset, node->size,
			node->type,
			0, node->nameLen, node->name
		);
	}

	int extract_kix_entry(const char *basedir, kix_node_t *node){
		uint8_t *kbf_start = mdata(kbf, uint8_t);

		size_t kbf_size = msize(kbf);
		if(node->offset + node->size > kbf_size){
			std::cerr << util::ssprintf("ERROR: cannot extract! out of boundary (0x%08X >= 0x%08X)!\n", node->offset + node->size, kbf_size);
			return -2;
		}

		// Get the file entry referenced by this index entry
		kbf_node_t *data = (kbf_node_t *)(kbf_start + node->offset);
		
		std::string filePath = util::ssprintf("%s/%.32s", basedir, data->name);
		std::cout << util::ssprintf("Extracting to '%s'\n", filePath.c_str());

		FILE *out = fopen(filePath.c_str(), "wb");
		if(!out){
			std::cerr << util::ssprintf("ERROR: cannot open output file '%s' for writing\n", filePath.c_str());
			return -3;
		}

		int written = fwrite(data->data, 1, node->size, out);
		
		fflush(out);
		fclose(out);
		
		if(written != node->size){
			std::cerr << util::ssprintf("WARNING: Expected %zu bytes, but written %zu\n", node->size, written);
			return -4;
		}

		return 0;
	}

	void print_kix_block(kix_hdr_t *hdr){
		std::cout << util::ssprintf("--- KIX BLOCK @0x%x ---\n", (uintptr_t)hdr - (uintptr_t)mdata(kix, uint8_t));
		std::cout << util::ssprintf("Name: %.32s\n", hdr->name);
		std::cout << util::ssprintf("nRecs: %d\n", hdr->numRecords);
		print_kix_entry(0, (kix_node_t *)((uintptr_t)hdr + sizeof(*hdr)));
		std::cout << "-----------------------\n";
	}

	long int parse_kix_block(const char *basedir, kix_hdr_t *hdr){
		uintptr_t start = (uintptr_t)hdr;

		print_kix_block(hdr);
		start += sizeof(*hdr);
		
		int i, parsed = 0;
		for(i=0; i<hdr->numRecords; i++){
			kix_node_t *node = (kix_node_t *)start;
			start += sizeof(*node);
			parsed += sizeof(*node);
			// nested KIX headers indicate directories
			if(node->type == KIX_BLOCK){
				std::cout << "[DBG] parsing directory\n";
				kix_hdr_t *dirent = (kix_hdr_t *)(mdata(kix, uint8_t) + node->offset);
				std::string subdir = util::ssprintf("%s/%.32s", basedir, dirent->name);

	#ifdef _WIN32
				mkdir(subdir.c_str());
	#else
				mkdir(subdir.c_str(), (mode_t)0777);
	#endif

				parsed += parse_kix_block(subdir.c_str(), dirent);
				parsed--; start--; //block doesn't have string size field
			} else {
				// Extract file entry
				print_kix_entry(i, node);
				extract_kix_entry(basedir, node);

				start += node->nameLen;
				parsed += node->nameLen;
			}
		}
		return parsed;
	}

	public:
	HeraUnpacker(const char *kixPath, const char *kbfPath){
		this->kix = mopen(kixPath, O_RDONLY);
		if(!this->kix){
			throw util::ssprintf("Cannot open KIX file '%s' for reading");
		}
		if(!isKixFile(this->kix)){
			mclose(this->kix);
			this->kix = nullptr;
			throw util::ssprintf("'%s' is not a valid KIX file", kixPath);
		}

		this->kbf = mopen(kbfPath, O_RDONLY);
		if(!this->kbf){
			throw util::ssprintf("Cannot open KBF file '%s' for reading");
		}
	}

	long int parse_kix_block(const char *basedir){
		return parse_kix_block(basedir, mdata(kix, kix_hdr_t));
	}

	~HeraUnpacker(){
		if(this->kix){
			mclose(this->kix);
		}
		if(this->kbf){
			mclose(this->kbf);
		}
	}
};

int main(int argc, char **argv){
	std::cout << "Kallisto Index File (KIX) | Kallisto Binary File (KBF) Unpacker\n";
	std::cout << "Copyright (C) 2020 Smx\n\n";
	if(argc < 2){
		std::cerr << util::ssprintf("Usage: %s [file.kix] [file.kbf]\n", argv[0]);
		return EXIT_FAILURE;
	}
		
	int fd_kix, fd_kbf;
	void *map, *map2;
	struct stat kix_statBuf, kbf_statBuf;

	char basedir[PATH_MAX + 1];
	if (getcwd(basedir, sizeof(basedir)) == NULL){
		perror("getcwd");
		return EXIT_FAILURE;
	}

	HeraUnpacker unp(argv[1], argv[2]);
	int parsed = unp.parse_kix_block((const char *)&basedir);

	std::cout << util::ssprintf("[DBG] Done!, parsed %d bytes\n", parsed);		
	return EXIT_SUCCESS;
}
