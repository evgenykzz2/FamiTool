#include "PcmTrack.h"
#include <stdexcept>
#include <iostream>

#define WAVE_FMT_INT   1
#define WAVE_FMT_FLOAT 3

PcmTrack::PcmTrack()
{
	memset(&m_format, 0, sizeof(m_format));
}

PcmTrack::PcmTrack(const char* file_name)
{
	if (!Load(file_name))
		throw std::runtime_error("PcmTrack: Can't load audio file");
}

bool PcmTrack::Load(const char* file_name)
{
	memset(&m_format, 0, sizeof(m_format));

    FILE* file = 0;
    fopen_s(&file, file_name, "rb");
    if (file == 0)
    {
        std::cerr << "File '" << file_name << "' open error" << std::endl;
        return false;
    }
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    std::vector<uint8_t> file_data(file_size);
    fread(file_data.data(), 1, file_size, file);
    fclose(file);

    //Parse Data
    size_t pos = 0;
    uint32_t riff_rag = ((uint32_t*)(file_data.data() + pos))[0];
    pos += 4;
    uint32_t riff_size = ((uint32_t*)(file_data.data() + pos))[0];
    pos += 4;
    uint32_t wave_tag = ((uint32_t*)(file_data.data() + pos))[0];
    pos += 4;
    //file_size = riff_size + 8
    if (riff_rag != 0x46464952)
    {
        std::cerr << "File '" << file_name << "' - invalid 'RIFF' tag" << std::endl;
        return false;
    }
    if (wave_tag != 0x45564157)
    {
        std::cerr << "File '" << file_name << "' - invalid 'WAVE' tag" << std::endl;
        return false;
    }
    if (file_size < (size_t)riff_size + 8)
    {
        std::cerr << "File '" << file_name << "' - invalid file size, or RIFF size" << std::endl;
        return false;
    }
    while (pos < (size_t)riff_size + 8)
    {
        uint32_t tag = ((uint32_t*)(file_data.data() + pos))[0];
        pos += 4;
        uint32_t tag_size = ((uint32_t*)(file_data.data() + pos))[0];
        pos += 4;
        if (tag == 0x20746d66) //fmt
        {
            m_format.sample_format = ((uint16_t*)(file_data.data() + pos + 0))[0];
            m_format.channel_count = ((uint16_t*)(file_data.data() + pos + 2))[0];
            m_format.sample_rate = ((uint32_t*)(file_data.data() + pos + 4))[0];
            m_format.bit_per_sample = ((uint16_t*)(file_data.data() + pos + 14))[0];
            //std::cerr << "Format:" << std::endl;
            //std::cerr << "    Sample Format=" << std::dec << format.sample_format << std::endl;
            //std::cerr << "    Channel Count=" << std::dec << format.channel_count << std::endl;
            //std::cerr << "    Sample Rate =" << std::dec << format.sample_rate << std::endl;
            //std::cerr << "    BitPerSample=" << std::dec << format.bit_per_sample << std::endl;
        }
        else if (tag == 0x61746164) //data
        {
            if (pos + tag_size > file_size)
            {
                return false;
                std::cerr << "File '" << file_name << "' - invalid data size" << std::endl;
            }
            //std::cerr << "Wave DataSize=" << std::dec << tag_size << std::endl;
            m_data.resize(tag_size);
            memcpy(m_data.data(), file_data.data() + pos, m_data.size());
        }
        //std::cerr << "Tag=" << std::hex << tag << " tag_size=" << std::hex << tag_size  << std::endl;
        pos += tag_size;
    }
    return true;
}

bool PcmTrack::Save(const char* file_name)
{
    size_t file_size;
    file_size = 4 + 4 + 4   //Riff size WAVE
        + 4 + 4 + 16  //fmt size FORMAT
        + 4 + 4 + m_data.size();

    std::vector<uint8_t> file_data(file_size);
    size_t pos = 0;
    ((uint32_t*)(file_data.data() + pos))[0] = 0x46464952;
    pos += 4;
    ((uint32_t*)(file_data.data() + pos))[0] = (uint32_t)(file_size - 8);
    pos += 4;
    ((uint32_t*)(file_data.data() + pos))[0] = 0x45564157;
    pos += 4;

    //Format
    ((uint32_t*)(file_data.data() + pos))[0] = 0x20746d66;
    pos += 4;
    ((uint32_t*)(file_data.data() + pos))[0] = 16;
    pos += 4;

    ((uint16_t*)(file_data.data() + pos + 0))[0] = m_format.sample_format;
    ((uint16_t*)(file_data.data() + pos + 2))[0] = m_format.channel_count;
    ((uint32_t*)(file_data.data() + pos + 4))[0] = m_format.sample_rate;
    uint32_t byte_rate = m_format.sample_rate * m_format.channel_count * m_format.bit_per_sample / 8;
    ((uint32_t*)(file_data.data() + pos + 8))[0] = byte_rate;
    uint16_t block_align = m_format.channel_count * m_format.bit_per_sample / 8;
    ((uint16_t*)(file_data.data() + pos + 12))[0] = block_align;
    ((uint16_t*)(file_data.data() + pos + 14))[0] = m_format.bit_per_sample;
    pos += 16;

    //data
    ((uint32_t*)(file_data.data() + pos))[0] = 0x61746164;
    pos += 4;
    ((uint32_t*)(file_data.data() + pos))[0] = (uint32_t)m_data.size();
    pos += 4;
    memcpy(file_data.data() + pos, m_data.data(), m_data.size());

    FILE* file = 0;
    fopen_s(&file, file_name, "wb");
    if (file == 0)
    {
        std::cerr << "File '" << file_name << "' open error" << std::endl;
        return false;
    }
    size_t wrote = fwrite(file_data.data(), 1, file_size, file);
    if (wrote != file_size)
    {
        fclose(file);
        std::cerr << "File '" << file_name << "' write error" << std::endl;
        return false;
    }
    fclose(file);
    return true;
}

bool PcmTrack::SaveBin(const char* file_name)
{
    FILE* file = 0;
    fopen_s(&file, file_name, "wb");
    if (file == 0)
    {
        std::cerr << "File '" << file_name << "' open error" << std::endl;
        return false;
    }
    size_t wrote = fwrite(m_data.data(), 1, m_data.size(), file);
    if (wrote != m_data.size())
    {
        fclose(file);
        std::cerr << "File '" << file_name << "' write error" << std::endl;
        return false;
    }
    fclose(file);
    return true;
}

static void AudioConvert8to16(const void* src, void* dst, size_t samples_count)
{
    for (size_t i = 0; i < samples_count; ++i)
    {
        int32_t sample = ((uint16_t)(((const uint8_t*)src)[i]) << 8) | (((const uint8_t*)src)[i]);
        ((int16_t*)dst)[i] = (int16_t)(sample - 32768);
    }
}

static void AudioConvert24to16(const void* src, void* dst, size_t samples_count)
{
    for (size_t i = 0; i < samples_count; ++i)
    {
        int32_t sample = ((const uint8_t*)src)[i * 3 + 0]
            | (((const uint8_t*)src)[i * 3 + 1] << 8)
            | (((const uint8_t*)src)[i * 3 + 2] << 16);
        ((int16_t*)dst)[i] = sample >> 8;
    }
}

static void AudioConvert32to16(const void* src, void* dst, size_t samples_count)
{
    for (size_t i = 0; i < samples_count; ++i)
    {
        int32_t sample = ((const int32_t*)src)[i];
        ((int16_t*)dst)[i] = sample >> 16;
    }
}

static void AudioConvert32fto16(const void* src, void* dst, size_t samples_count)
{
    for (size_t i = 0; i < samples_count; ++i)
    {
        float sample = ((const float*)src)[i];
        int32_t value = (int)(sample * 32767.0f);
        if (value > INT16_MAX)
            value = INT16_MAX;
        if (value < INT16_MIN)
            value = INT16_MIN;
        ((int16_t*)dst)[i] = value;
    }
}

static void AudioConvert64fto16(const void* src, void* dst, size_t samples_count)
{
    for (size_t i = 0; i < samples_count; ++i)
    {
        double sample = ((const double*)src)[i];
        int32_t value = (int)(sample * 32767.0f);
        if (value > INT16_MAX)
            value = INT16_MAX;
        if (value < INT16_MIN)
            value = INT16_MIN;
        ((int16_t*)dst)[i] = value;
    }
}

bool PcmTrack::ConvertTo_16bit(PcmTrack& dest)
{
    if (m_format.sample_format == WAVE_FMT_INT && m_format.bit_per_sample == 8)
    {
        size_t samples_count = m_data.size() / (m_format.bit_per_sample / 8);
        dest.m_data.resize(samples_count * 2);
        AudioConvert8to16(m_data.data(), dest.m_data.data(), samples_count);
    }
    else if (m_format.sample_format == WAVE_FMT_INT && m_format.bit_per_sample == 16)
    {
        dest.m_data.resize(m_data.size());
        memcpy(dest.m_data.data(), m_data.data(), m_data.size());
    }
    else if (m_format.sample_format == WAVE_FMT_INT && m_format.bit_per_sample == 24)
    {
        size_t samples_count = m_data.size() / (m_format.bit_per_sample / 8);
        dest.m_data.resize(samples_count * 2);
        AudioConvert24to16(m_data.data(), dest.m_data.data(), samples_count);
    }
    else if (m_format.sample_format == WAVE_FMT_INT && m_format.bit_per_sample == 32)
    {
        size_t samples_count = m_data.size() / (m_format.bit_per_sample / 8);
        dest.m_data.resize(samples_count * 2);
        AudioConvert32to16(m_data.data(), dest.m_data.data(), samples_count);
    }
    else if (m_format.sample_format == WAVE_FMT_FLOAT && m_format.bit_per_sample == 32)
    {
        size_t samples_count = m_data.size() / (m_format.bit_per_sample / 8);
        dest.m_data.resize(samples_count * 2);
        AudioConvert32fto16(m_data.data(), dest.m_data.data(), samples_count);
    }
    else if (m_format.sample_format == WAVE_FMT_FLOAT && m_format.bit_per_sample == 64)
    {
        size_t samples_count = m_data.size() / (m_format.bit_per_sample / 8);
        dest.m_data.resize(samples_count * 2);
        AudioConvert64fto16(m_data.data(), dest.m_data.data(), samples_count);
    }
    else
    {
        std::cerr << "Can't convert to 16bit" << std::endl;
        return false;
    }
    dest.m_format.bit_per_sample = 16;
    dest.m_format.sample_format = WAVE_FMT_INT;
    dest.m_format.sample_rate = m_format.sample_rate;
    dest.m_format.channel_count = m_format.channel_count;
    return true;
}

bool PcmTrack::ConvertTo_Mono(PcmTrack& dest)
{
    if (m_format.channel_count == 1)
    {
        size_t frame_count = m_data.size() / ((size_t)m_format.channel_count * ((size_t)m_format.bit_per_sample / 8));
        dest.m_data.resize(frame_count * ((size_t)m_format.bit_per_sample / 8));
        memcpy(dest.m_data.data(), m_data.data(), dest.m_data.size());
        dest.m_format.bit_per_sample = m_format.bit_per_sample;
        dest.m_format.sample_format = m_format.sample_format;
        dest.m_format.sample_rate = m_format.sample_rate;
        dest.m_format.channel_count = 1;
        return true;
    } else if (m_format.sample_format == WAVE_FMT_INT && m_format.bit_per_sample == 16 && m_format.channel_count==2)
    {
        size_t frame_count = m_data.size() / ((size_t)m_format.channel_count * ((size_t)m_format.bit_per_sample / 8));
        dest.m_data.resize(frame_count * ((size_t)m_format.bit_per_sample / 8));
        for (size_t i = 0; i < frame_count; ++i)
        {
            int32_t left = ((const int16_t*)m_data.data())[i * 2 + 0];
            int32_t right = ((const int16_t*)m_data.data())[i * 2 + 1];
            int16_t sample = (left + right) >> 1;
            ((int16_t*)dest.m_data.data())[i] = sample;
        }
        dest.m_format.bit_per_sample = m_format.bit_per_sample;
        dest.m_format.sample_format = m_format.sample_format;
        dest.m_format.sample_rate = m_format.sample_rate;
        dest.m_format.channel_count = 1;
        return true;
    } else if (m_format.sample_format == WAVE_FMT_INT && m_format.bit_per_sample == 32 && m_format.channel_count == 2)
    {
        size_t frame_count = m_data.size() / ((size_t)m_format.channel_count * ((size_t)m_format.bit_per_sample / 8));
        dest.m_data.resize(frame_count * ((size_t)m_format.bit_per_sample / 8));
        for (size_t i = 0; i < frame_count; ++i)
        {
            int32_t left = ((const int32_t*)m_data.data())[i * 2 + 0];
            int32_t right = ((const int32_t*)m_data.data())[i * 2 + 1];
            int32_t sample = (left + right) >> 1;
            ((int32_t*)dest.m_data.data())[i] = sample;
        }
        dest.m_format.bit_per_sample = m_format.bit_per_sample;
        dest.m_format.sample_format = m_format.sample_format;
        dest.m_format.sample_rate = m_format.sample_rate;
        dest.m_format.channel_count = 1;
        return true;
    }
    else
    {
        std::cerr << "Can't convert to mono" << std::endl;
        return false;
    }
}

bool PcmTrack::Resample(PcmTrack& dest, int dst_sample_rate)
{
    size_t frame_count_src = m_data.size() / ((size_t)m_format.channel_count * m_format.bit_per_sample / 8);
    size_t frame_count_dst = frame_count_src * (size_t)dst_sample_rate / (size_t)m_format.sample_rate;

    if (m_format.sample_format == WAVE_FMT_INT && m_format.bit_per_sample == 16)
    {
        dest.m_format.bit_per_sample = m_format.bit_per_sample;
        dest.m_format.sample_format = m_format.sample_format;
        dest.m_format.sample_rate = dst_sample_rate;
        dest.m_format.channel_count = m_format.channel_count;
        dest.m_data.resize(frame_count_dst * m_format.channel_count * (m_format.bit_per_sample / 8));
        const int16_t* src_ptr = (const int16_t*)m_data.data();
        int16_t* dst_ptr = (int16_t*)dest.m_data.data();
        for (size_t i = 0; i < frame_count_dst; i++)
        {
            for (size_t ch = 0; ch < m_format.channel_count; ++ch)
            {
                int32_t samples_summ = 0;
                for (int n = 0; n < 64; ++n)
                {
                    float dst_pos = (float)i + ((float)n / 64.0f);
                    float src_pos = dst_pos * (float)m_format.sample_rate / (float)dest.m_format.sample_rate;
                    size_t pos = (size_t)src_pos;
                    if (pos < 0)
                        pos = 0;
                    if (pos > frame_count_src)
                        pos = frame_count_src;
                    samples_summ += src_ptr[pos*((size_t)m_format.channel_count * m_format.bit_per_sample / 16) + ch];
                }
                samples_summ /= 64;
                if (samples_summ > INT16_MAX)
                    samples_summ = INT16_MAX;
                if (samples_summ < INT16_MIN)
                    samples_summ = INT16_MIN;
                dst_ptr[i* ((size_t)dest.m_format.channel_count * dest.m_format.bit_per_sample / 16) + ch] = samples_summ;
            }
        }
        return true;
    }
    else
    {
        std::cerr << "Can't resample" << std::endl;
        return false;
    }
}

bool PcmTrack::ConvertTo_8bit_signed(PcmTrack& dest)
{
    size_t frames_count = m_data.size() / ((size_t)m_format.bit_per_sample / 8);
    if (m_format.sample_format == WAVE_FMT_INT && m_format.bit_per_sample == 16)
    {
        dest.m_data.resize(frames_count);
        for (size_t i = 0; i < frames_count; ++i)
            ((int8_t*)dest.m_data.data())[i] = ((const int16_t*)m_data.data())[i] >> 8;
        dest.m_format.bit_per_sample = 8;
        dest.m_format.sample_format = WAVE_FMT_INT;
        dest.m_format.sample_rate = m_format.sample_rate;
        dest.m_format.channel_count = m_format.channel_count;
        return true;
    }
    else
        return false;
}

bool PcmTrack::ConvertTo_8bit_unsigned(PcmTrack& dest)
{
    size_t frames_count = m_data.size() / ((size_t)m_format.bit_per_sample / 8);
    if (m_format.sample_format == WAVE_FMT_INT && m_format.bit_per_sample == 16)
    {
        dest.m_data.resize(frames_count);
        for (size_t i = 0; i < frames_count; ++i)
            ((uint8_t*)dest.m_data.data())[i] = (((const int16_t*)m_data.data())[i] >> 8) + 128;
        dest.m_format.bit_per_sample = 8;
        dest.m_format.sample_format = WAVE_FMT_INT;
        dest.m_format.sample_rate = m_format.sample_rate;
        dest.m_format.channel_count = m_format.channel_count;
        return true;
    }
    else
        return false;
}

bool PcmTrack::ConvertTo_7bit(PcmTrack& dest, float amplify)
{
    size_t frames_count = m_data.size() / ((size_t)m_format.bit_per_sample / 8);
    if (m_format.sample_format == WAVE_FMT_INT && m_format.bit_per_sample == 16)
    {
        dest.m_data.resize(frames_count);
        dest.m_format.bit_per_sample = 8;
        dest.m_format.sample_format = WAVE_FMT_INT;
        dest.m_format.sample_rate = m_format.sample_rate;
        dest.m_format.channel_count = m_format.channel_count;


        #if 0
        for (size_t i = 0; i < frames_count; ++i)
        {
            int sample = ((const int16_t*)m_data.data())[i];
            int sample_7bit = ((sample - INT16_MIN) + 256)  / 512;
            //error = sample - ((sample_7bit * 512) + INT16_MIN);
            if (sample_7bit < 0)
                sample_7bit = 0;
            if (sample_7bit > 127)
                sample_7bit = 127;
            ((uint8_t*)dest.m_data.data())[i] = sample_7bit;
        }
        #else
        int minimum = INT32_MAX;
        int maximum = INT32_MIN;
        for (size_t i = 0; i < frames_count; ++i)
        {
            int sample = ((const int16_t*)m_data.data())[i];
            if (sample > maximum)
                maximum = sample;
            if (sample < minimum)
                minimum = sample;
        }

        for (size_t i = 0; i < frames_count; ++i)
        {
            int sample = ((const int16_t*)m_data.data())[i];
            float fsample = (float(sample) * amplify - (float)minimum) / float(maximum - minimum);
            int sample_7bit = (int)floor(127.0f * fsample);
            if (sample_7bit < 0)
                sample_7bit = 0;
            if (sample_7bit > 127)
                sample_7bit = 127;
            ((uint8_t*)dest.m_data.data())[i] = sample_7bit;
        }
        #endif
        
        return true;
    }
    else
        return false;
}

bool PcmTrack::ConvertTo_7bit_Amplify(PcmTrack& dest)
{
    size_t frames_count = m_data.size() / ((size_t)m_format.bit_per_sample / 8);
    if (m_format.sample_format == WAVE_FMT_INT && m_format.bit_per_sample == 16)
    {
        dest.m_data.resize(frames_count);
        dest.m_format.bit_per_sample = 8;
        dest.m_format.sample_format = WAVE_FMT_INT;
        dest.m_format.sample_rate = m_format.sample_rate;
        dest.m_format.channel_count = m_format.channel_count;

        int part_size = 4096;
        std::vector<int> volume_min;
        std::vector<int> volume_max;
        {
            int part_prev = -1;
            int minimum = INT32_MAX;
            int maximum = INT32_MIN;
            for (size_t i = 0; i < frames_count; ++i)
            {
                int part = i / part_size;
                if (part != part_prev)
                {
                    if (part_prev != -1)
                    {
                        volume_min.push_back(minimum);
                        volume_max.push_back(maximum);
                        minimum = INT32_MAX;
                        maximum = INT32_MIN;
                    }
                    part_prev = part;
                }
                int sample = ((const int16_t*)m_data.data())[i];
                if (sample > maximum)
                    maximum = sample;
                if (sample < minimum)
                    minimum = sample;
            }
            volume_min.push_back(minimum);
            volume_max.push_back(maximum);
        }

        for (size_t i = 0; i < frames_count; ++i)
        {
            int64_t sum_min = 0;
            int64_t sum_max = 0;
            int sum_count = 0;
            for (int d = -part_size/2; d <= part_size/2; ++d)
            {
                if (i + d < 0 || i + d >= frames_count)
                    continue;
                int part = (i+d) / part_size;
                sum_count++;
                sum_min += volume_min[part];
                sum_max += volume_max[part];
            }
            int minimum = sum_min / sum_count;
            int maximum = sum_max / sum_count;
            int sample = ((const int16_t*)m_data.data())[i];
            float fsample = (float(sample) - (float)minimum) / float(maximum - minimum);
            int sample_7bit = (int)floor(127.0f * fsample);
            if (sample_7bit < 0)
                sample_7bit = 0;
            if (sample_7bit > 127)
                sample_7bit = 127;
            ((uint8_t*)dest.m_data.data())[i] = sample_7bit;
        }

        return true;
    }
    else
        return false;
}