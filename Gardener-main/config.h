#ifndef CONFIG_H
#define CONFIG_H

// STATE MACHINE MACROS
#define GENERATE_STATE_ENUM(STATE) STATE,       // Generates STATE entries for enums
#define GENERATE_STATE_STRING(STATE) #STATE,    // Generates STATE entries for a c string array.

#define DEBUG Serial
#define PORT_SENSORS Serial2
#define PORT_ENABLE 4
// 
#define PROPERTIES_SAVE_INTERVAL 10000

// WEBGUI
#define WEBGUI_AP_TIMEOUT 60000 * 10
#define WEBGUI_PORT 80

// TEMPERATURE MANAGEMENT
#define TEMP_MAX 28.0
#define TEMP_MIN 26.0
#define TEMP_MEASUREMENT_INTERVAL 60    // s

// WINDOW ACTUATOR DEFAULTS
#define WINDOW_DURATION 40000           // ms
#define WINDOW_DURATION_MARGIN 5000     // ms
#define WINDOW_UPDATE_INTERVAL 60       // s

// PINS
#define PIN_SENS_TEMP_HUM_INTERIOR 14
#define PIN_SENS_TEMP_HUM_EXTERIOR 25
#define PIN_ACT_WINDOW_0 12
#define PIN_ACT_WINDOW_1 13
#define PIN_SENS_WINDOW_ENDSTOP 39

#define PIN_SENS_MOISTURE_ADDR_0 23
#define PIN_SENS_MOISTURE_ADDR_1 19
#define PIN_SENS_MOISTURE_ADDR_2 18
#define PIN_SENS_MOISTURE_ADDR_3 5
#define PIN_SENS_MOISTURE_ENABLE 4
#define PIN_SENS_MOISTURE_P0 33
#define PIN_SENS_MOISTURE_P1 32

#define PORT_FEEDER Serial2
#define PIN_FEEDER_TX 17
#define PIN_FEEDER_RX 16
#define PIN_ACT_FEEDER_VALVE 26
#define PIN_ACT_FEEDER_PUMP 27

#define PIN_SENS_WEBGUI_ENABLE 36
#define PIN_ACT_WEBGUI_ACTIVE 2

// FEEDER
#define FEEDER_RANGE 65123
#define FEEDER_NOZZLE_RETRACT_POS 60
#define FEEDER_NOZZLE_EXTRUDE_POS 160

// MOISTURE SENSORS
#define MS_MAX_SENSORS 12
#define MS_RESPONSE_TIMEOUT 5000
#define MS_MEASUREMENT_DURATION 5000
#define MS_UPDATE_INTERVAL  10 * 60             // s
#define MS_READINGS         10ul                // Number of measurements to take to average out noise.
#define MS_FIXED_RESISTOR   4600l               // Resistor divider fixed resistance (Ohm).
#define MS_ADC_RESOLUTION   12
#define MS_VCC_VOLT         3300l               // Sensor supply voltage (mV).
#define MS_RANGE_UPPER_VOLT 3100l               // ADC upper voltage limit (mV) for ADC_ATTEN_DB_11.
#define MS_RANGE_LOWER_VOLT 150l                // (mV)
#define MS_RANGE_VOLT       (MS_RANGE_UPPER_VOLT - MS_RANGE_LOWER_VOLT)
#define MS_RANGE_UPPER_ADC  (((MS_VCC_VOLT - MS_RANGE_LOWER_VOLT) * (1 << MS_ADC_RESOLUTION)) / MS_RANGE_VOLT) // Supply voltage (ADC units)
#define MS_RANGE_LOWER_ADC  (((0 - MS_RANGE_LOWER_VOLT) * (1 << MS_ADC_RESOLUTION)) / MS_RANGE_VOLT)             // Ground voltage in ADC units.
#define MS_RANGE_ADC        (MS_RANGE_UPPER_ADC - MS_RANGE_LOWER_ADC)



#endif