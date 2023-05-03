#pragma once
#include <stdint.h>

class FamiNsf
{
    struct NsfHeader
    {
        char magic[4];
        uint8_t magic2;
        uint8_t version;
        uint8_t total_songs;
        uint8_t starting_song;
        uint16_t address_load;
        uint16_t address_init;
        uint16_t address_play;
        char str_name[32];
        char str_artist[32];
        char str_copyright[32];
        uint16_t play_speed_ntsc;
        uint8_t bank_switch_init[8];
        uint16_t play_speed_pal;
        uint8_t region;
        uint8_t extra_sound_chip;
        uint32_t pgm_size;
    };
    static uint8_t NsfMemoryRead(uint16_t addr);
    const void* m_nsf_data;
public:
    FamiNsf();
    bool Load(const void* data, size_t data_size);
    bool Init(uint8_t song_number, uint8_t region); //1 for PAL, 0 for NTSC
};