#include "adc.h"
#include "gd32vf103.h"

void ADC0powerUpInit(void)
{
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_ADC0);

    rcu_adc_clock_config(RCU_CKADC_CKAPB2_DIV8);

    // PA0 / A0 som analog ingång
    gpio_init(GPIOA,
              GPIO_MODE_AIN,
              GPIO_OSPEED_50MHZ,
              GPIO_PIN_0);

    adc_deinit(ADC0);

    adc_mode_config(ADC_MODE_FREE);

    adc_special_function_config(
        ADC0,
        ADC_CONTINUOUS_MODE,
        DISABLE
    );

    adc_special_function_config(
        ADC0,
        ADC_SCAN_MODE,
        DISABLE
    );

    adc_data_alignment_config(
        ADC0,
        ADC_DATAALIGN_RIGHT
    );

    adc_channel_length_config(
        ADC0,
        ADC_REGULAR_CHANNEL,
        1
    );

    // A0 = ADC channel 0
    adc_regular_channel_config(
        ADC0,
        0,
        ADC_CHANNEL_0,
        ADC_SAMPLETIME_13POINT5
    );

    adc_external_trigger_source_config(
        ADC0,
        ADC_REGULAR_CHANNEL,
        ADC0_1_EXTTRIG_REGULAR_NONE
    );

    adc_external_trigger_config(
        ADC0,
        ADC_REGULAR_CHANNEL,
        ENABLE
    );

    adc_enable(ADC0);

    for (int i = 0; i < 0xFFFF; i++);

    adc_calibration_enable(ADC0);

    for (int i = 0; i < 0xFFFF; i++);

    // Starta första mätningen
    adc_software_trigger_enable(
        ADC0,
        ADC_REGULAR_CHANNEL
    );
}