#ifndef _CUBICAT_SPINE_EXTENSION_H_
#define _CUBICAT_SPINE_EXTENSION_H_
#include "spine/Extension.h"

using namespace spine;

class CubicatSpineExtension : public SpineExtension {
public:
    CubicatSpineExtension();
    virtual ~CubicatSpineExtension() = default;
    static void init();
protected:
    virtual void *_alloc(size_t size, const char *file, int line) override;

    virtual void *_calloc(size_t size, const char *file, int line) override;

    virtual void *_realloc(void *ptr, size_t size, const char *file, int line) override;

    virtual void _free(void *mem, const char *file, int line) override;

    virtual char *_readFile(const String &path, int *length) override;
};

#endif