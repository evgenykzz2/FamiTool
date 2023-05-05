#pragma once
#include <stdint.h>

struct APU
{
    uint16_t cycles_count;
    uint16_t cycles_acumulator;
    uint16_t frames_counter_120Hz;
    uint16_t frames_counter_240Hz;

    uint16_t output_filter;

    uint8_t channel_enable;

    //---------------------------- Pulse
    uint8_t pulse_duty_cycle[2];
    uint8_t pulse_constant_volume[2];
    uint8_t pulse_volume[2];

    uint8_t pulse_sweep_enable[2];
    uint8_t pulse_sweep_divider_period[2];
    uint8_t pulse_sweep_negate[2];
    uint8_t pulse_sweep_shift_count[2];
    uint16_t pulse_timer[2];    //frec

    uint8_t pulse_envelope_loop[2];
    uint8_t pulse_envelope_div_period[2];

    uint8_t pulse_length_counter_halt[2];
    uint8_t pulse_length_counter[2];

    //internal
    //uint8_t pulse_update[2];
    uint8_t pulse_sweep_update[2];
    uint8_t pulse_envelope_update[2];

    uint8_t pulse_envelope_counter[2];
    uint8_t pulse_envelope_div[2];

    uint8_t pulse_duty_clock[2];
    uint16_t pulse_timer_clock[2];
    uint16_t pulse_output_sample[2];
    uint8_t pulse_sweep_divider[2];
    uint16_t pulse_sweep_timer[2];

    //---------------------------- Triangle
    uint8_t triangle_linear_counter_load;
    uint8_t triangle_linear_counter;
    uint8_t triangle_counter_reload_flag;
    uint16_t triangle_timer;
    uint16_t triangle_timer_clock;
    uint8_t triangle_length_counter_halt;
    uint8_t triangle_length_counter;
    uint8_t triangle_sampler;
    uint16_t triangle_output_sample;
    uint16_t triangle_output_sample_filter;
    uint16_t triangle_volume_smooth;

    //---------------------------- Noise
    uint8_t noise_length_counter_halt;
    uint8_t noise_length_counter;
    uint8_t noise_constant_volume;
    uint8_t noise_volume;

    uint8_t noise_mode;
    uint8_t noise_period;

    uint8_t noise_envelope_div_period;
    uint8_t noise_envelope_loop;
    uint8_t noise_envelope_write;
    uint8_t noise_envelope_counter;
    uint8_t noise_envelope_div;

    uint8_t noise_update;
    uint16_t noise_shift_register;
    uint16_t noise_timer_tick;
    uint16_t noise_output_sample;
};


extern APU s_apu;

void APU_Init(APU* apu);
//uint8_t APU_pop_sample();
void APU_write(APU* apu, uint16_t addr, uint8_t val);
void APU_cpu_tick(APU* apu, uint16_t cycles);