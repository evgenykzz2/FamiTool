#include "AviReader.h"
#include <stdexcept>
#include <iostream>
#include <vector>

uint16_t AviReader::Invert16(uint16_t val) const
{
    uint16_t inv = 0;
    inv =  (val >> 8) & 0xff;
    inv |= (val << 8) & 0xff00;

    return inv;
}

uint32_t AviReader::Invert32(uint32_t val) const
{
    uint32_t inv = 0;
    inv =  (val >> 24) & 0xff;
    inv |= (val >> 8)  & 0xff00;
    inv |= (val << 8)  & 0xff0000;
    inv |= (val << 24) & 0xff000000;

    return inv;
}

uint64_t AviReader::Invert64(uint64_t val) const
{
    uint64_t inv = 0;
    inv =   (val >> 56) & 0xffLL;
    inv |=  (val >> 40) & 0xff00LL;
    inv |=  (val >> 24) & 0xff0000LL;
    inv |=  (val >> 8)  & 0xff000000LL;
    inv |=  (val << 8)  & 0xff00000000LL;
    inv |=  (val << 24) & 0xff0000000000LL;
    inv |=  (val << 40) & 0xff000000000000LL;
    inv |=  (val << 56) & 0xff00000000000000LL;

    return inv;
}

uint16_t AviReader::Read_usint()
{
    uint16_t val;
    size_t res = fread( &val, 1, sizeof(val), m_file->m_file );
    if (res != sizeof(val))
        throw std::runtime_error("File read error");
    return val;
}

uint16_t AviReader::Read_usintI()
{
    uint16_t val;
    size_t res = fread( &val, 1, sizeof(val), m_file->m_file );
    if (res != sizeof(val))
        throw std::runtime_error("File read error");
    return Invert16(val);
}

uint32_t AviReader::Read_uint()
{
    uint32_t val;
    size_t res = fread( &val, 1, sizeof(val), m_file->m_file );
    if (res != sizeof(val))
        throw std::runtime_error("File read error");
    return val;
}

uint32_t AviReader::Read_uintI()
{
    uint32_t val;
    size_t res = fread( &val, 1, sizeof(val), m_file->m_file );
    if (res != sizeof(val))
        throw std::runtime_error("File read error");
    return Invert32(val);
}

void AviReader::Read_buffer(void* ptr, size_t size)
{
    size_t res = fread(ptr, 1, size, m_file->m_file );
    if (res != size)
        throw std::runtime_error("File read error");
}

size_t AviReader::FilePos()
{
    return ftell(m_file->m_file);
}

void AviReader::FileSeek(size_t pos)
{
    fseek(m_file->m_file, pos, SEEK_SET);
}

AviReader::AviReader(const wchar_t* file_name) :
    m_file(0), m_file_size(0), m_movi_pos(0)
{
    m_file_name = file_name;
    FILE* file = 0;
    _wfopen_s( &file, file_name, L"rb" );
    if (!file)
        throw::std::runtime_error("Can't open file");
    m_file = std::make_shared<FileObject>(file);

    fseek(m_file->m_file, 0, SEEK_END);
    m_file_size = ftell(m_file->m_file);
    fseek(m_file->m_file, 0, SEEK_SET);

    //Read Avi structure
    unsigned int header_riff = Read_uintI();
    m_riff_size = (long long)(Read_uint());
    unsigned int header_avi = Read_uintI();

    if ( header_riff != 'RIFF' ||  header_avi != 'AVI ')
        throw std::runtime_error( "Invalid AVI file" );

    uint32_t stream_index = 0;

    //Read HDRL Header
    {
        long long hdrl_pos = FilePos();
        unsigned int header_list = Read_uintI();
        unsigned int hdrl_size = Read_uint();
        unsigned int header_hdrl = Read_uintI();
        if ( header_list != 'LIST' || header_hdrl != 'hdrl' )
            throw std::runtime_error( "Invalid hdrl header" );

        //avi header
        unsigned int stream_count = 0;
        {
            long long avih_pos = FilePos();
            unsigned int header_avih = Read_uintI();
            unsigned int avih_size = Read_uint();

            unsigned int duration = Read_uint();
            unsigned int frame_size = Read_uint();

            unsigned int padding = Read_uint();    //padding_granularity (Reserved)
            unsigned int flags = Read_uint();

            unsigned int video_frame_count = Read_uint();   //TotalFrames
            unsigned int initial_frames = Read_uint();      //InitialFrames

            stream_count = Read_uint();                     //StreamCount
            unsigned int suggestion_buffer_size = Read_uint();  //Suggestion_buffer_size

            unsigned int video_width = Read_uint();
            unsigned int video_height = Read_uint();

            FileSeek(avih_pos+avih_size+8);
        }

        //Read all streams
        for ( size_t i = 0; i < stream_count; i ++ )
        {
            long long strl_pos = FilePos();
            unsigned int header_list = Read_uintI();
            unsigned int strl_size = Read_uint();
            unsigned int header_strl = Read_uintI();
            if ( header_list != 'LIST' || header_strl != 'strl' )
                throw std::runtime_error( "Invalid strl header" );

            //strh  Stream Header
            {
                long long strh_pos = FilePos();
                unsigned int header_strh = Read_uintI();
                unsigned int strh_size = Read_uint();

                if ( header_strh != 'strh' )
                    throw std::runtime_error( "Invalid strh header" );

                unsigned int stream_type = Read_uintI();
                unsigned int codec = Read_uintI();
                unsigned int flags = Read_uint();
                unsigned int reserved = Read_uint();
                unsigned int init_frames = Read_uint();

                uint32_t frame_duration = Read_uint();
                uint32_t time_scale = Read_uint();

                unsigned int frame_start = Read_uint();
                unsigned int frame_count = Read_uint();
                unsigned int buffer_size = Read_uint();
                unsigned int quality = Read_uint();
                unsigned int sample_size = Read_uint();

                //Rect
                unsigned short int rect_x = Read_usint();
                unsigned short int rect_y = Read_usint();
                unsigned short int rect_width = Read_usint();
                unsigned short int rect_height = Read_usint();
                FileSeek(strh_pos+strh_size+8);

                if (stream_type == 'vids')
                {
                    ReadVideoStream( strl_pos+strl_size+8, codec, frame_count );
                    m_video_stream->index = stream_index;
                    m_video_stream->frame_duration = frame_duration;
                    m_video_stream->time_scale = time_scale;
                    ++stream_index;
                } else if (stream_type == 'auds')
                {
                    ReadAudioStream( strl_pos+strl_size+8, codec, frame_count );
                    m_audio_stream->index = stream_index;
                    m_audio_stream->frame_duration = frame_duration;
                    m_audio_stream->time_scale = time_scale;
                    ++stream_index;
                } else
                    throw std::runtime_error( "Unknown strh header" );
            }
            FileSeek(strl_pos+strl_size+8);
        }
        FileSeek(hdrl_pos+hdrl_size+8);
    }

    ReadAviTags( (size_t)m_file_size );
}

void AviReader::ReadVideoStream( long long end_pos, unsigned int codec, unsigned int frame_count )
{
    m_video_stream = std::make_shared<VideoStream>();

    while ( FilePos() < end_pos )
    {
        long long pos = FilePos();
        unsigned int header = Read_uintI();
        unsigned int size = Read_uint();

        //strf Stream Format
        if ( header == 'strf' )
        {
            m_video_stream->size = Read_uint();        //Size
            m_video_stream->width = Read_uint();       //Width
            m_video_stream->height = Read_uint();      //Height

            m_video_stream->planes = Read_usint();     //Planes
            m_video_stream->bpp = Read_usint();        //BitPerPixel

            m_video_stream->compression = Read_uintI();    //'MJPG' 'UYVY' 'HDYC' 'V210' 'AVdn'
            unsigned int img_size = Read_uint();        //SizeImage (in bytes)
            unsigned int x_per_meter = Read_uint();     //XPelsPerMeter
            unsigned int y_per_meter = Read_uint();     //YPelsPerMeter

            unsigned int clr_used = Read_uint();        //ClrUsed: Number of colors used
            unsigned int clr_important = Read_uint();   //ClrImportant: Number of
        }

        //indx AVI2 index extension
        if ( header == 'indx' )
        {
            std::cout << "Video stream indx" << std::endl;
            /*if( !video_stream  )
                throw MediaException( ErrorFileFormat, "Can't open file (Video stream not initialized)" );

            if (ReadAviExtIndex(video_stream))
            {
                AviExIndex* ex_index = video_stream->IndexMap()->m_ext_index;
                for ( int i = 0; i < ex_index->count; i ++)
                {
                    ex_index->ex_chunk[i].start_time = (long long)(ex_index->ex_chunk[i].start_frame * video_stream->FrameDuration());
                    ex_index->ex_chunk[i].duration = (long long)(ex_index->ex_chunk[i].count * video_stream->FrameDuration());

                    //Calc each frame time
                    for ( int n = 0; n < ex_index->ex_chunk[i].count; n ++)
                        ex_index->ex_chunk[i].ex_time[n] = (unsigned)(n*video_stream->FrameDuration());
                }
                ex_index->total_duration = ex_index->total_frame_count*video_stream->FrameDuration();

                //Fix video stream duration
                video_stream->SetDuration( ex_index->total_duration );
            }*/
        }

        //JUNK
        {

        }

        FileSeek(pos+size+8);
    }
}


void AviReader::ReadAudioStream(long long end_pos, unsigned int codec, unsigned int frame_count )
{
    m_audio_stream = std::make_shared<AudioStream>();

    while ( FilePos() < end_pos )
    {
        size_t pos = FilePos();
        uint32_t header = Read_uintI();
        uint32_t size = Read_uint();

        //strf Stream Format
        if ( header == 'strf' )
        {
            unsigned int format_tag = Read_usint();     //Format Tag
            m_audio_stream->format_tag = format_tag;

            if ( format_tag == 1 || format_tag == 3 )   //WAVE_FORMAT_PCM    //WAVE_FORMAT_IEEE_FLOAT
            {
                m_audio_stream->channel_count = Read_usint();  //Channels
                m_audio_stream->sample_rate = Read_uint();     //SampleRate
                m_audio_stream->byte_per_sec = Read_uint();    //BytesPerSec

                unsigned int block_align = Read_usint();    //BlockAlign
                m_audio_stream->bit_per_sample = Read_usint(); //Bit per sample
                unsigned int cb_size = Read_usint();        //cbSize

            } else if ( format_tag == 0xFFFE )  //WAVE_FORMAT_EXTENSIBLE
            {
                m_audio_stream->channel_count = Read_usint();  //Channels
                m_audio_stream->sample_rate = Read_uint();     //SampleRate
                m_audio_stream->byte_per_sec = Read_uint();    //BytesPerSec

                unsigned int block_align = Read_usint();    //BlockAlign
                m_audio_stream->bit_per_sample = Read_usint(); //Bit per sample
                unsigned int cb_size = Read_usint();        //cbSize

                unsigned short int valid_bits_per_sample = Read_usint();;
                unsigned short int samples_per_block = Read_usint();
                unsigned short int reserved = Read_usint();
                /*AGUID  sub_format;
                Read_buffer( &sub_format, sizeof(sub_format));
                MediaStream::MediaStreamAudioCodec codec = MSA_NONE;
                if ( sub_format == GUID_AUDIO_PCM )
                {
                    if ( bit_per_sample == 16 )
                        codec = MSA_RAW_16le;
                    else if ( bit_per_sample == 24 )
                        codec = MSA_RAW_24le;
                    else if ( bit_per_sample == 32 )
                        codec = MSA_RAW_32le;
                } else if ( sub_format == GUID_AUDIO_FLOAT )
                {
                    if ( bit_per_sample == 32 )
                        codec = MSA_RAW_Fle;
                }

                if ( codec != MSA_NONE )
                {
                    audio_stream = new FileReaderStreamAviAudio( codec, sample_rate, channel_count, time_scale, frame_duration, (long long)frame_count*(long long)frame_duration );
                    m_stream_vector.push_back( audio_stream );
                } else
                {
                    throw MediaException( ErrorFileFormat, "Can't open file (Audio stream encoder not support)" );
                }*/
            }else if ( format_tag == 2 || format_tag == 17 )  // ADPCM_MS ADPCM_IMA_WAV
            {
                throw std::runtime_error("Can't open file (Audio stream encoder is ADPCM)" );
            }
        }

        if ( header == 'indx')
        {
            std::cout << "Audio stream indx" << std::endl;
            /*if( !audio_stream  )
                throw MediaException( ErrorFileFormat, "Can't open file (Audio stream not initialized)" );
            long long time = 0;
            if (ReadAviExtIndex(audio_stream))
            {
                AviExIndex* ex_index = audio_stream->IndexMap()->m_ext_index;
                long long byte_per_sample = audio_stream->ChannelCount();

                size_t frame_size = MediaAudioRawFrame::AudioCodecFrameSize( audio_stream->AudioCodec() );
                byte_per_sample *= frame_size > 0 ? frame_size : 1;

                long long total_size = 0;
                for ( int i = 0; i < ex_index->count; i ++)
                {
                    ex_index->ex_chunk[i].duration = (long long)(ex_index->ex_chunk[i].total_size)/byte_per_sample;
                    ex_index->ex_chunk[i].start_time = time;
                    total_size += ex_index->ex_chunk[i].total_size;
                    time += ex_index->ex_chunk[i].duration;

                    //Calc each frame time
                    unsigned int size = 0;
                    for ( int n = 0; n < ex_index->ex_chunk[i].count; n ++)
                    {
                        ex_index->ex_chunk[i].ex_time[n] = size/(unsigned int)byte_per_sample;
                        size += ex_index->ex_chunk[i].ex_frame[n].size;
                    }
                }
                ex_index->total_duration = total_size/byte_per_sample;
                audio_stream->SetDuration( ex_index->total_duration );
            }*/
        }

        //JUNK
        {

        }

        FileSeek(pos+size+8);
    }
}

void AviReader::ReadAviTags( size_t end_pos )
{
    //Read Rest
    while ( FilePos() < (long long)end_pos )
    {
        size_t pos = FilePos();
        uint32_t header = Read_uintI();
        uint32_t size = Read_uint();

        //movi
        if (header == 'LIST')
        {
            unsigned int header_typ = Read_uintI();

            if (header_typ == 'movi')
            {
                m_movi_pos = FilePos();
            } else if ( header_typ == 'Tdat' )
            {
                /*long long end_pos = pos+size+8;
                while ( FilePos() < end_pos )
                {
                    unsigned int header_tc = Read_uintI();
                    unsigned int size_tc = Read_uint();
                    char* data_tc = new char[ size_tc + 1 ];
                    data_tc[size_tc] = 0;
                    Read_buffer(data_tc, size_tc);
                    std::wstring text = Utf82Ws(data_tc);
                    delete [] data_tc;

                    switch ( header_tc )
                    {
                    case 'tc_O':
                        break;
                    case 'tc_A':
                        break;
                    case 'rn_O':
                        m_xmp_config.SetTapeName( text );
                        break;
                    case 'rn_A':
                        m_xmp_config.SetTapeNameAlt( text );
                        break;
                    default:
                        break;
                    }
                }*/
            }
        } else if (header == 'idx1')
        {
            ReadAvi10Index( size );
        } else if ( header == '_PMX' )
        {
        } else if ( header == 'RIFF' )
        {
            uint32_t avix = Read_uintI();
            if ( avix == 'AVIX' )
            {
                ReadAviTags( (size_t)( pos + (long long)size ) );
            }
        }

        //JUNK
        {
            //skip it
        }

        if ( size == 0 )
            break;

        FileSeek(pos+size+8);
    }
}

void AviReader::ReadAvi10Index( uint32_t size )
{
    std::vector<uint8_t> index_buffer(size);
    Read_buffer(index_buffer.data(), size);
    //int index_max = size / 16;
    bool first_tag = true;
    uint32_t pos_dif = m_movi_pos + 4;
    uint32_t pos = 0;
    while ( pos < size )
    {
        uint32_t stream_id = *((uint32_t*)(index_buffer.data() + pos));
        uint32_t flags     = *((uint32_t*)(index_buffer.data() + pos + 4));
        uint32_t data_pos  = *((uint32_t*)(index_buffer.data() + pos + 8));
        uint32_t data_size = *((uint32_t*)(index_buffer.data() + pos + 12));
        pos += 16;

        if ( first_tag )
        {
            size_t current_pos = FilePos();
            FileSeek(m_movi_pos + data_pos-4);
            //uint32_t stream_id_file_movi =
            Read_uint();

            FileSeek(data_pos);
            uint32_t stream_id_file_abs = Read_uint();

            if (stream_id == stream_id_file_abs)
                pos_dif = 8;    //Use abs

            FileSeek(current_pos);
            first_tag = false;
        }

        FrameInfo info;
        info.pos = data_pos + pos_dif;
        info.flags = flags;
        info.size = data_size;

        unsigned int stream_number = ((stream_id & 0xFF) - '0') * 10 + (((stream_id & 0xFF00) >> 8) - '0') * 1;
        if (m_video_stream && m_video_stream->index == stream_number)
            m_video_stream->frame_info.push_back(info);
        if (m_audio_stream && m_audio_stream->index == stream_number)
            m_audio_stream->frame_info.push_back(info);
    }
}

uint32_t AviReader::VideoCompression() const
{
    if (m_video_stream)
        return m_video_stream->compression;
    else
        return 0;
}

uint32_t AviReader::VideoWidth() const
{
    if (m_video_stream)
        return m_video_stream->width;
    else
        return 0;
}

uint32_t AviReader::VideoHeight() const
{
    if (m_video_stream)
    {
        if ( (uint32_t)(m_video_stream->height & 0x80000000) != 0)
            return (m_video_stream->height ^ 0xFFFFFFFF) + 1;
        else
            return m_video_stream->height;
    } else
        return 0;
}

bool AviReader::IsVideoFlipped() const
{
    return (uint32_t)(m_video_stream->height & 0x80000000) == 0;
}

uint32_t AviReader::VideoTicksPerFrame() const
{
    return m_video_stream->frame_duration;
}

uint32_t AviReader::VideoTimeScale() const
{
    return m_video_stream->time_scale;
}

size_t AviReader::VideoFrameCount() const
{
    if (m_video_stream)
        return m_video_stream->frame_info.size();
    else
        return 0;
}

bool AviReader::VideoFrameRead(size_t frame, std::vector<uint8_t> &data)
{
    if (!m_video_stream || frame >= m_video_stream->frame_info.size())
        return false;
    data.resize(m_video_stream->frame_info[frame].size);
    FileSeek(m_video_stream->frame_info[frame].pos);
    size_t res = fread(data.data(), 1, data.size(), m_file->m_file );
    return (res == data.size());
}
