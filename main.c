#include "gd32vf103.h"
#include "drivers.h"
#include "adc.h"

#define SAMPLE_COUNT 1000

void display_level(uint8_t level)
{
    uint8_t pattern;

    if (level == 0)
    {
        pattern = 0x00;
    }
    else if (level >= 8)
    {
        pattern = 0xFF;
    }
    else
    {
        pattern = (1 << level) - 1;
    }

    for (int col = 0; col < 8; col++)
    {
        l88mem(col, pattern);
    }
}

int main(void)
{
    uint16_t min_value = 4095;
    uint16_t max_value = 0;
    uint16_t sample;
    uint16_t peak_to_peak;

    /*
     * Filtrerat peak-to-peak-värde.
     * static gör att värdet behålls mellan mätningarna.
     */
    static uint16_t filtered_peak = 0;

    int sample_counter = 0;

    t5omsi();
    colinit();
    l88init();
    ADC0powerUpInit();

    display_level(0);

    while (1)
    {
        /*
         * Håll 8x8 LED-matrisen igång.
         */
        if (t5expq())
        {
            l88row(colset());
        }

        /*
         * När en ADC-konvertering är klar.
         */
        if (adc_flag_get(ADC0, ADC_FLAG_EOC) == SET)
        {
            sample = adc_regular_data_read(ADC0);

            adc_flag_clear(
                ADC0,
                ADC_FLAG_EOC
            );

            /*
             * Leta efter minsta och största ADC-värdet.
             */
            if (sample < min_value)
            {
                min_value = sample;
            }

            if (sample > max_value)
            {
                max_value = sample;
            }

            sample_counter++;

            /*
             * Efter 1000 samples beräknar vi
             * signalens peak-to-peak-värde.
             */
            if (sample_counter >= SAMPLE_COUNT)
            {
                peak_to_peak = max_value - min_value;

                /*
                 * Lågpassfiltrering:
                 *
                 * 87.5 % gammalt värde
                 * 12.5 % nytt värde
                 *
                 * Detta gör LED-stapeln stabilare.
                 */
                filtered_peak =
                    (7 * filtered_peak + peak_to_peak) / 8;

                /*
                 * Omvandla filtrerat peak-to-peak-värde
                 * till nivå 0-8.
                 *
                 * Varje 128 ADC-counts motsvarar
                 * ungefär ett extra steg på displayen.
                 */
                uint8_t level = filtered_peak / 128;

                if (level > 8)
                {
                    level = 8;
                }

                display_level(level);

                /*
                 * Börja ett nytt mätfönster.
                 */
                min_value = 4095;
                max_value = 0;
                sample_counter = 0;
            }

            /*
             * Starta nästa ADC-konvertering.
             */
            adc_software_trigger_enable(
                ADC0,
                ADC_REGULAR_CHANNEL
            );
        }
    }
}