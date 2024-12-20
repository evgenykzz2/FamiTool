#pragma once
#include <QString>
#include <QImage>
#include <vector>
#include <map>
#include <memory.h>
#include <string.h>

#define YUUREIKUN_WIDTH 16
#define YUUREIKUN_HEIGHT 11

enum ECompression
{
    Compression_None = 0,
    Compression_Rle3 = 1,
    Compression_Rle4 = 2
};

enum EChrAlign
{
    ChrAlign_None = 0,
    ChrAlign_1K = 1,
    ChrAlign_2K = 2,
    ChrAlign_4K = 4,
    ChrAlign_8K = 8
};

enum ELevelType
{
    LevelType_Horizontal = 0,
    LevelType_VerticalMoveDown = 1,
    LevelType_VerticalMoveUp = 2,
};

struct OAM
{
    int x;
    int y;
    int palette;
    int mode;
    std::vector<uint8_t> chr_export;  //Raw chr, can duplicate chr
    std::vector<uint8_t> chr_export_blink;
};

struct SliceArea
{
    int x;
    int y;
    int width;
    int height;
    QString caption;
    std::vector<OAM> oam;

    //std::vector<int> chr_export_index;
    //std::map<std::vector<uint8_t>, int> chr_export_map;
};

struct Block
{
    QString name;
    //uint8_t tile_id[4];
    uint8_t palette;
    QString overlay;
    int tileset;
    int chrbank;
    int tile_x;
    int tile_y;
};


struct ChrSet
{
    QString name;
    int chr_size;
    //QString file_name;
    std::shared_ptr<std::vector<uint8_t>> chr_data;
};

struct TileSet
{
    QString file_name;
};


QImage ChrRender(const void* chr, int tile_width, int tile_height, int tile_zoom, const uint32_t *color);
