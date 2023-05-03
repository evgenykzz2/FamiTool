#pragma once
#include <QString>
#include <vector>
#include <map>

enum ESpriteMode
{
    SpriteMode_8x8 = 0,
    SpriteMode_8x16
};

struct OAM
{
    int x;
    int y;
    int palette;
    int mode;
    std::vector<uint8_t> chr_export;  //Raw chr, can duplicate chr
};

struct SliceArea
{
    int64_t id;
    int x;
    int y;
    int width;
    int height;
    QString caption;
    std::vector<OAM> oam;

    //std::vector<int> chr_export_index;
    //std::map<std::vector<uint8_t>, int> chr_export_map;
};
