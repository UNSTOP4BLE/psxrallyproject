#include "host.hpp"
#include "ps1/pcdrv.h"
#include <assert.h>

namespace ENGINE::FS {

/* PCDRV file and directory classes */
size_t HostFile::read(void *output, size_t length) {
	int actuallen = pcdrvRead(m_fd, output, length); 
    assert(actuallen >= 0);
	return size_t(actuallen);
}

uint32_t HostFile::seek(uint32_t offset) {
	int actualoff = pcdrvSeek(m_fd, int(offset), PCDRV_SEEK_SET);
	assert(actualoff >= 0);
	return uint32_t(actualoff);
}

uint32_t HostFile::tell(void) const {
	int actualoff = pcdrvSeek(m_fd, 0, PCDRV_SEEK_CUR);
    assert(actualoff >= 0);
	return uint32_t(actualoff);
}

void HostFile::close(void) {
	delete[] fdata;
	pcdrvClose(m_fd);
}

/* PCDRV filesystem provider */

bool HostProvider::init(void) {
	int error = pcdrvInit();
    assert(error >= 0);
	return true;
}

File *HostProvider::openFile(const char *path) {
	PCDRVOpenMode mode = PCDRV_MODE_READ;
	int fd = pcdrvOpen(path, mode);

    assert(fd >= 0);

	auto file  = new HostFile();
	file->m_fd  = fd;
	file->size = pcdrvSeek(fd, 0, PCDRV_SEEK_END);

	pcdrvSeek(fd, 0, PCDRV_SEEK_SET);

	file->fdata = new uint8_t[file->size];
	file->read(file->fdata, file->size);

	pcdrvSeek(fd, 0, PCDRV_SEEK_SET);

	
	return file;
}

}