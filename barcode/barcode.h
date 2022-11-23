#ifndef BARCODE_H_
#define BARCODE_H_

#define IR_SENSOR_ADC_PIN 26 // ADC pin used for IR-sensor
#define IR_SENSOR_ADC_CHANNEL 0 // ADC channel used for IR-sensor

#define IR_SENSOR_SAMPLE_SIZE 50 // number of samples to collect
#define IR_SENSOR_DIFFERENCE 10 // difference before average sample is updated 
#define IR_SENSOR_THRESHOLD 230 // threshold to determine black or white

#define WHITE_SURFACE 0
#define BLACK_SURFACE 1

typedef struct barcode_character_t {
    char character;
    uint32_t black_value;
    uint32_t white_value;
} barcode_character_t;

typedef struct node_t {
    int64_t duration;
    struct node_t* next_node;
} node_t;

typedef struct bars_t {
    struct node_t* start_node;
    struct node_t* end_node;
    uint8_t length;
} bars_t;

extern void adc_handler();

#endif