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
    FrameMode_None = 0,
    FrameMode_Skip = 1,
    FrameMode_Black = 2,
    FrameMode_KeyFrame = 4
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
    IndexMethod_WuQuantizer,

    IndexMethod_EvgenyNes,
};

enum EDietherMethod
{
    DietherMethod_None = 0,
    DietherMethod_Bayer = 1,
    DietherMethod_BlueNoise = 2,
    DietherMethod_Rnd = 3,
};

struct Tile
{
    std::vector<uint8_t> bits;
    bool operator < (const Tile& other ) const
    {
        return (bits < other.bits);
    }
};


struct GopInfo
{
    //bool indexed;
    EIndexMethod index_method;
    EDietherMethod diether_method;
    int colors; //color count for indexing method
    int brightness;
    int saturation;
    int noise_power;
    std::vector<int> palette;

    GopInfo():
        //indexed(false),
        index_method(IndexMethod_EvgenyNes),
        diether_method(DietherMethod_None),
        colors(11),
        brightness(0),
        saturation(0),
        noise_power(0),
        palette(16, 0x0F)
    {}
};

struct FrameInfo
{
    EFrameMode frame_mode;
    bool force_update;

    FrameInfo() :
        frame_mode(FrameMode_None),
        force_update(true)
    {}
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
