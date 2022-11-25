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

volatile uint8_t black_value, white_value, status;
volatile uint16_t current_sample_average, previous_sample_average;
volatile uint32_t sample_index, sample_sum;

volatile int64_t duration_sum, duration_average;

node_t node = { 0, NULL };

char print_character;

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

char barcode_characters_find(uint8_t black_value, uint8_t white_value, barcode_character_t *barcode_characters) { // find a barcode character
    for (uint8_t i = 0; i < 27; i++) { // iterate through all 27 characters
        if (barcode_characters[i].black_value == black_value && barcode_characters[i].white_value == white_value) // compare the black and white values
            return barcode_characters[i].character; // return the character if match
    }

    return '\0'; // return terminating character if nothing found
}
/* END OF BARCODE CHARACTERS */

/* QUEUE */
bars_t bars = { NULL, NULL, 0 }; // linked list

void bars_add(int64_t duration, bars_t *bars) { // add a bar
    node_t *node = malloc(sizeof(node_t)); // allocate memory for a new node
    node->duration = duration; // set the duration of the node
    node->next_node = NULL; // set the pointer of the next node

    if (bars->start_node == NULL) { // linked list is empty
        bars->start_node = bars->end_node = node; // set the start node & end node to the new node
        bars->length = 1; // set the length to 1
    } else if (bars->length == 9) { // linked list is full - 9 nodes
        node_t *delete_node = bars->start_node; // set the start node pointer for deletion

        bars->start_node = bars->start_node->next_node; // set the start node to the 2nd node
        bars->end_node->next_node = node; // set the 2nd last node's next node to the new node
        bars->end_node = node; // set the end node to the new node

        free(delete_node); // free the previous start node pointer to delete
    } else if (bars->length < 9) { // linked list is not empty & not full
        bars->end_node->next_node = node; // set the current end node's next node to the new node
        bars->end_node = node; // set the end node to the new node
        bars->length++; // increment the length of the linked list
    }
}

uint8_t bars_is_ready(bars_t *bars) { // is bar ready
    return bars->start_node != NULL && bars->end_node != NULL && bars->length == 9; // return true if start and end node is not empty, and has length of 9
}
/* END OF QUEUE */

void adc_handler() { // adc handler
    if (!start_time) // current time is not saved
        start_time = get_absolute_time(); // save the current time
    
    if (!adc_fifo_is_empty()) { // check if adc fifo queue is empty
        if (sample_index < IR_SENSOR_SAMPLE_SIZE) { // not enough sample size
            sample_sum += adc_fifo_get(); // add to the sum of samples
            sample_index++; // increment the amount of samples
        } else { // enough sample size
            current_sample_average = sample_sum / IR_SENSOR_SAMPLE_SIZE; // get the sample average

            //printf("%d\n", current_sample_average);

            if (abs(current_sample_average - previous_sample_average) >= IR_SENSOR_DIFFERENCE) { // there is a significant difference from previous sample
                previous_sample_average = current_sample_average; // set the previous sample average to current sample average 
                
                //printf("%d\n", current_sample_average);

                previous_detected_surface = current_detected_surface; // set the previous detected surface to current detected surface
                current_detected_surface = current_sample_average >= IR_SENSOR_THRESHOLD ? BLACK_SURFACE : WHITE_SURFACE; // set the current detected surface based on threshold

                if (current_detected_surface != previous_detected_surface) { // there is a change in surface colour
                    bars_add(absolute_time_diff_us(start_time, get_absolute_time()), &bars); // add the duration of change
                    start_time = get_absolute_time(); // restart the current time

                    if (bars_is_ready(&bars)) { // if we have 9 bars
                        node_t node = *bars.start_node; // set the node to start node
                        for (uint8_t i = 0; i < 9; i++) { // iterate through all 9 nodes
                            duration_sum += node.duration; // add to the sum of durations
                            node = *node.next_node; // go to next node
                        }

                        duration_average = duration_sum / 9; // get the duration average

                        node = *bars.start_node; // set the node to start node
                        for (uint8_t i = 0; i < 9; i++) { // iterate through all 9 nodes
                            if (i % 2 == 0) { // even is black
                                black_value = black_value << 1; // left shift 1 bit on black value
                                if (node.duration >= duration_average) // is thick bar
                                    black_value = black_value | 0x1; // bitwise OR to set to 1
                            } else { // odd is white
                                white_value = white_value << 1; // left shift 1 bit on white value
                                if (node.duration >= duration_average) // is thick bar
                                    white_value = white_value | 0x1; // bitwise OR to set to 1
                            }
                            node = *node.next_node; //go to next node
                        }

                        char character = barcode_characters_find(black_value, white_value, barcode_characters); // find character

                        if (character != '\0') { // valid character found
                            if (character == '*' && status == 0) // 1st asterisk
                                status = 1; // set status to 1 - waiting for actual character
                            else if (character != '*' && status == 1) { // actual character
                                print_character = character; // save the actual character
                                status = 2; // set status to 2 - waiting 2nd asterisk
                            } else if (character == '*' && status == 2) { // 2nd asterisk
                                printf("*%c*\n", print_character); // print actual character
                                print_character = '\0'; // reset actual character
                                status = 0; // reset status
                            } else { // not valid character 
                                print_character = '\0'; // reset actual character
                                status = 0; // reset status
                            }

                            bars = (bars_t) { NULL, NULL, 0 }; // flush linked list
                        }

                        duration_sum = duration_average = black_value = white_value = 0; // reset values
                    }
                }
            }

            sample_sum = 0; // reset the total sample sum
            sample_index = 0; // reset the number of samples collected
        }
    }

    irq_clear(ADC_IRQ_FIFO); // clear IRQ flag
}

void barcode_init() { // init barcode
    adc_init(); // init ADC
    adc_gpio_init(IR_SENSOR_ADC_PIN); // init the ADC pin
    adc_select_input(IR_SENSOR_ADC_CHANNEL); // select ADC channel
    adc_fifo_setup(true, false, 1, false, false); // set up FIFO with 4 samples
    adc_irq_set_enabled(true); // enable interrupts for ADC
    adc_run(true); // enable free running mode
    
    irq_set_exclusive_handler(ADC_IRQ_FIFO, adc_handler); // set exclusive handler for ADC
	irq_set_enabled(ADC_IRQ_FIFO, true); // enable irq for ADC

    printf("BARCODE READER IS READY!\n");
}

int main() { // main function

    stdio_init_all(); // init standard input output
    sleep_ms(1000); // wait for standard input output to initialize

    barcode_init(); // init barcode

    while(1);

    return 0;
}