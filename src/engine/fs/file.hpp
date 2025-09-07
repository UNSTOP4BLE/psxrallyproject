#pragma once

#include <stdint.h>
#include <stddef.h>

namespace ENGINE::FS {

class File {
public:
	uint32_t size;

	virtual size_t read(void *output, size_t length) { return 0; }
	virtual uint32_t seek(uint32_t offset) { return 0; }
	virtual uint32_t tell(void) const { return 0; }
	virtual void close(void) {}
	virtual ~File(void);
};

class Provider {
public:
	inline Provider(void) {}
    
	template<class T> inline size_t readStruct(T &output, const char *path) {
		return loadData(&output, sizeof(output), path);
	}
	virtual File *openFile(const char *path) { return nullptr; }
	virtual size_t loadData(void *output, size_t length, const char *path);
	virtual void close(void) {}

	virtual ~Provider(void);
};

}