
#pragma once

#include <string>

#include "BrushTool.h"

class BrushManager 
{
    public:
        // init function that loads in all default brushes
        void init();

        // active brush stuff
        const BrushTool& getActiveBrush();
        void setActiveBrush(int index);
        bool brushChange = false;

        // brush loader methods
        void loadBrush(const std::string& path);
        bool loadBrushTipFromPNG(const std::string& path, BrushTool& outBrush);
        bool loadBrushFromGBR(const std::string& path, BrushTool& outBrush);

        // various getter methods
        const std::vector<BrushTool>& getLoadedBrushes();

        // function for generating and grabbing the brush dab
        // want brush size to be a diameter measurement as opposed to a multiplicativie relationship 
        const std::vector<float> generateBrushDab(int brushSize);

    private:
        // list of all loaded brushes
        std::vector<BrushTool> loaded_Brushes;

        // list of all default brush paths to load on init
        const std::vector<std::string> defaultBrushPaths = {"circle.gbr", "cross.gbr", "confetti.gbr"};

        // index of the active brush in loaded_Brushes
        int activeBrushIndex = 0;

        // default functions
        void configureAsDefault(BrushTool& brush);

        // helper functions
        uint32_t read_be32(std::ifstream& f);
};