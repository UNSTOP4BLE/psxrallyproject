#pragma once 

#include "file.hpp"

namespace ENGINE::FS {

class HostFile : public File {
	friend class HostProvider;

public:
	size_t read(void *output, size_t length);
	uint32_t seek(uint32_t offset);
	uint32_t tell(void) const;
	void close(void);
private:
	int m_fd;
};


class HostProvider : public Provider {
public:
	bool init(void);

	File *openFile(const char *path);

};

}