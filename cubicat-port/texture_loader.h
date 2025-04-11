#ifndef _SPINE_TEXTURE_LOADER_H_
#define _SPINE_TEXTURE_LOADER_H_
#include "spine/TextureLoader.h"
#include "spine/Atlas.h"
#include "graphic_engine/drawable/image_data.h"

using namespace spine;

typedef const ImageData& (*GetImageDataInMemory)(const char* name);

enum ResLocation {
    MEMORY,
    SPIFFS,
    SDCARD
};

class CubicatTextureLoader : public TextureLoader {
public:
    static void init(ResLocation location, GetImageDataInMemory getImageInMemory = nullptr);
    
    virtual void load(AtlasPage &page, const String &path);

    virtual void unload(void *texture);
private:
    void loadPNG(AtlasPage &page, const char* path);
    static ResLocation          m_eResLocation;
    static GetImageDataInMemory m_pGetImageInMemory;
};

#endif