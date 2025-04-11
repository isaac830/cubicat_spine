#include "spine_extension.h"
#include "esp_heap_caps.h"
#include <string.h>
#include "spine/SpineString.h"
#include "cubicat.h"

CubicatSpineExtension* g_Instance = nullptr;

CubicatSpineExtension::CubicatSpineExtension() {
}
void CubicatSpineExtension::init() {
    if (!g_Instance) {
        g_Instance = new CubicatSpineExtension();
        SpineExtension::setInstance(g_Instance);
    }
}
void* CubicatSpineExtension::_alloc(size_t size, const char *file, int line) {
    return psram_prefered_malloc(size);
}

void* CubicatSpineExtension::_calloc(size_t size, const char *file, int line) {
    void* ptr = _alloc(size, file, line); 
    if (ptr) {
        memset(ptr, 0, size);
        return ptr;
    }
    return  nullptr;
}

void* CubicatSpineExtension::_realloc(void *ptr, size_t size, const char *file, int line) {
    return psram_prefered_realloc(ptr, size);
}

void CubicatSpineExtension::_free(void *mem, const char *file, int line) {
    heap_caps_free(mem);
}

char* CubicatSpineExtension::_readFile(const String &path, int *length) {
    char *data;
	FILE *file = CUBICAT.storage.openFileFlash(path.buffer());
	if (!file) {
        return 0;
    }

	fseek(file, 0, SEEK_END);
	*length = (int) ftell(file);
	fseek(file, 0, SEEK_SET);

	data = SpineExtension::alloc<char>(*length, __FILE__, __LINE__);
	fread(data, 1, *length, file);
	fclose(file);
	return data;
}