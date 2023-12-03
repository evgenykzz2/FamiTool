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
    bool behind_background;
    std::vector<uint8_t> chr_export;  //Raw chr, can duplicate chr
    std::vector<uint8_t> chr_export_blink;
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
    int dx;
    int dy;

    //std::vector<int> chr_export_index;
    //std::map<std::vector<uint8_t>, int> chr_export_map;
};
