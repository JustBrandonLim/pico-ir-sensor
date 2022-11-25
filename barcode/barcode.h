#ifndef BARCODE_H_
#define BARCODE_H_

#define IR_SENSOR_ADC_PIN 26 // ADC pin used for IR-sensor
#define IR_SENSOR_ADC_CHANNEL 0 // ADC channel used for IR-sensor

#define IR_SENSOR_SAMPLE_SIZE 50 // number of samples to collect
#define IR_SENSOR_DIFFERENCE 10 // difference before average sample is updated 
#define IR_SENSOR_THRESHOLD 230 // threshold to determine black or white

#define WHITE_SURFACE 0 // 0 signifies white
#define BLACK_SURFACE 1 // 1 signifies black

typedef struct barcode_character_t { // barcode character struct
    char character; // the character itself
    uint32_t black_value; // value of black
    uint32_t white_value; // value of white
} barcode_character_t;

typedef struct node_t { // node struct - for linked list
    int64_t duration; // duration of the bar
    struct node_t* next_node; // pointer to the next node
} node_t;

typedef struct bars_t { // bars struct - linked list
    struct node_t* start_node; // start node of linked list
    struct node_t* end_node; // end node of linked list
    uint8_t length; // length of linked list
} bars_t;

extern void adc_handler(); // adc handler
extern void barcode_init(); // init barcode

#endif