#include "texture_loader.h"
#include "cubicat.h"
#include "libpng/png.h"
#include "utils/logger.h"

GetImageDataInMemory CubicatTextureLoader::m_pGetImageInMemory = nullptr;
ResLocation CubicatTextureLoader::m_eResLocation = MEMORY;

png_voidp
PNGCBAPI png_spine_malloc(png_structp png_ptr, png_alloc_size_t size)
{
    if (size == 0)
      return (NULL);
    return (png_voidp)psram_prefered_malloc(size);
}

/* Free a pointer.  It is removed from the list at the same time. */
void PNGCBAPI
png_spine_free(png_structp png_ptr, png_voidp ptr)
{
   free(ptr);
   ptr = NULL;
}

void CubicatTextureLoader::init(ResLocation location, GetImageDataInMemory getImageInMemory) {
    m_pGetImageInMemory = getImageInMemory;
    m_eResLocation = location;
}

void CubicatTextureLoader::load(AtlasPage &page, const String &path) {
    if (m_eResLocation == MEMORY) {
        if (m_pGetImageInMemory) {
            auto& img = m_pGetImageInMemory(path.buffer());
#if (CONFIG_SPINE_VERSION_38 || CONFIG_SPINE_VERSION_40)
            page.setRendererObject((void*)img.data);
            page.texturePath = path;
#elif CONFIG_SPINE_VERSION_42
            page.texture = (void*)img.data;
#else
    #error "Spine version not supported"
#endif
            page.width = img.width;
            page.height = img.height;
        } else {
            LOGE("GetImageDataInMemory function is not set");
        }
    } else {
        std::string ext = std::string(path.buffer()).substr(path.length() - 4);
        if (ext == ".png") {
            loadPNG(page, path.buffer());
        } else {
            LOGI("not png file %s", path.buffer());
        }
    }
}
void CubicatTextureLoader::unload(void *texture) {
    if (texture) {
		free(texture);
	}
}

void CubicatTextureLoader::loadPNG(AtlasPage &page, const char* path) {
    png_structp png = png_create_read_struct_2(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr, nullptr, png_spine_malloc, png_spine_free);
    if (!png) {
        return;
    }
    FILE* fp = nullptr;
    if (m_eResLocation == SPIFFS) {
        fp = CUBICAT.storage.openFileFlash(path);
    } else {
        fp = CUBICAT.storage.openFileSD(path);
    }
    if (!fp) {
        png_destroy_read_struct(&png, nullptr, nullptr);
        LOGI("open file %s fail", path);
        return;
    }
    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_read_struct(&png, nullptr, nullptr);
        fclose(fp);
        return;
    }
    if (setjmp(png_jmpbuf(png))) {
        png_destroy_read_struct(&png, &info, nullptr);
        fclose(fp);
        return;
    }
    png_init_io(png, fp);
    png_read_info(png, info);

    auto width = png_get_image_width(png, info);
    auto height = png_get_image_height(png, info);
    png_byte colorType = png_get_color_type(png, info);
    png_byte bitDepth = png_get_bit_depth(png, info);
    if (bitDepth != 8) {
        assert(false && "not support bit depth not 8 yet");
    }
    // 处理透明度
    if (colorType == PNG_COLOR_TYPE_PALETTE) {
        assert(false && "not support palette yet");
        png_set_palette_to_rgb(png);
    }
    if (colorType == PNG_COLOR_TYPE_GRAY) {
        assert(false && "not support gray yet");
        png_set_expand_gray_1_2_4_to_8(png);
    }
    if (png_get_valid(png, info, PNG_INFO_tRNS)) {
        png_set_tRNS_to_alpha(png);
    }
    if (bitDepth == 16) {
        png_set_strip_16(png);
    }
    // 设置转换后的颜色类型
    png_read_update_info(png, info);
    // 读取图像数据
    bool noAlpha = colorType == PNG_COLOR_TYPE_RGB;
    uint8_t bytePerPixel = noAlpha?3:4;
    uint8_t bitPerPixel = noAlpha?16:32;
    uint8_t* imgData = (uint8_t*)heap_caps_malloc(width * height * bytePerPixel, MALLOC_CAP_SPIRAM);
    if (!imgData) {
        png_destroy_read_struct(&png, &info, nullptr);
        fclose(fp);
        return;
    }
    uint8_t* rowPointers[height];
    for (int y = 0; y < height; ++y) {
        rowPointers[y] = imgData + y * width * bytePerPixel;
    }
    png_read_image(png, rowPointers);
    if (noAlpha) {
        uint16_t* rgb565 = (uint16_t*)heap_caps_malloc(width * height * 2, MALLOC_CAP_SPIRAM);
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                uint8_t* pixel = rowPointers[y] + x * bytePerPixel;
                uint16_t r = pixel[0] >> 3;
                uint16_t g = pixel[1]  >> 2;
                uint16_t b = pixel[2]  >> 3;
                rgb565[y * width + x] = (uint16_t)((r << 11) | (g << 5) | b);
            }
        }
        free(imgData);
        imgData = (uint8_t*)rgb565;
    } else {
        uint32_t* rgba8888 = (uint32_t*)imgData;
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                uint8_t* pixel = rowPointers[y] + x * bytePerPixel;
                uint32_t r = pixel[0];
                uint32_t g = pixel[1];
                uint32_t b = pixel[2];
                uint32_t a = pixel[3];
                rgba8888[y * width + x] = (uint32_t)((r << 24) | (g << 16) | (b << 8) | a);
            }
        }
    }
    cubicat::Texture* texture = NEW cubicat::Texture(width, height, imgData, true, 1, 1, nullptr, bitPerPixel, !noAlpha);
    png_destroy_read_struct(&png, &info, nullptr);
    fclose(fp);
#if defined(CONFIG_SPINE_VERSION_38) || defined(CONFIG_SPINE_VERSION_40)
    page.texturePath = path;
    page.setRendererObject(texture);
#else
    page.texture = texture;
#endif
    page.width = width;
    page.height = height;
}