#include "APU.h"
#include <cstring>
#define AUDIO_SAMPLE_RATE 48000

#if defined(ESP8266)
#include <I2S.h>
#endif

#ifndef WIN32
//#include <Arduino.h>
//#include <driver/dac.h>
//#include "driver/i2s.h"

//static const i2s_port_t i2s_num = I2S_NUM_0; // i2s port number
#else
#include <stdio.h>
#endif

APU s_apu;

class AudioChannel
{
public:
    uint8_t  length_counter_halt;
    uint16_t length_counter;
    bool     need_update;
    uint16_t output_sample;

public:
    AudioChannel() : length_counter_halt(0), length_counter(0), need_update(true), output_sample(0)
    {}

    virtual uint16_t ApuTick(uint16_t ticks) = 0;
    virtual void ApuFrameTick()
    {
        if (length_counter_halt == 0)
        {
            if (length_counter > 0)
                length_counter--;
        }
    }
};

#define SVOL_MIN 0
#define SVOL_MAX 8192
static const uint16_t g_pattern[32] =
{
  SVOL_MIN, SVOL_MAX, SVOL_MIN, SVOL_MIN, SVOL_MIN, SVOL_MIN, SVOL_MIN, SVOL_MIN, //12.5
  SVOL_MIN, SVOL_MAX, SVOL_MAX, SVOL_MIN, SVOL_MIN, SVOL_MIN, SVOL_MIN, SVOL_MIN, //25
  SVOL_MIN, SVOL_MAX, SVOL_MAX, SVOL_MAX, SVOL_MAX, SVOL_MIN, SVOL_MIN, SVOL_MIN, //50
  SVOL_MAX, SVOL_MIN, SVOL_MIN, SVOL_MAX, SVOL_MAX, SVOL_MAX, SVOL_MAX, SVOL_MAX  //75
};

static const uint16_t g_noise_timer_ntsc[16] = { 4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068 };  //NTSC
//static const uint16_t g_noise_timer_pal[16] = { 4, 7, 14, 30, 60, 88, 118, 148, 188, 236, 354, 472, 708, 944, 1890, 3778 };    //PAL


static const uint8_t g_lengthtable[0x20]=
{
  10,254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14,
  12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};

#define TVOL 512
#define TMID 0
static const uint16_t g_triangle[0x20]=
{
  0 *TVOL-TMID,  1*TVOL-TMID,  2*TVOL-TMID,  3*TVOL-TMID,  4*TVOL-TMID,  5*TVOL-TMID,  6*TVOL-TMID,  7*TVOL-TMID,  8*TVOL-TMID,  9*TVOL-TMID, 10*TVOL-TMID, 11*TVOL-TMID, 12*TVOL-TMID, 13*TVOL-TMID, 14*TVOL-TMID, 15*TVOL-TMID, 
  15*TVOL-TMID, 14*TVOL-TMID, 13*TVOL-TMID, 12*TVOL-TMID, 11*TVOL-TMID, 10*TVOL-TMID,  9*TVOL-TMID,  8*TVOL-TMID,  7*TVOL-TMID,  6*TVOL-TMID,  5*TVOL-TMID,  4*TVOL-TMID,  3*TVOL-TMID,  2*TVOL-TMID,  1*TVOL-TMID,  0*TVOL-TMID
};

class Pulse : public AudioChannel
{
public:
    uint8_t duty_cycle;
    uint8_t constant_volume;
    uint8_t volume;

    uint8_t sweep_enable;
    uint8_t sweep_divider_period;
    uint8_t sweep_negate;
    uint8_t sweep_shift_count;

    uint16_t timer; //setup timer

    uint8_t duty_clock;
    uint16_t timer_clock;
    uint16_t output_smooth;

public:
    Pulse()
    {
        duty_cycle = 0;
        length_counter_halt = 0;
        constant_volume = 0;
        volume = 0;

        sweep_enable = 0;
        sweep_divider_period = 0;
        sweep_negate = 0;
        sweep_shift_count = 0;

        timer = 0;
        duty_clock = 0;
        timer_clock = 0;
        output_smooth = 0;
    }

    uint16_t ApuTick(uint16_t ticks) override
    {
#if 1
        if (timer < 8 || timer > 0x7ff || length_counter == 0
            || (constant_volume && volume == 0))
        {
            output_sample = 0;
            return 0;
        }

        uint32_t output_summ = 0;
        for (uint16_t tick = 0; tick < ticks; ++tick)
        {
            if (timer_clock == 0)
            {
                duty_clock = (duty_clock + 1) & 7;
                timer_clock = timer;
                {
                    output_sample = g_pattern[duty_cycle * 8 + duty_clock];
                    if (constant_volume)
                        output_sample = (output_sample * volume) >> 4;
                    else
                    {
                        //envelope
                        output_sample = 0;//sample;
                    }
                    if (sweep_enable)
                        output_sample = 0;//sample;
                }
            }
            else
            {
                timer_clock--;
                output_summ += output_sample;
            }
        }
        return output_summ == 0 ? 0 : output_summ / ticks;
#elif 0
        timer_clock += ticks;
        while (timer_clock >= timer + 1)
        {
            timer_clock -= timer + 1;
            duty_clock = (duty_clock + 1) & 7;
            need_update = true;
        }

        if (need_update)
        {
            if (timer < 8 || timer > 0x7ff || length_counter == 0)
            {
                output_sample = 0;
            }
            else
            {
                output_sample = g_pattern[duty_cycle * 8 + duty_clock];

                if (constant_volume)
                    output_sample = (output_sample * volume) >> 4;
                else
                {
                    //envelope
                    output_sample = 0;//sample;
                }

                if (sweep_enable)
                    output_sample = 0;//sample;
            }
        }
        return output_sample;
#else
        uint32_t output_summ = 0;
        uint16_t output_cnt = 0;
        timer_clock += ticks;
        while (timer_clock >= timer + 1)
        {
            output_summ += output_sample * (timer + 1);
            timer_clock -= timer + 1;
            duty_clock = (duty_clock + 1) & 7;
            if (timer < 8 || timer > 0x7ff || length_counter == 0)
            {
                output_sample = 0;
            }
            else
            {
                output_sample = g_pattern[duty_cycle * 8 + duty_clock];

                if (constant_volume)
                    output_sample = (output_sample * volume) >> 4;
                else
                {
                    //envelope
                    output_sample = 0;//sample;
                }

                if (sweep_enable)
                    output_sample = 0;//sample;
            }
        }
        return output_summ / ticks;
#endif
    }
};

class Triangle : public AudioChannel
{
public:
    uint8_t linear_counter_load;
    uint8_t linear_counter;
    uint8_t counter_reload_flag;

    uint16_t timer;
    uint16_t timer_clock;


    uint8_t  sampler;
public:
    Triangle()
    {
        linear_counter_load = 0;
        linear_counter = 0;
        counter_reload_flag = 0;

        timer = 0;
        timer_clock = 0;
        sampler = 0;
    }

    virtual void ApuFrameTick() override
    {
        if (counter_reload_flag)
            linear_counter = linear_counter_load;
        if (length_counter_halt == 0)
        {
            if (length_counter > 0)
                length_counter--;
            if (linear_counter > 0)
                linear_counter--;
            counter_reload_flag = 0;
        }
    }

    uint16_t ApuTick(uint16_t ticks) override
    {
        while (ticks > 0 && timer > 0)
        {
            if (ticks >= timer_clock)
            {
                ticks -= timer_clock;
                sampler = (sampler + 1) & 31;
                need_update = true;
                timer_clock = timer;
            }
            else
            {
                timer_clock -= ticks;
                break;
            }
        }

        if (length_counter == 0 || linear_counter == 0)
            return 0;
        else
        {
            if (timer < 2)
                return 0;
            return g_triangle[sampler];
        }
    }
};

class Noise : public AudioChannel
{
public:
    uint8_t length_counter_halt;
    uint8_t constant_volume;
    uint8_t volume;

    uint8_t mode;
    uint8_t period;

    uint16_t shift_register;
    uint32_t timer_tick;
public:
    Noise()
    {
        length_counter_halt = 0;
        constant_volume = 0;
        volume = 0;

        mode = 0;
        period = 0;
        shift_register = 1;
        timer_tick = 0;
    }

    uint16_t ApuTick(uint16_t ticks) override
    {
        if (timer_tick == 0)
        {
            timer_tick = g_noise_timer_ntsc[period] / 32;
            if (mode)
            {
                uint16_t feedback = ((shift_register >> 8) & 1) ^ ((shift_register >> 14) & 1);
                shift_register = (shift_register << 1) + feedback;
                shift_register &= 0x7fff;
                need_update = true;
            }
            else
            {
                uint16_t feedback = ((shift_register >> 13) & 1) ^ ((shift_register >> 14) & 1);
                shift_register = (shift_register << 1) + feedback;
                shift_register &= 0x7fff;
                need_update = true;
            }
        }
        else
            timer_tick -= 1;

        if (need_update)
        {
            output_sample = (uint16_t)((shift_register >> 0xe) & 1) << 13;  //*1024*8
            if (length_counter == 0)
                output_sample = 0;
            if (constant_volume)
                output_sample = (output_sample * volume) >> 4;
            else
            {
                //envelope
                //output_sample = 0;//output_sample;
            }
        }
        return output_sample;
    }
};

//Pulse       s_pulse[2];
//Triangle    s_triangle;
//Noise       s_noise;
//uint8_t     s_channel_enable;

static uint16_t ApuDoPulse(APU* apu, uint16_t ticks, uint8_t channel)
{
    if (apu->pulse_timer[channel] < 8 || apu->pulse_timer[channel] > 0x7ff || apu->pulse_length_counter[channel] == 0
        || (apu->pulse_constant_volume[channel] && apu->pulse_volume[channel] == 0))
    {
        apu->pulse_output_sample[channel] = 0;
        return 0;
    }

    uint32_t output_summ = 0;
    for (uint16_t tick = 0; tick < ticks; ++tick)
    {
        if (apu->pulse_timer_clock[channel] == 0)
        {
            apu->pulse_duty_clock[channel] = (apu->pulse_duty_clock[channel] + 1) & 7;
            apu->pulse_timer_clock[channel] = apu->pulse_timer[channel];
            if (apu->pulse_constant_volume[channel])
            {
                apu->pulse_output_sample[channel] = (g_pattern[apu->pulse_duty_cycle[channel] * 8 + apu->pulse_duty_clock[channel]] * apu->pulse_volume[channel]) >> 4;
            } else
            {
                //envelope
                apu->pulse_output_sample[channel] = (g_pattern[apu->pulse_duty_cycle[channel] * 8 + apu->pulse_duty_clock[channel]] * apu->pulse_envelope_counter[channel]) >> 4;
            }
        }
        else
        {
            apu->pulse_timer_clock[channel]--;
            output_summ += apu->pulse_output_sample[channel];
        }
    }
    return output_summ == 0 ? 0 : output_summ / ticks;
}

static uint16_t ApuDoTriangle(APU* apu, uint16_t ticks)
{
#if 1
    if (apu->triangle_length_counter == 0 || apu->triangle_linear_counter == 0 || apu->triangle_timer < 2)
        return apu->triangle_output_sample;

    uint32_t summ = 0;
    for (uint16_t t = 0; t < ticks; ++t)
    {
        if (apu->triangle_timer_clock == 0)
        {
            apu->triangle_sampler = (apu->triangle_sampler + 1) & 31;
            apu->triangle_timer_clock = apu->triangle_timer;
            apu->triangle_output_sample = g_triangle[apu->triangle_sampler];
            summ += apu->triangle_output_sample;
        }
        else
        {
            apu->triangle_timer_clock--;
            summ += apu->triangle_output_sample;
        }
    }
    uint16_t avg = summ / ticks;
    return avg;
#else
    while (ticks > 0 && apu->triangle_timer > 0)
    {
        if (ticks >= apu->triangle_timer_clock)
        {
            ticks -= apu->triangle_timer_clock;
            apu->triangle_sampler = (apu->triangle_sampler + 1) & 31;
            apu->triangle_timer_clock = apu->triangle_timer;
        }
        else
        {
            apu->triangle_timer_clock -= ticks;
            break;
        }
    }

    if (apu->triangle_length_counter == 0 || apu->triangle_linear_counter == 0)
        return 0;
    else
    {
        if (apu->triangle_timer < 2)
            return 0;
        return g_triangle[apu->triangle_sampler];
    }
#endif
}

static uint16_t ApuDoNoise(APU* apu, uint16_t ticks)
{
    if (apu->noise_length_counter == 0 ||
        (apu->noise_constant_volume != 0 && apu->noise_volume == 0) ||
        (apu->noise_constant_volume == 0 && apu->noise_envelope_counter == 0) )
    {
        apu->noise_output_sample = 0;
        return 0;
    }

    uint32_t summ = 0;
    for (uint16_t t = 0; t < ticks; ++t)
    {
        if (apu->noise_timer_tick == 0)
        {
            apu->noise_timer_tick = g_noise_timer_ntsc[apu->noise_period] / 2;
            if (apu->noise_mode)
            {
                uint16_t feedback = ((apu->noise_shift_register >> 8) & 1) ^ ((apu->noise_shift_register >> 14) & 1);
                apu->noise_shift_register = (apu->noise_shift_register << 1) + feedback;
                apu->noise_shift_register &= 0x7fff;
                //apu->noise_update = 1;
            }
            else
            {
                uint16_t feedback = ((apu->noise_shift_register >> 13) & 1) ^ ((apu->noise_shift_register >> 14) & 1);
                apu->noise_shift_register = (apu->noise_shift_register << 1) + feedback;
                apu->noise_shift_register &= 0x7fff;
                //apu->noise_update = 1;
            }

            if (apu->noise_length_counter == 0)
                apu->noise_output_sample = 0;
            else
            {
                if (apu->noise_constant_volume)
                {
                    apu->noise_output_sample = (uint16_t)((apu->noise_shift_register >> 0xe) & 1) * 5000;  //*1024*8
                    apu->noise_output_sample = (apu->noise_output_sample * apu->noise_volume) >> 4;
                }
                else
                {
                    apu->noise_output_sample = (uint16_t)((apu->noise_shift_register >> 0xe) & 1) * 5000;  //*1024*8
                    apu->noise_output_sample = (apu->noise_output_sample * apu->noise_envelope_counter) >> 4;
                }
            }
            summ += apu->noise_output_sample;
        } else
        {
            apu->noise_timer_tick--;
            summ += apu->noise_output_sample;
        }
    }
    return summ / ticks;
}

void APU_write(APU* apu, uint16_t addr, uint8_t val)
{
    switch(addr)
    {
    //Pulse
    case 0x4000:
    case 0x4004:
    {
        uint8_t n = (addr >> 2) & 1;
        apu->pulse_duty_cycle[n] = (val >> 6) & 0x3;
        apu->pulse_length_counter_halt[n] = (val >> 5) & 0x1;
        apu->pulse_constant_volume[n] = (val >> 4) & 0x1;
        apu->pulse_volume[n] = val & 0xF;
        //apu->pulse_update[n] = 1;
        apu->pulse_envelope_loop[n] = (val >> 5) & 0x1;
        apu->pulse_envelope_div_period[n] = val & 0xF;
    }
    break;

    case 0x4001:
    case 0x4005:
    {
        uint8_t n = (addr >> 2) & 1;
        apu->pulse_sweep_enable[n] = val >> 7;
        apu->pulse_sweep_divider_period[n] = (val >> 4) & 7;
        apu->pulse_sweep_negate[n] = (val >> 3) & 1;
        apu->pulse_sweep_shift_count[n] = val & 7;
        //apu->pulse_update[n] = 1;
        apu->pulse_sweep_update[n] = 1;
    }
    break;

    case 0x4002:
    case 0x4006:
    {
        uint8_t n = (addr >> 2) & 1;
        apu->pulse_timer[n] = (uint16_t)val | (apu->pulse_timer[n] & 0xFF00);
        //apu->pulse_update[n] = 1;
    }
    break;

    case 0x4003:
    case 0x4007:
    {
        uint8_t n = (addr >> 2) & 1;
        apu->pulse_timer[n] = (apu->pulse_timer[n] & 0xFF) | ((val&7) << 8);
        apu->pulse_length_counter[n] = g_lengthtable[(val >> 3) & 0x1F];
        //apu->pulse_update[n] = 1;
        apu->pulse_envelope_update[n] = 1;
    }
    break;

    //Triangle
    case 0x4008:
        apu->triangle_length_counter_halt = (val >> 7) & 0x1;
        apu->triangle_linear_counter_load = val & 0x7F;
    break;

    case 0x400A:
        apu->triangle_timer = (uint16_t)val | (apu->triangle_timer & 0xFF00);
    break;

    case 0x400B:
        apu->triangle_timer = (apu->triangle_timer & 0xFF) | ((val&7) << 8);
        apu->triangle_length_counter = g_lengthtable[(val >> 3) & 0x1F];
        apu->triangle_counter_reload_flag = 1;
    break;

    //Noise
    case 0x400C:
        apu->noise_length_counter_halt = (val >> 5) & 0x1;
        apu->noise_constant_volume = (val >> 4) & 0x1;
        apu->noise_volume = val & 0xF;
        apu->noise_envelope_div_period = val & 15;
        apu->noise_envelope_loop = (val >> 5) & 1;
    break;

    case 0x400E:
        apu->noise_mode = (val >> 7) & 0x1;
        apu->noise_period = val & 0xF;
    break;

    case 0x400F:
        apu->noise_length_counter = g_lengthtable[(val >> 3) & 0x1F];
        apu->noise_envelope_write = 1;
    break;

    case 0x4015:
        apu->channel_enable = val;
    break;
    }
}

static uint16_t Apu_ExtractSample(APU* apu, uint16_t cpu_ticks)
{
    apu->frames_counter_120Hz++;
    if (apu->frames_counter_120Hz >= AUDIO_SAMPLE_RATE / 120)
    {
        apu->frames_counter_120Hz = 0;
        for (uint8_t n = 0; n < 2; ++n)
        {
            if (apu->pulse_length_counter_halt[n] == 0 && apu->pulse_length_counter[n] > 0)
                apu->pulse_length_counter[n]--;
            if (apu->pulse_sweep_enable[n])
            {
                if (apu->pulse_sweep_divider[n] > 0)
                    apu->pulse_sweep_divider[n]--;
                if (apu->pulse_sweep_divider[n] == 0)
                {
                    apu->pulse_sweep_divider[n] = apu->pulse_sweep_divider_period[n] + 1;

                    uint16_t shift = apu->pulse_timer[n] >> apu->pulse_sweep_shift_count[n];
                    if (n == 0 && apu->pulse_sweep_negate[n])
                        shift += 1;
                    apu->pulse_sweep_timer[n] = apu->pulse_timer[n] + (apu->pulse_sweep_negate[n] ? -shift : shift);

                    if (apu->pulse_timer[n] >= 8 && apu->pulse_sweep_timer[n] < 0x800 && apu->pulse_sweep_shift_count[n] != 0)
                        apu->pulse_timer[n] = apu->pulse_sweep_timer[n] < 0 ? 0 : apu->pulse_sweep_timer[n];
                }
            }

            if (apu->pulse_sweep_update[n])
            {
                apu->pulse_sweep_divider[n] = apu->pulse_sweep_divider_period[n] + 1;
                apu->pulse_sweep_update[n] = 0;
            }
        }

        //Triangle tick
        if (apu->triangle_counter_reload_flag)
            apu->triangle_linear_counter = apu->triangle_linear_counter_load;
        if (apu->triangle_length_counter_halt == 0)
        {
            if (apu->triangle_length_counter > 0)
                apu->triangle_length_counter--;
            if (apu->triangle_linear_counter > 0)
                apu->triangle_linear_counter--;
            apu->triangle_counter_reload_flag = 0;
        }

        //Noise envelope
        {
            uint8_t divider = 1;
            if (apu->noise_envelope_write)
            {
                apu->noise_envelope_write = 0;
                apu->noise_envelope_counter = 15;
                apu->noise_envelope_div = 0;
            }
            else
            {
                ++apu->noise_envelope_div;
                if (apu->noise_envelope_div > apu->noise_envelope_div_period)
                {
                    divider ++;
                    apu->noise_envelope_div = 0;
                }
            }
            if (divider)
            {
                if (apu->noise_envelope_loop && apu->noise_envelope_counter == 0)
                    apu->noise_envelope_counter = 15;
                else if (apu->noise_envelope_counter > 0)
                    --apu->noise_envelope_counter;
            }
        }
    }

    apu->frames_counter_240Hz++;
    if (apu->frames_counter_240Hz >= AUDIO_SAMPLE_RATE / 240)
    {
        apu->frames_counter_240Hz = 0;
        for (uint8_t n = 0; n < 2; ++n)
        {
            uint8_t divider = 0;
            if (apu->pulse_envelope_update[n])
            {
                apu->pulse_envelope_update[n] = 0;
                apu->pulse_envelope_counter[n] = 15;
                apu->pulse_envelope_div[n] = 0;
            }
            else
            {
                apu->pulse_envelope_div[n] ++;
                if (apu->pulse_envelope_div[n] > apu->pulse_envelope_div_period[n])
                {
                    divider ++;
                    apu->pulse_envelope_div[n] = 0;
                }
            }
            if (divider)
            {
                if (apu->pulse_envelope_loop[n] && apu->pulse_envelope_counter[n] == 0)
                    apu->pulse_envelope_counter[n] = 15;
                else if (apu->pulse_envelope_counter[n] > 0)
                    apu->pulse_envelope_counter[n]--;
            }
        }
    }

    uint16_t sample = 0;
    if ((uint8_t)(apu->channel_enable & 1) != 0)
        sample += ApuDoPulse(apu, cpu_ticks, 0);
    if ( (uint8_t)(apu->channel_enable & 2) != 0 )
        sample += ApuDoPulse(apu, cpu_ticks, 1);
    if ( (uint8_t)(apu->channel_enable & 4) != 0 )
        sample += ApuDoTriangle(apu, cpu_ticks);
    if ((uint8_t)(apu->channel_enable & 8) != 0)
        sample += ApuDoNoise(apu, cpu_ticks);
    return sample;
}

#ifdef WIN32
FILE* s_file_dump = 0;
#endif

#ifndef WIN32
//portMUX_TYPE s_apu_mutex = portMUX_INITIALIZER_UNLOCKED;
#endif

void APU_Init(APU* apu)
{
    memset(apu, 0, sizeof(APU));
    apu->pulse_sweep_divider[0] = 1;
    apu->pulse_sweep_divider[1] = 1;
    apu->noise_shift_register = 1;
#ifdef WIN32
    fopen_s(&s_file_dump, "apu.bin", "wb");
#endif

#if 0
    i2s_config_t i2s_config;
    memset(&i2s_config, 0, sizeof(i2s_config));
    i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN);
    i2s_config.sample_rate = AUDIO_SAMPLE_RATE;
    i2s_config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT; /* the DAC module will only take the 8bits from MSB */
    i2s_config.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
    i2s_config.communication_format = I2S_COMM_FORMAT_I2S_MSB;
    i2s_config.intr_alloc_flags = 0; // default interrupt priority
    i2s_config.dma_buf_count = 2;
    i2s_config.dma_buf_len = 128;
    i2s_config.use_apll = false;

    i2s_driver_install(i2s_num, &i2s_config, 0, NULL);   //install and start i2s driver
    i2s_set_pin(i2s_num, NULL); //for internal DAC, this will enable both of the internal channels
    i2s_set_sample_rates(i2s_num, AUDIO_SAMPLE_RATE); //set sample rates
#endif
}

#if defined(ESP8266)
void ApuWriteDAC(uint16_t DAC)
{
    uint16_t i2sACC = 0;
    static uint16_t err = 0;
    for (uint8_t i = 0; i < 32; i++)
    {
        i2sACC = i2sACC << 1;
        if (DAC >= err)
        {
            i2sACC |= 1;
            err += 0xFFFF - DAC;
        }
        else
        {
            err -= DAC;
        }
    }
    i2s_write_sample(i2sACC);
}
#endif

void APU_cpu_tick(APU* apu, uint16_t cycles)
{
    //2 CPU cycles = 1 APU cycle
    apu->cycles_count += cycles * 7;     //261.0085625 (7*1789773 / 48000)
    while (apu->cycles_count >= 261)     //make 48000 loop
    {
        apu->cycles_count -= 261;
        //calculate apu ticks
        apu->cycles_acumulator += 261;
        uint16_t apu_cnt = apu->cycles_acumulator / 14;
        apu->cycles_acumulator -= apu_cnt * 14;
        uint16_t vol = Apu_ExtractSample(apu, apu_cnt) * 2;
        apu->output_filter = (apu->output_filter * (4096-32) + vol * 32) >> 12;

        int16_t sample = (int16_t)(vol)-(int16_t)apu->output_filter;

#if defined(ESP8266)
		i2s_write_sample( (int32_t)sample | ((int32_t)sample << 16));
#elif WIN32
        if (s_file_dump)
            fwrite(&sample, sizeof(sample), 1, s_file_dump);
#endif
    }
}
