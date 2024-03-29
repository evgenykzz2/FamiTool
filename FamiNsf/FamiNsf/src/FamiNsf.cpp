#include "FamiNsf.h"
#include "Registers.h"
#include "Memory.h"
#include "CPU_6502.h"
#include "APU.h"
#include <iostream>
#include <iomanip>
#include <cstring>

//NTSC 1789773  29780.5
//PAL  1662607  33247.5

#define DEBUG_PRINT

FamiNsf::FamiNsf() : 
    m_nsf_data(0),
    m_song_number(0xFF)
{}

bool FamiNsf::Load(const void* data, size_t data_size)
{
    if (sizeof(NsfHeader) != 0x80)
    {
        std::cout << "Compiler error, infalid header size " << sizeof(NsfHeader) << std::endl;
        return false;
    }
    if (data_size < sizeof(NsfHeader))
    {
        std::cout << "Data size too small " << data_size << std::endl;
        return false;
    }

#if defined(ESP8266)
    for (size_t n = 0; n < sizeof(NsfHeader); ++n)
        ((uint8_t*)(&m_header))[n] = pgm_read_byte((const uint8_t*)data + n);
    const NsfHeader* header = &m_header;
#elif defined(WIN32)
    const NsfHeader* header = (const NsfHeader*)data;
#endif
    if (header->magic[0] != 'N' || header->magic[1] != 'E' || header->magic[2] != 'S' || header->magic[3] != 'M' ||
        header->magic2 != 0x1A)
    {
        std::cout << "Invalid header tag" << std::endl;
        return false;
    }

    m_nsf_data = data;

#ifdef DEBUG_PRINT
    std::cout << "Version: 0x" << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)header->version << std::endl;
    std::cout << "Total songs: " << std::dec << (uint32_t)header->total_songs << std::endl;
    std::cout << "Starting song: " << std::dec << (uint32_t)header->starting_song << std::endl;
    std::cout << "Address load: 0x" << std::hex << std::setw(4) << std::setfill('0') << (uint32_t)header->address_load << std::endl;
    std::cout << "Address init: 0x" << std::hex << std::setw(4) << std::setfill('0') << (uint32_t)header->address_init << std::endl;
    std::cout << "Address play: 0x" << std::hex << std::setw(4) << std::setfill('0') << (uint32_t)header->address_play << std::endl;
    std::cout << "Name: " << header->str_name << std::endl;
    std::cout << "Artist: " << header->str_artist << std::endl;
    std::cout << "Copyright: " << header->str_copyright << std::endl;
    std::cout << "Play speed NTSC: " << std::dec << (uint32_t)header->play_speed_ntsc << std::endl;
    std::cout << "Bank Init: "
        <<  "0x" << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)header->bank_switch_init[0]
        << " 0x" << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)header->bank_switch_init[1]
        << " 0x" << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)header->bank_switch_init[2]
        << " 0x" << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)header->bank_switch_init[3]
        << " 0x" << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)header->bank_switch_init[4]
        << " 0x" << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)header->bank_switch_init[5]
        << " 0x" << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)header->bank_switch_init[6]
        << " 0x" << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)header->bank_switch_init[7]
        << std::endl;
    std::cout << "Play speed PAL: " << std::dec << (uint32_t)header->play_speed_pal << std::endl;
    std::cout << "Regin: 0x" << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)header->region;
    if (header->region & 2)
        std::cout << " dual PAL/NTSC" << std::endl;
    else if (header->region & 1)
        std::cout << " PAL" << std::endl;
    else
        std::cout << " NTSC" << std::endl;

    std::cout << "Extra sound chip: 0x" << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)header->extra_sound_chip;
    if (header->extra_sound_chip & 0x01)
        std::cout << " VRC6";
    if (header->extra_sound_chip & 0x02)
        std::cout << " VRC7";
    if (header->extra_sound_chip & 0x04)
        std::cout << " FDS";
    if (header->extra_sound_chip & 0x08)
        std::cout << " MMC5";
    if (header->extra_sound_chip & 0x10)
        std::cout << " Namco 163";
    if (header->extra_sound_chip & 0x20)
        std::cout << " Sunsoft 5B";
    if (header->extra_sound_chip & 0x40)
        std::cout << " VT02+";
    std::cout << std::endl;

    std::cout << "Program size: 0x" << std::hex << std::setw(6) << std::setfill('0') << (uint32_t)header->pgm_size << std::endl;
#endif

    return true;
}

static const void* s_nsf_memory = 0;
#if defined(ESP8266)
const FamiNsf::NsfHeader* s_nsf_header;
#endif

uint8_t FamiNsf::NsfMemoryRead(uint16_t addr)
{
#if defined(ESP8266)
    return pgm_read_byte((const uint8_t*)s_nsf_memory + 0x80 + addr - s_nsf_header->address_load);
#elif defined(WIN32)
    return ((const uint8_t*)s_nsf_memory)[0x80 + addr - ((const NsfHeader*)s_nsf_memory)->address_load];
#endif
}

bool FamiNsf::Init(uint8_t song_number, uint8_t region)
{
    std::cout << "Init" << std::endl;
#if defined(ESP8266)
    const NsfHeader* header = &m_header;
    s_nsf_header = header;
#elif defined(WIN32)
    if (m_nsf_data == 0)
        return false;
    const NsfHeader* header = (const NsfHeader*)m_nsf_data;
#endif

    if (song_number >= header->total_songs)
    {
        std::cout << "Invalid song " << (int)song_number << " total songs=" << (int)header->total_songs << std::endl;
        return false;
    }
    m_song_number = song_number;
    //TODO region check

    std::cout << "Memory_Init" << std::endl;
    Memory_Init();
    std::cout << "CPU_6502_Init" << std::endl;
    CPU_6502_Init();
    std::cout << "APU_Init" << std::endl;
    APU_Init(&s_apu);

    memset(s_nes_ram, 0, sizeof(s_nes_ram));
    memset(s_nes_sram, 0, sizeof(s_nes_sram));

    s_nsf_memory = m_nsf_data;
    //8000-9FFF ROM memory
    s_nes_memory_read_func[4] = NsfMemoryRead;
    //A000-BFFF ROM memory
    s_nes_memory_read_func[5] = NsfMemoryRead;
    //C000-DFFF ROM memory
    s_nes_memory_read_func[6] = NsfMemoryRead;
    //E000-FFFF ROM memory
    s_nes_memory_read_func[7] = NsfMemoryRead;

    for (uint16_t addr = 0x4000; addr <= 0x4013; ++addr)
        Memory_Write(addr, 0x00);

    Memory_Write(0x4015, 0x00);
    Memory_Write(0x4015, 0x0F);

    Memory_Write(0x4017, 0x40);

    s_nes_regs.A = song_number;
    s_nes_regs.X = region;

    s_nes_regs.PC = header->address_init;
    s_nes_execution_finished = 0;

    std::cout << "Init: CPU_6502_Execute" << std::endl;
    CPU_6502_Execute(29780, false);

    return true;
}

bool FamiNsf::PlayFrame()
{
#if defined(ESP8266)
    const NsfHeader* header = &m_header;
#elif defined(WIN32)
    if (m_nsf_data == 0)
        return false;
    const NsfHeader* header = (const NsfHeader*)m_nsf_data;
#endif

    if (m_song_number >= header->total_songs)
        return false;

    s_nes_regs.PC = header->address_play;
    s_nes_execution_finished = 0;
    CPU_6502_Execute(29780, true);
    return true;
}