#include "file.hpp"
#include <assert.h>

namespace ENGINE::FS {

File::~File(void) {
	close();
}

size_t Provider::loadData(void *output, size_t length, const char *path) {
	auto file = openFile(path);

	if (!file)
		return 0;

	assert(file->size >= length);

	size_t actuallen = file->read(output, length);

	file->close();
	delete file;
	return actuallen;
}

Provider::~Provider(void) {
	close();
}

}