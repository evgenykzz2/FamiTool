#include "APU.h"

#define AUDIO_SAMPLE_RATE 48000

#ifndef WIN32
#include <Arduino.h>
#include <driver/dac.h>
#include "driver/i2s.h"

static const i2s_port_t i2s_num = I2S_NUM_0; // i2s port number
#endif

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

static const uint16_t g_noise_timer[16] = { 4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068 };

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

    uint16_t timer;

    uint8_t duty_clock;
    uint16_t timer_clock;

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
    }

    uint16_t ApuTick(uint16_t ticks) override
    {
        while (ticks > 0 && timer > 0)
        {
            if (ticks >= timer_clock)
            {
                ticks -= timer_clock;
                duty_clock = (duty_clock + 1) & 7;
                need_update = true;
                timer_clock = timer;
            }
            else
            {
                timer_clock -= ticks;
                break;
            }
        }

        if (need_update)
        {
            if (timer < 8 || timer > 0x7ff)
            {
                output_sample = 0;
            }
            else
                output_sample = g_pattern[duty_cycle * 8 + duty_clock];

            if (length_counter == 0)
                output_sample = 0;


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

        return output_sample;
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
            timer_tick = g_noise_timer[period] / 32;
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

Pulse       s_pulse[2];
Triangle    s_triangle;
Noise       s_noise;
uint8_t     s_channel_enable;

void APU_write(uint16_t addr, uint8_t val)
{
  switch(addr)
  {
    //Pulse
  case 0x4000:
  case 0x4004:
    {
      uint8_t n = (addr >> 2) & 1;
      s_pulse[n].duty_cycle = (val >> 6) & 0x3;
      s_pulse[n].length_counter_halt = (val >> 5) & 0x1;
      s_pulse[n].constant_volume = (val >> 4) & 0x1;
      s_pulse[n].volume = val & 0xF;
      s_pulse[n].need_update = true;
    }
    break;

  case 0x4001:
  case 0x4005:
    {
      uint8_t n = (addr >> 2) & 1;
      s_pulse[n].sweep_enable = val >> 7;
      s_pulse[n].sweep_divider_period = (val >> 4) & 7;
      s_pulse[n].sweep_negate = (val >> 3) & 1;
      s_pulse[n].sweep_shift_count = val & 7;
      s_pulse[n].need_update = true;
    }
    break;

  case 0x4002:
  case 0x4006:
    {
      uint8_t n = (addr >> 2) & 1;
      s_pulse[n].timer = (uint16_t)val | (s_pulse[n].timer & 0xFF00);
      s_pulse[n].need_update = true;
    }
    break;

  case 0x4003:
  case 0x4007:
    {
      uint8_t n = (addr >> 2) & 1;
      s_pulse[n].timer = (s_pulse[n].timer & 0xFF) | ((val&7) << 8);
      s_pulse[n].length_counter = g_lengthtable[(val >> 3) & 0x1F];
      s_pulse[n].need_update = true;
    }
    break;

    //Triangle
  case 0x4008:
    s_triangle.length_counter_halt = (val >> 7) & 0x1;
    s_triangle.linear_counter_load = val & 0x7F;
    s_triangle.need_update = true;
    break;

  case 0x400A:
    s_triangle.timer = (uint16_t)val | (s_triangle.timer & 0xFF00);
    s_triangle.need_update = true;
    break;

  case 0x400B:
    s_triangle.timer = (s_triangle.timer & 0xFF) | ((val&7) << 8);
    s_triangle.length_counter = g_lengthtable[(val >> 3) & 0x1F];
    s_triangle.counter_reload_flag = 1;
    s_triangle.need_update = true;
    break;

    //Noise
  case 0x400C:
    s_noise.length_counter_halt = (val >> 5) & 0x1;
    s_noise.constant_volume = (val >> 4) & 0x1;
    s_noise.volume = val & 0xF;
    s_noise.need_update = true;
    break;

  case 0x400E:
    s_noise.mode = (val >> 7) & 0x1;
    s_noise.period = val & 0xF;
    s_noise.need_update = true;
    break;

  case 0x400F:
    s_noise.length_counter = g_lengthtable[val >> 3];
    s_noise.need_update = true;
    break;

  case 0x4015:
    s_channel_enable = val;
    break;
  }
}

static uint32_t s_frame_counter = 0;

uint16_t Apu_ExtractSample(int cpu_ticks)
{
  //6502 Audio
  s_frame_counter++;
  if ( s_frame_counter >= AUDIO_SAMPLE_RATE / 120)
  {
    s_frame_counter = 0;
    s_pulse[0].ApuFrameTick();
    s_pulse[1].ApuFrameTick();
    s_triangle.ApuFrameTick();
    s_noise.ApuFrameTick();
  }

  uint16_t sample = 0;
  if ( (uint8_t)(s_channel_enable & 1) != 0 )
    sample += s_pulse[0].ApuTick(cpu_ticks);
  if ( (uint8_t)(s_channel_enable & 2) != 0 )
    sample += s_pulse[1].ApuTick(cpu_ticks);
  if ( (uint8_t)(s_channel_enable & 4) != 0 )
    sample += s_triangle.ApuTick(cpu_ticks);
  if ( (uint8_t)(s_channel_enable & 8) != 0 )
    sample += s_noise.ApuTick(cpu_ticks);

      /*
      //SS5 Audio
      //1789773/(48000*16*2) = 1789773/1536000
      m_ss5_tick_counter++;
      uint64_t ss5t = m_ss5_tick_counter*1789773/(AUDIO_SAMPLE_RATE*16*2);
      while (m_ss5_tick < ss5t)
      {
        for (int c = 0; c < 3; ++c)
        {
          if (m_ss5_counter[c] == 0)
          {
            m_ss5_counter[c] = ((uint16_t)m_ss5_reg[c*2] | ((uint16_t)(m_ss5_reg[c*2+1] & 0x0F) << 8)) + 1;
            m_ss5_value[c] ^= 1;
          } else
            m_ss5_counter[c] --;
        }
        m_ss5_tick ++;
      }

      uint16_t ss5_mix = 0;
      for (int c = 0; c < 3; ++c)
      {
        if (m_ss5_value[c] && (uint8_t)(m_ss5_counter[7] >> c & 1))
          ss5_mix += m_ss5_reg[c+8] & 0x0F;
      }
      sample += ss5_mix << 10;
      if (sample > INT16_MAX)
        sample = INT16_MAX;
      if (sample < INT16_MIN)
        sample = INT16_MIN;*/
  /*sample = sample/2 + INT16_MAX/2;
  if (sample > INT16_MAX)
    sample = INT16_MAX;
  if (sample < INT16_MIN)
    sample = INT16_MIN;*/
  return sample;
}

static int s_apu_cycles = 0;
static int s_apu_sample = 0;

int16_t s_apu_buffer[64];
volatile uint8_t s_apu_pos_read = 0;
volatile uint8_t s_apu_pos_write = 0;

#ifndef WIN32
//portMUX_TYPE s_apu_mutex = portMUX_INITIALIZER_UNLOCKED;
#endif

uint8_t APU_pop_sample()
{
  uint8_t sample = 0;
#ifndef WIN32
  //portENTER_CRITICAL_ISR(&s_apu_mutex);
  sample = s_apu_buffer[s_apu_pos_read & 63];
  if (s_apu_pos_read < s_apu_pos_write)
    s_apu_pos_read ++;
  //portEXIT_CRITICAL_ISR(&s_apu_mutex);
#endif
  return sample;
}

void APU_Init()
{
#ifndef WIN32
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

int s_cpu_cycles_total = 0;
void APU_cpu_tick(int cycles)
{
  s_cpu_cycles_total += cycles;
  s_apu_cycles += cycles * 7;   //261.0085625
  //int sample = (uint64_t)(s_apu_cycles * AUDIO_SAMPLE_RATE) / 1789773;
  //for (int i = s_apu_sample; i < sample; ++i)
  if (s_apu_cycles >= 261)
  {
    s_apu_cycles -= 261;
    int16_t vol = Apu_ExtractSample(s_cpu_cycles_total >> 1);
    s_cpu_cycles_total = s_cpu_cycles_total & 1;
#ifndef WIN32
    //portENTER_CRITICAL_ISR(&s_apu_mutex);
    s_apu_buffer[s_apu_pos_write & 63] = vol;
    s_apu_pos_write ++;
    if (s_apu_pos_write == 64)
    {
      s_apu_pos_write = 0;
      size_t bytes_written;
      i2s_write(i2s_num, s_apu_buffer, 128, &bytes_written, portMAX_DELAY);
    }
    //portEXIT_CRITICAL_ISR(&s_apu_mutex);
#else
#endif
  }
  /*s_apu_sample = sample;
  if (s_apu_cycles >= 1789773)
  {
    s_apu_cycles -= 1789773;
    s_apu_sample -= AUDIO_SAMPLE_RATE;
  }*/
}
