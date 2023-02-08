#pragma once
#include <vector>
#include <stdint.h>

struct WaveFormat
{
    uint16_t sample_format;
    uint16_t channel_count;
    uint16_t sample_rate;
    uint16_t bit_per_sample;
};

class PcmTrack
{
public:
	std::vector<uint8_t> m_data;
    WaveFormat m_format;
public:
    PcmTrack();
    PcmTrack(const char* file_name);
    bool Load(const char* file_name);
    bool Save(const char* file_name);
    bool SaveBin(const char* file_name);
    bool ConvertTo_16bit(PcmTrack& dest);
    bool ConvertTo_Mono(PcmTrack& dest);
    bool Resample(PcmTrack& dest, int dst_sample_rate);
    bool ConvertTo_8bit_signed(PcmTrack& dest);
    bool ConvertTo_8bit_unsigned(PcmTrack& dest);
    bool ConvertTo_7bit(PcmTrack& dest, float amplify = 1.5f);
    bool ConvertTo_7bit_Amplify(PcmTrack& dest);
};
