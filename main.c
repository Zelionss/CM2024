/*
 * CM2024 - Hearing-loop relative signal-strength indicator
 * Target: GD32VF103 + KTH IO-Card 8x8 LED matrix
 * Analog input: PA0 / ADC0 channel 0, biased to ~1.65 V by MCP6002.
 *
 * Signal processing:
 *   1) acquire 1000 ADC samples;
 *   2) find their mean (removes the ~1.65 V DC bias);
 *   3) calculate mean absolute deviation (MAD) as relative amplitude;
 *   4) apply first-order digital smoothing;
 *   5) map to 0..8 illuminated LED rows.
 *
 * The thresholds below are calibration parameters. They do NOT directly
 * measure magnetic field strength or guarantee particular row counts.
 */

#include <stdint.h>
#include "gd32vf103.h"
#include "drivers.h"
#include "adc.h"

#define SAMPLE_COUNT         1000U
#define ADC_AMPLITUDE_MIN      10U  /* adjust to measured no-signal noise floor */
#define ADC_AMPLITUDE_MAX      80U  /* adjust to desired full-scale sensitivity */
#define FILTER_OLD_WEIGHT      3U
#define FILTER_TOTAL_WEIGHT    4U

static uint16_t samples[SAMPLE_COUNT];
static uint16_t sample_index = 0U;
static uint32_t sample_sum = 0U;
static uint32_t filtered_amplitude = 0U;
static uint8_t filter_initialized = 0U;

static uint8_t amplitude_to_rows(uint32_t amplitude)
{
    if (amplitude <= ADC_AMPLITUDE_MIN) {
        return 0U;
    }
    if (amplitude >= ADC_AMPLITUDE_MAX) {
        return 8U;
    }

    /* Round to nearest LED level. Intermediate levels range 1..7. */
    uint32_t range = ADC_AMPLITUDE_MAX - ADC_AMPLITUDE_MIN;
    uint32_t position = amplitude - ADC_AMPLITUDE_MIN;
    uint32_t rows = (position * 8U + range / 2U) / range;
    if (rows > 8U) {
        rows = 8U;
    }
    return (uint8_t)rows;
}

static void display_rows(uint8_t rows)
{
    uint8_t pattern;
    if (rows == 0U) {
        pattern = 0x00U;
    } else if (rows >= 8U) {
        pattern = 0xFFU;
    } else {
        pattern = (uint8_t)((1U << rows) - 1U);
    }

    for (uint8_t column = 0U; column < 8U; ++column) {
        l88mem(column, pattern);
    }
}

static void process_window(void)
{
    const uint32_t mean = sample_sum / SAMPLE_COUNT;
    uint32_t absolute_deviation_sum = 0U;

    for (uint16_t i = 0U; i < SAMPLE_COUNT; ++i) {
        uint32_t value = samples[i];
        absolute_deviation_sum += (value >= mean) ? (value - mean) : (mean - value);
    }

    const uint32_t amplitude = absolute_deviation_sum / SAMPLE_COUNT;
    if (!filter_initialized) {
        filtered_amplitude = amplitude;
        filter_initialized = 1U;
    } else {
        filtered_amplitude = (FILTER_OLD_WEIGHT * filtered_amplitude + amplitude
                            + FILTER_TOTAL_WEIGHT / 2U) / FILTER_TOTAL_WEIGHT;
    }

    display_rows(amplitude_to_rows(filtered_amplitude));
    sample_index = 0U;
    sample_sum = 0U;
}

int main(void)
{
    t5omsi();
    colinit();
    l88init();
    display_rows(0U);
    ADC0powerUpInit(); /* adc.c must configure PA0 / ADC_CHANNEL_0 */

    while (1) {
        /* Refresh multiplexed LED matrix independently of measurement windows. */
        if (t5expq()) {
            l88row(colset());
        }

        /* Single ADC conversion complete (no continuous mode needed). */
        if (adc_flag_get(ADC0, ADC_FLAG_EOC) != RESET) {
            const uint16_t sample = (uint16_t)adc_regular_data_read(ADC0);
            adc_flag_clear(ADC0, ADC_FLAG_EOC);

            samples[sample_index] = sample;
            sample_sum += sample;
            ++sample_index;

            if (sample_index >= SAMPLE_COUNT) {
                process_window();
            }

            adc_software_trigger_enable(ADC0, ADC_REGULAR_CHANNEL);
        }
    }
}