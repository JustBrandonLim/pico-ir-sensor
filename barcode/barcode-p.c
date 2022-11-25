#include <stdio.h>
#include <time.h>
#include <math.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/irq.h"

#include "barcode.h"

volatile uint8_t current_detected_surface = BLACK_SURFACE;
volatile uint8_t previous_detected_surface = BLACK_SURFACE;

volatile uint8_t parse_black, black_value, white_value;
volatile uint16_t current_sample_average, previous_sample_average;
volatile uint32_t sample_index, sample_sum;

uint32_t adc;

void adc_handler() {
    if (!adc_fifo_is_empty()) {
        adc = adc_fifo_get();
        
        if (sample_index < IR_SENSOR_SAMPLE_SIZE) {
            sample_sum += adc;
            sample_index++;
        } else {
            current_sample_average = sample_sum / IR_SENSOR_SAMPLE_SIZE;

            //printf("avg:%d\n", current_sample_average);

            if (abs(current_sample_average - previous_sample_average) >= IR_SENSOR_DIFFERENCE) { 
                previous_sample_average = current_sample_average;

                //printf("avg_rng:%d\n", current_sample_average);
            }

            sample_sum = 0; // reset the total sample sum
            sample_index = 0; // reset the number of samples collected
        }

        printf("%d:%d:%d\n", adc, current_sample_average, previous_sample_average);
    }

    irq_clear(ADC_IRQ_FIFO); // clear IRQ flag
}

int main() { // main function

    stdio_init_all(); // init standard input output
    sleep_ms(1000); // wait for standard input output to initialize

    adc_init(); // init ADC
    adc_gpio_init(IR_SENSOR_ADC_PIN); // init the ADC pin
    adc_select_input(IR_SENSOR_ADC_CHANNEL); // select ADC channel
    adc_fifo_setup(true, false, 1, false, false); // set up FIFO with 4 samples
    adc_irq_set_enabled(true); // enable interrupts for ADC
    adc_run(true); // enable free running mode
    
    irq_set_exclusive_handler(ADC_IRQ_FIFO, adc_handler); // set exclusive handler for ADC
	irq_set_enabled(ADC_IRQ_FIFO, true); // enable irq for ADC

    printf("BARCODE READER IS READY!\n");

    while(1);

    return 0;
}