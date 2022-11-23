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

volatile int64_t duration_sum, duration_average;

absolute_time_t start_time, end_time = 0;

/* START OF BARCODE CHARACTERS */
barcode_character_t barcode_characters[27] = 
{
    { 'A', 0x11, 0x02 },
    { 'B', 0x09, 0x02 }, 
    { 'C', 0x18, 0x02 }, 
    { 'D', 0x05, 0x02 },
    { 'E', 0x14, 0x02 }, 
    { 'F', 0x0C, 0x02 },
    { 'G', 0x03, 0x02 }, 
    { 'H', 0x12, 0x02 }, 
    { 'I', 0x0A, 0x02 }, 
    { 'J', 0x06, 0x02 }, 
    { 'K', 0x11, 0x01 }, 
    { 'L', 0x09, 0x01 }, 
    { 'M', 0x18, 0x01 }, 
    { 'N', 0x05, 0x01 }, 
    { 'O', 0x14, 0x01 }, 
    { 'P', 0x0C, 0x01 },  
    { 'Q', 0x03, 0x01 }, 
    { 'R', 0x12, 0x01 }, 
    { 'S', 0x0A, 0x01 },
    { 'T', 0x06, 0x01 }, 
    { 'U', 0x11, 0x08 }, 
    { 'V', 0x09, 0x08 }, 
    { 'W', 0x18, 0x08 },
    { 'X', 0x05, 0x08 }, 
    { 'Y', 0x14, 0x08 }, 
    { 'Z', 0x0C, 0x08 },
    { '*', 0x06, 0x08 }, 
};

char barcode_characters_find(uint8_t black_value, uint8_t white_value, barcode_character_t *barcode_characters) {
    for (uint8_t i = 0; i < 27; i++) {
        if (barcode_characters[i].black_value == black_value && barcode_characters[i].white_value == white_value)
            return barcode_characters[i].character;
    }

    return '\0';
}
/* END OF BARCODE CHARACTERS */

/* QUEUE */
bars_t bars = { NULL, NULL, 0 };

void bars_add(int64_t duration, bars_t *bars) {
    node_t *node = malloc(sizeof(node_t));
    node->duration = duration;
    node->next_node = NULL;

    //node_t *delete_node = NULL;

    if (bars->start_node == NULL) {
        bars->start_node = bars->end_node = node;
        bars->length = 1;
    } else if (bars->length == 9) {
        //delete_node = bars->start_node;

        bars->start_node = bars->start_node->next_node; // remove head, set to 2nd in line
        bars->end_node->next_node = node;
        bars->end_node = node;

        //free(delete_node);
    } else if (bars->length < 9) {
        bars->end_node->next_node = node;
        bars->end_node = node;
        bars->length++;
    }
}

uint8_t bars_is_ready(bars_t *bars) {
    return bars->start_node != NULL && bars->end_node != NULL && bars->length == 9;
}
/* END OF QUEUE */

void adc_handler() {
    if (!start_time)
        start_time = get_absolute_time();
    
    if (!adc_fifo_is_empty()) {
        if (sample_index < IR_SENSOR_SAMPLE_SIZE) {
            sample_sum += adc_fifo_get();
            sample_index++;
        } else {
            current_sample_average = sample_sum / IR_SENSOR_SAMPLE_SIZE;

            //printf("%d\n", current_sample_average);

            if (abs(current_sample_average - previous_sample_average) >= IR_SENSOR_DIFFERENCE) { 
                previous_sample_average = current_sample_average;
                
                //printf("%d\n", current_sample_average);

                previous_detected_surface = current_detected_surface;
                current_detected_surface = current_sample_average >= IR_SENSOR_THRESHOLD ? BLACK_SURFACE : WHITE_SURFACE;

                if (current_detected_surface != previous_detected_surface) {
                    bars_add(absolute_time_diff_us(start_time, get_absolute_time()), &bars);
                    start_time = get_absolute_time();

                    if (bars_is_ready(&bars)) {
                        node_t node = *bars.start_node;
                        for (uint8_t i = 0; i < 9; i++) {
                            duration_sum += node.duration;
                            node = *node.next_node;
                        }

                        duration_average = duration_sum / 9;

                        node = *bars.start_node;
                        for (uint8_t i = 0; i < 9; i++) {
                            if (i % 2 == 0) { 
                                black_value = black_value << 1;
                                if (node.duration >= duration_average)
                                    black_value = black_value | 0x1;
                            } else {
                                white_value = white_value << 1;
                                if (node.duration >= duration_average)
                                    white_value = white_value | 0x1;
                            }
                            node = *node.next_node;
                        }

                        char character =  barcode_characters_find(black_value, white_value, barcode_characters);

                        if (character != '\0') {
                            printf("%c\n", character);
                            bars = (bars_t) { NULL, NULL, 0 };
                        }

                        duration_sum = duration_average = black_value = white_value = 0;
                    }
                }
            }

            sample_sum = 0; // reset the total sample sum
            sample_index = 0; // reset the number of samples collected
        }
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