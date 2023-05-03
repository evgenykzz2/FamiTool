#pragma once
#include <QString>
#include <QImage>
#include <vector>
#include <map>
#include <QDateTime>

enum EViewMode
{
    ViewMode_Original = 0,
    ViewMode_Crop,
    ViewMode_Indexed,
    ViewMode_NesPalette,
    ViewMode_TileConverted,
    ViewMode_TileOptimized
};

enum EFrameMode
{
    FrameMode_Skip = 0,
    FrameMode_Black,
    FrameMode_KeyFrame,
    FrameMode_Inter,
};

enum EIndexMethod
{
    IndexMethod_DivQuantizer = 0,
    IndexMethod_Dl3Quantizer,
    IndexMethod_EdgeAwareSQuantizer,
    IndexMethod_MedianCut,
    IndexMethod_MoDEQuantizer,
    IndexMethod_NeuQuantizer,
    IndexMethod_PnnLABQuantizer,
    IndexMethod_PnnQuantizer,
    IndexMethod_SpatialQuantizer,
    IndexMethod_WuQuantizer
};

struct Tile
{
    std::vector<uint8_t> bits;
    bool operator < (const Tile& other ) const
    {
        return (bits < other.bits);
    }
};

struct FrameImageCompilation
{
    //QDateTime last_use_time;
    //QImage movie;
    //QImage crop;
    QImage indexed;
    QImage palette_convert;
    QImage tile_convert;
    QImage final;
};

struct FrameInfo
{
    EFrameMode frame_mode;
    bool indexed;
    EIndexMethod index_method;
    bool diether;
    int colors; //color count for indexing method
    std::vector<int> palette;
    std::map<uint32_t, int> color_map;
    bool force_update;

    FrameInfo() :
        frame_mode(FrameMode_Skip),
        indexed(false),
        index_method(IndexMethod_DivQuantizer),
        diether(false),
        colors(4),
        force_update(true)
    {
        palette.resize(16, 0x0F);
    }
};

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
    int x;
    int y;
    int width;
    int height;
    QString caption;
    std::vector<OAM> oam;

    //std::vector<int> chr_export_index;
    //std::map<std::vector<uint8_t>, int> chr_export_map;
};
