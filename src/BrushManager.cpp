
#include <vector>
#include <iostream>
#include <algorithm>
#include <string>
#include <fstream>

#include "BrushManager.h"
#include "stb_image.h"

// ----- Billinear Sampling helper function ----- 
static float bilinearSample(const std::vector<float>& src, int srcW, int srcH, float x, float y) {
    int x0 = static_cast<int>(std::floor(x));
    int y0 = static_cast<int>(std::floor(y));
    int x1 = std::min(srcW - 1, x0 + 1);
    int y1 = std::min(srcH - 1, y0 + 1);
    float sx = x - static_cast<float>(x0);
    float sy = y - static_cast<float>(y0);

    auto at = [&](int xi, int yi) -> float {
        xi = std::clamp(xi, 0, srcW - 1);
        yi = std::clamp(yi, 0, srcH - 1);
        return src[yi * srcW + xi];
        };

    float a00 = at(x0, y0);
    float a10 = at(x1, y0);
    float a01 = at(x0, y1);
    float a11 = at(x1, y1);

    float i0 = a00 + (a10 - a00) * sx;
    float i1 = a01 + (a11 - a01) * sx;
    return i0 + (i1 - i0) * sy;
}



// dab cache for brush pointer and brush size as an integer diameter
struct DabCache {
    const BrushTool* brush;
    int diameter; // our new concept of brush size is the pixel diameter that the tip takes up 
    std::vector<float> dab;
};

static std::vector<DabCache> s_dabCache; 


/*
    Init method that loads in the default brushes. 

    Default brush is 1px. 
*/
void BrushManager::init()
{
    loaded_Brushes.emplace_back(BrushTool(1,1, "DefaultBrush"));

    for (const auto& path : defaultBrushPaths)
    {
        BrushTool temp = BrushTool();
        if (loadBrushFromGBR(path, temp))
            loaded_Brushes.emplace_back(temp);
    }

    activeBrushIndex = 0;

    brushChange = true;
}



// generates and returns a brush dab of the active brush
// the dab contains the width, height, and then the values of the tip alpha
const std::vector<float> BrushManager::generateBrushDab(int requestedBrushSize)
{ 
    if (loaded_Brushes.empty()) {
        return {};
    }

    const BrushTool& activeBrush = loaded_Brushes[activeBrushIndex];

    // clamping to min brush size of 1 pixel, should already be accounted for with UI element 
    // might not be needed  
    //if (requestedBrushSize < 1) requestedBrushSize = 1; 

    // checking cache before doing any calculation 
    for (const auto& cachedDab : s_dabCache) {
        // if there have been no changes since our last dab generation, reuse last dab generation 
        if (cachedDab.brush == &activeBrush && cachedDab.diameter == requestedBrushSize) {
            return cachedDab.dab; 
        }
    }

    // grabbing brush resolution 
    int baseW = activeBrush.tipWidth;
    int baseH = activeBrush.tipHeight;
    // clamping a minimum 
    if (baseW <= 0) baseW = 1;
    if (baseH <= 0) baseH = 1;

    // -- deprecated previous multiplicative brush size 
    // -- remove this entirely after new brush size is complete 
    //int W = baseW * requestedBrushSize;
    //int H = baseH * requestedBrushSize;
     
    // ---- KEY: actual brush size part ----
    // taking the max of the base resolution diamater 
    int baseMax = std::max(baseW, baseH); 
    // determining our scale factor by dividing the UI brush size by the base resolution diameter 
    float scale = static_cast<float>(requestedBrushSize) / static_cast<float>(baseMax); 

    // actual brush height and width accounting for scale factor 
    int W = std::max(1, static_cast<int>(std::round(baseW * scale))); 
    int H = std::max(1, static_cast<int>(std::round(baseH * scale))); 


    std::vector<float> dab;
    dab.reserve(2 + W * H);

    // the first two values in the dab are the width and height of the tip
    dab.push_back(static_cast<float>(W));
    dab.push_back(static_cast<float>(H));

    // for the billinear filtering 
    const auto& src = activeBrush.tipAlpha; 

    // mapping each target pixel to source space and sampling using bilinear filtering 
    float invScaleX = static_cast<float>(baseW) / static_cast<float>(W); 
    float invScaleY = static_cast<float>(baseH) / static_cast<float>(H); 

    for (int y = 0; y < H; ++y)
    {
		float srcY = (y + 0.5f) * invScaleY - 0.5f;
        for (int x = 0; x < W; ++x)
        {
            float srcX = (x + 0.5f) * invScaleX - 0.5f; 
            float sample = bilinearSample(src, baseW, baseH, srcX, srcY); 
            dab.push_back(std::clamp(sample, 0.0f, 1.0f)); 
        }
    }

    // cache it 
    s_dabCache.push_back(DabCache{ &activeBrush, requestedBrushSize, dab });

    return dab;
}



/*
    Getter for loaded brushes.

    @return vector of the loaded brushes
*/
const std::vector<BrushTool>& BrushManager::getLoadedBrushes()
{
    return loaded_Brushes;
}



/*
    ActiveBrush Getter.

    @return pointer to the activeBrush.
*/
const BrushTool& BrushManager::getActiveBrush()
{
    if (loaded_Brushes.empty()) {
        static BrushTool fallback;
        return fallback;
    }

    if (activeBrushIndex < 0 || activeBrushIndex >= static_cast<int>(loaded_Brushes.size())) {
        activeBrushIndex = 0;
    }

    return loaded_Brushes[activeBrushIndex];
}



/*
    ActiveBrush Setter.

    @param index: The index of the brush to be set as the active brush. 
*/
void BrushManager::setActiveBrush(int index) 
{
    if (index >= 0 && index < static_cast<int>(loaded_Brushes.size())) {
        activeBrushIndex = index;
        brushChange = true;
    }
}



/*
    Loads a brush tip given a PNG image.

    This image must be in the build out folder in your project (for now). 

    Uses STBimage to load the image, parse the pixels, and derive a brushtip. 

    @param path: The path to where the png is stored from build out file as current root dir. 

    @param outBrush: The brush that will have the PNG tip associated with it. 

    @return true if the brushtip was successfully loaded.
*/
bool BrushManager::loadBrushTipFromPNG(const std::string& path, BrushTool& outBrush)
{
    int width, height, channels;

    // force RGBA so we don't care about input format
    unsigned char* data = stbi_load(
        path.c_str(),
        &width,
        &height,
        &channels,
        STBI_rgb_alpha
    );

    // error msg 
    if (!data) {
        std::cerr << "Failed to load brush tip: " << path << "\n";
        return false;
    }

    outBrush.tipWidth = width;
    outBrush.tipHeight = height;
    outBrush.tipAlpha.resize(width * height);

    // convert to normalized alpha
    for (int i = 0; i < width * height; ++i) {
        unsigned char r = data[i * 4 + 0];
        unsigned char g = data[i * 4 + 1];
        unsigned char b = data[i * 4 + 2];
        unsigned char a = data[i * 4 + 3];

        // luminance * alpha
        float luminance =
            (0.2126f * r +
                0.7152f * g +
                0.0722f * b) / 255.0f;

        float alpha = (a / 255.0f) * luminance;

        outBrush.tipAlpha[i] = std::clamp(alpha, 0.0f, 1.0f);
    }

    stbi_image_free(data);
    return true;
}



/*
    Loads a brush tip given a GBR file. 

    This file must be in the build out folder in your project (for now). 

    Parse the gbr file format to derive a brushtip composed of alpha values.

    @param path: The path to where the png is stored from build out file as current root dir.

    @param outBrush: The brush that will have the GBR tip associated with it.

    @return true if the brushtip was successfully loaded.
*/
bool BrushManager::loadBrushFromGBR(const std::string& path, BrushTool& out)
{
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open file: " << path << "\n";
        return false;
    }

    // load in all of the different grb format elements
    // layout figured out from: https://developer.gimp.org/core/standards/gbr/#header
    uint32_t header_size        = read_be32(file);  // defines how large the header and name are
    uint32_t version            = read_be32(file);  // not important
    out.tipWidth                = read_be32(file);
    out.tipHeight               = read_be32(file);
    uint32_t bpp                = read_be32(file);  // how many bytes are in each pixel
    uint32_t magic              = read_be32(file);  // not important
    out.spacing                 = read_be32(file);

    // read in the brush name
    // the header_size is equal to 28 + the name length so we can use that
    // to figure out the name length and then grab it
    size_t name_length = (header_size > 28) ? header_size - 28 : 0;
    std::string name;
    if (name_length > 0) {
        name.resize(name_length);
        file.read(&name[0], name_length);
    }
    out.brushName = name;

    // reading in the pixel data
    // i do not understand this code super well, the stuff before this is all good though
    size_t num_pixels = size_t(out.tipWidth) * size_t(out.tipHeight);
    std::vector<uint8_t> pixels(num_pixels * bpp);
    file.read(reinterpret_cast<char*>(pixels.data()), pixels.size());

    // Convert to alpha values [0,1]
    out.tipAlpha.resize(num_pixels);
    for (size_t i = 0; i < num_pixels; ++i) {
        if (bpp == 1) { // if just an alpha value
            out.tipAlpha[i] = pixels[i] / 255.0f;
        } else if (bpp == 3) {  // if an RGB value
            out.tipAlpha[i] = (pixels[i*3] + pixels[i*3+1] + pixels[i*3+2]) / (3.0f * 255.0f);
        } else if (bpp == 4) {  // if an RGBA value
            out.tipAlpha[i] = pixels[i*4 + 3] / 255.0f;
        }
    }

    return true;
}



/*
    Loads the default brush tip. 

    This brush specifically is a hard coded list of pixel values. 

    @param brush: The brush to be configured as the default brush. 

    Returns nothing as it cannot fail to load. 
*/
void BrushManager::configureAsDefault(BrushTool& brush) {

	float cx = (brush.tipWidth - 1) * 0.5f; 
	float cy = (brush.tipHeight - 1) * 0.5f;
	float radius = std::min(cx, cy);
    brush.brushName = "D_Circle";

	for (int y = 0; y < brush.tipHeight; ++y) {
		
		for (int x = 0; x < brush.tipWidth; ++x) {

			float dx = x - cx; 
			float dy = y - cy; 
			float dist = std::sqrt(dx * dx + dy * dy); 

			// normalize distance 
			float t = dist / radius; 

			// soft falloff 
			float alpha = 1.0f - t; 

			brush.tipAlpha[y * brush.tipWidth + x] = std::clamp(alpha, 0.0f, 1.0f); 
		}
	}
}



/*
    Helper method for parsing the GBR brush files.

    @return groups of bits needed for file parsing. 
*/
uint32_t BrushManager::read_be32(std::ifstream& f)
{
    // reads in the next 4 bits
    unsigned char b[4];
    f.read(reinterpret_cast<char*>(b), 4);
    
    // moves them into their correct spots and combines them into a single number
    return (uint32_t(b[0]) << 24) |
           (uint32_t(b[1]) << 16) |
           (uint32_t(b[2]) << 8)  |
            uint32_t(b[3]);
}

void BrushManager::loadBrush(const std::string& path)
{
    BrushTool temp = BrushTool();

    // if the path is too short then print an error and return
    if (path.size() < 4) {
        std::cerr << "Invalid brush file path: " << path << "\n";
        return;
    }

    std::string fileType = path.substr(path.size() - 4);
    if (fileType == ".gbr") {
        if (loadBrushFromGBR(path, temp)) {
            loaded_Brushes.emplace_back(temp);
            brushChange = true;
        }
    }
    else if (fileType == ".png") {
        if (loadBrushTipFromPNG(path, temp)) {
            loaded_Brushes.emplace_back(temp);
            brushChange = true;
        }
    }
    else {
        std::cout << "Unsupported brush file type: " << fileType << "\n";
    }
    
}