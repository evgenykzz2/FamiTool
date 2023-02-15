#pragma once
#include <string>
#include <stdint.h>
#include <memory>
#include <vector>

class AviReader
{
    struct FrameInfo
    {
        uint32_t pos;
        uint32_t flags;
        uint32_t size;
    };

    struct VideoStream
    {
        uint32_t index;
        uint32_t frame_duration;
        uint32_t time_scale;

        uint32_t size;
        uint32_t width;
        uint32_t height;
        uint32_t planes;
        uint32_t bpp;
        uint32_t compression;
        std::vector<FrameInfo> frame_info;
    };

    struct AudioStream
    {
        uint32_t index;
        uint32_t frame_duration;
        uint32_t time_scale;

        uint32_t format_tag;
        uint32_t channel_count;
        uint32_t sample_rate;
        uint32_t byte_per_sec;
        uint32_t bit_per_sample;
        std::vector<FrameInfo> frame_info;
    };

    struct FileObject
    {
        FILE* m_file;
        FileObject(FILE* file) : m_file(file){}
        ~FileObject()
        {
            fclose(m_file);
        }
    };

    std::wstring m_file_name;
    //FILE* m_file;
    std::shared_ptr<FileObject> m_file;
    size_t m_file_size;
    size_t m_riff_size;
    size_t m_movi_pos;

    std::shared_ptr<VideoStream> m_video_stream;
    std::shared_ptr<AudioStream> m_audio_stream;

    uint16_t Invert16(uint16_t val) const;
    uint32_t Invert32(uint32_t val) const;
    uint64_t Invert64(uint64_t val) const;

    uint16_t Read_usint();
    uint16_t Read_usintI();
    uint32_t Read_uint();
    uint32_t Read_uintI();
    void Read_buffer(void* ptr, size_t size);

    size_t FilePos();
    void FileSeek(size_t pos);

    void ReadVideoStream( long long end_pos, unsigned int codec, unsigned int frame_count );
    void ReadAudioStream( long long end_pos, unsigned int codec, unsigned int frame_count );
    void ReadAviTags( size_t end_pos );
    void ReadAvi10Index( uint32_t size );
public:
    AviReader(const wchar_t* file_name);
    uint32_t VideoCompression() const;
    uint32_t VideoWidth() const;
    uint32_t VideoHeight() const;
    size_t VideoFrameCount() const;
    uint32_t VideoTicksPerFrame() const;
    uint32_t VideoTimeScale() const;
    bool VideoFrameRead(size_t frame, std::vector<uint8_t> &data);
};
