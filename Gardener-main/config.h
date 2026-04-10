#ifndef CONFIG_H
#define CONFIG_H

// STATE MACHINE MACROS
#define GENERATE_STATE_ENUM(STATE) STATE,       // Generates STATE entries for enums
#define GENERATE_STATE_STRING(STATE) #STATE,    // Generates STATE entries for a c string array.

// THINGSBOARD
#ifdef GARDENER_TEST
#define TB_GARDENER_CONTROL_NAME "Test-gardener-control"
#define TB_GARDENER_GATEWAY_NAME "Test-gardener-gateway"
#define TB_GARDENER_WATERINGRULEENGINE_NAME "Test-watering-rule-engine"
#define TB_GARDENER_PROPERTYRULEENGINE_NAME "Test-property-rule-engine"
#else
#define TB_GARDENER_CONTROL_NAME "Gardener-control"
#define TB_GARDENER_GATEWAY_NAME "Gardener-gateway"
#define TB_GARDENER_WATERINGRULEENGINE_NAME "Watering-rule-engine"
#define TB_GARDENER_PROPERTYRULEENGINE_NAME "Property-rule-engine"
#endif

// WIFI
#define WIFI_AP_DISCONNECT_DELAY 3000ul

// WEBGUI
#define WEBGUI_PORT 80

// DEBUGSOCKET
#define DEBUGSOCKET_PORT 80

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

#define PIN_SENS_WEBGUI_ENABLE 36
#define PIN_ACT_WEBGUI_ACTIVE 2

#define PIN_SENS_LEVEL_ECHO 35
#define PIN_ACT_LEVEL_TRIGGER 2

// TANK LEVEL SENSOR
#define TL_UPDATE_INTERVAL 60           // s
#define TL_SOUND_VELOCITY 343.0         // sound velocity at 20deg C (m/s)
#define TL_BARREL_RADIUS_END 17.91f     // Barrel bottom radius (cm)
#define TL_BARREL_RADIUS_CENTER 25.32f  // Barrel center (bilge) and top radius (cm)
#define TL_BARREL_FRUSTRUM_HEIGHT 25.0f // Height of the lower frustrum section of the barrel (cm)

// FEEDER
#define FEEDER_NOZZLE_RETRACT_POS 60
#define FEEDER_NOZZLE_EXTRUDE_POS 160

// MOISTURE SENSORS
#define MS_MAX_SENSORS      12
#define MS_UPDATE_INTERVAL  10 * 60             // s
#define MS_READINGS         10ul                // Number of measurements to take to average out noise.
#define MS_FIXED_RESISTOR   4640l               // Resistor divider fixed resistance (Ohm).
#define MS_ADC_RESOLUTION   12
#define MS_VCC_VOLT         3300l               // Sensor supply voltage (mV).
#define MS_RANGE_UPPER_VOLT 3100l               // ADC upper voltage limit (mV) for ADC_ATTEN_DB_11.
#define MS_RANGE_LOWER_VOLT 150l                // (mV)
#define MS_RANGE_VOLT       (MS_RANGE_UPPER_VOLT - MS_RANGE_LOWER_VOLT)
#define MS_RANGE_UPPER_ADC  (((MS_VCC_VOLT - MS_RANGE_LOWER_VOLT) * (1 << MS_ADC_RESOLUTION)) / MS_RANGE_VOLT) // Supply voltage (ADC units)
#define MS_RANGE_LOWER_ADC  (((0 - MS_RANGE_LOWER_VOLT) * (1 << MS_ADC_RESOLUTION)) / MS_RANGE_VOLT)             // Ground voltage in ADC units.
#define MS_RANGE_ADC        (MS_RANGE_UPPER_ADC - MS_RANGE_LOWER_ADC)

#ifdef GARDENER_TEST
#define MS_ENABLED false                // Disable moisture sensors in Thingsboard to prevent feeding random data to production sensors with the same name.  
#else
#define MS_ENABLED true
#endif
#define MS_NAME(i) "m_sens_" #i     // Ensure name remains compatible with TinyExpr++ variable naming conventions.

// DEPENDENCY RESOLUTION

#ifdef ENABLE_THINGSBOARD
    #undef ENABLE_WEBGUI                // Disable WEBGUI if thingsboard is enabled
    #undef ENABLE_WIFI_AP               // Disable WiFi accesspoint if defined.
    #define ENABLE_WIFI_STA             // Enable WiFi station mode.
    #include "ArduinoJson.h"            // Include ArduinoJson before properties to ensure properties compile with ArduinoJson support.

    // Enumerate enabled devices
    #ifdef ENABLE_WATERINGRULES
        #define N_DEV_WATERINGRULES 1
    #else
        #define N_DEV_WATERINGRULES 0
    #endif
    #ifdef ENABLE_PROPERTYRULES
        #define N_DEV_PROPERTYRULES 1
    #else
        #define N_DEV_PROPERTYRULES 0
    #endif
    #ifdef ENABLE_MOISTURE_SENSORS
        #define N_DEV_MOISTURE MS_MAX_SENSORS
    #else
        #define N_DEV_MOISTURE 0
    #endif

#endif

// Disable hard-coded logic
#ifdef ENABLE_WATERINGRULES
    #ifndef ENABLE_FEEDER               // Depends on feeder presence.
        #undef ENABLE_WATERINGRULES
    #else
        #undef ENABLE_WATERING_FIXED    // Disable conflicting systems
        #undef ENABLE_WATERING_MOISTURE
    #endif
#endif

// Disable watering logic if the Feeder is unavailable.
#if defined(ENABLE_WATERING_FIXED) && !defined(ENABLE_FEEDER)
#undef ENABLE_WATERING_FIXED
#endif

// Disable moisture-based watering logic if the Feeder and moisture sensors are not available.
#if defined(ENABLE_WATERING_MOISTURE) && !(defined(ENABLE_FEEDER) && defined(ENABLE_MOISTURE_SENSORS))
#undef ENABLE_WATERING_MOISTURE
#endif

#define TB_DEVICES 1 + (N_DEV_MOISTURE + N_DEV_WATERINGRULES + N_DEV_PROPERTYRULES)

// If wifi is enabled, ...
#ifdef ENABLE_WIFI
    // enable wifi station mode if mode is not specified
    #if !defined(ENABLE_WIFI_STA) && !defined(ENABLE_WIFI_AP)
    #define ENABLE_WIFI_STA
    #endif
#endif

#if defined(ENABLE_WIFI_STA)
    #define WIFI_SSID WIFI_STA_SSID
    #define WIFI_WPA2PSK WIFI_STA_WPA2PSK
    #define ENABLE_WIFI
#elif defined(ENABLE_WIFI_AP)
    #define WIFI_SSID WIFI_AP_SSID
    #define WIFI_WPA2PSK WIFI_AP_WPA2PSK
    #define ENABLE_WIFI
#endif

#endif