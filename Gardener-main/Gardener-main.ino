#include "Arduino.h"

// Enable this definition when the existing memory on the chip needs to be wiped.
// #define WIPE_MEMORY

#define ENABLE_WEBGUI
#define ENABLE_OTA
#define ENABLE_WINDOW
#define ENABLE_FEEDER
#define ENABLE_TEMP
#define ENABLE_MOISTURE_SENSORS

#include "config.h"
#include "property.hpp"
#include "propertystore.hpp"
#include "debug.hpp"

#ifdef ENABLE_WEBGUI
#include "webinterface/webinterface.hpp"
WebInterface        webgui(WEBGUI_PORT, PIN_ACT_WEBGUI_ACTIVE);
String              webgui_buffer;
volatile bool       webgui_btn_state = 1;           // Set to 1 to let webgui_management update the current state of the switch.
volatile uint32_t   webgui_btn_ts = 0;
bool                webgui_des_state = false;
Debug               debug({&webgui, &Serial});
#else
Debug debug({&Serial});
#endif


// IMPORTANT: property keys can not exceed 15 characters.
// WINDOW CONFIG
#ifdef ENABLE_WINDOW
#include "window.hpp"
BooleanProperty  window_manual("wd_manual", false);
BooleanProperty  window_manual_pos("wd_manual_pos", false);
IntegerProperty  window_open_duration("wd_open_dur", WINDOW_DURATION);
IntegerProperty  window_duration_margin("wd_dur_margin", WINDOW_DURATION_MARGIN);
#define N_PROP_WINDOW 4
// BooleanProperty  window_switch("window_switch");
#define N_VARS_WINDOW 0
Window act_window(PIN_ACT_WINDOW_0, PIN_ACT_WINDOW_1, PIN_SENS_WINDOW_ENDSTOP, debug);
#else
#define N_PROP_WINDOW 0
#define N_VARS_WINDOW 0
#endif

// TEMPHUM SENSOR CONFIG
#ifdef ENABLE_TEMP
#include "temphum_sensor.hpp"
IntegerProperty   temp_measurement_interval("t_measure_int", TEMP_MEASUREMENT_INTERVAL);
RealProperty temp_int("temp_internal");
RealProperty hum_int("hum_internal");
RealProperty temp_ext("temp_external");
RealProperty hum_ext("hum_external");

#ifdef ENABLE_WINDOW
RealProperty     window_open_temp("wd_open_temp", TEMP_MAX);
RealProperty     window_close_temp("wd_close_temp", TEMP_MIN);
IntegerProperty  window_update_interval("wd_update_int", WINDOW_UPDATE_INTERVAL);
#define N_PROP_TEMP 4
#else
#define N_PROP_TEMP 1
#endif

#define N_VARS_TEMP 4

TempHumSensor th_interior(PIN_SENS_TEMP_HUM_INTERIOR, &temp_int, &hum_int);
TempHumSensor th_exterior(PIN_SENS_TEMP_HUM_EXTERIOR, &temp_ext, &hum_ext);
#else
#define N_PROP_TEMP 0
#define N_VARS_TEMP 0
#endif

// MOISTURE SENSOR CONFIG
#ifdef ENABLE_MOISTURE_SENSORS
#include "moisture_sensor.hpp"
IntegerProperty moisture_measurement_interval("ms_meas_int", MS_UPDATE_INTERVAL);
IntegerProperty moisture_sensor_0("ms_0");
IntegerProperty moisture_sensor_1("ms_1");
#define N_PROP_MOISTURE 1
#define N_VARS_MOISTURE 2

MoistureSensorArray moisture_sensors(
    PIN_SENS_MOISTURE_ENABLE,
    PIN_SENS_MOISTURE_ADDR_0,
    PIN_SENS_MOISTURE_ADDR_1,
    PIN_SENS_MOISTURE_ADDR_2,
    PIN_SENS_MOISTURE_ADDR_3,
    PIN_SENS_MOISTURE_P0,
    PIN_SENS_MOISTURE_P1,
    {
        {&moisture_sensor_0, 0},
        {&moisture_sensor_1, 1}
    },
    &moisture_measurement_interval
);
#else
#define N_PROP_MOISTURE 0
#define N_VARS_MOISTURE 0
#endif

// FEEDER CONFIG
#ifdef ENABLE_FEEDER
#include "feeder.hpp"

IntegerProperty feeder_nozzle_retract_pos("fd_nz_retr", FEEDER_NOZZLE_RETRACT_POS);
IntegerProperty feeder_nozzle_extrude_pos("fd_nz_extr", FEEDER_NOZZLE_EXTRUDE_POS);
#define N_PROP_FEEDER 2
#define N_VARS_FEEDER 0

Feeder act_feeder(PORT_FEEDER, PIN_FEEDER_TX, PIN_FEEDER_RX, debug);

bool start_feed(uint32_t position, uint32_t duration)
{
    return act_feeder.start_feed(position, duration);
}
#else
#define N_PROP_FEEDER 0
#define N_VARS_FEEDER 0
#endif

// MEASURED VARIABLES
#define N_PROP (N_PROP_WINDOW + N_PROP_TEMP + N_PROP_MOISTURE + N_PROP_FEEDER)
PropertyStore<N_PROP> properties({
#ifdef ENABLE_WINDOW
    &window_manual, 
    &window_manual_pos,
    &window_open_duration,
    &window_duration_margin,
#endif
#if defined(ENABLE_WINDOW) && defined(ENABLE_TEMP)
    &window_update_interval,
    &window_open_temp,
    &window_close_temp,
#endif
#ifdef ENABLE_TEMP
    &temp_measurement_interval,
#endif
#ifdef ENABLE_WEBGUI
#endif
#ifdef ENABLE_MOISTURE_SENSORS
    &moisture_measurement_interval,
#endif
#ifdef ENABLE_FEEDER
    &feeder_nozzle_extrude_pos,
    &feeder_nozzle_retract_pos,
#endif
});

#define N_VARS (N_VARS_WINDOW + N_VARS_TEMP + N_VARS_MOISTURE + N_VARS_FEEDER)
TelemetryStore<N_VARS> variables({
#ifdef ENABLE_TEMP
    &temp_int, 
    &hum_int,
    &temp_ext, 
    &hum_ext,
#endif
#ifdef WINDOW
    // &window_switch
#endif
#ifdef ENABLE_MOISTURE_SENSORS
    &moisture_sensor_0,
    &moisture_sensor_1,
#endif
});

// DEVICES

String serial_buffer;

// const size_t l = sizeof(uint);

void setup()
{
    Serial.begin(115200);
    delay(5000);
    debug.print("[Gardener] Starting.");
    serial_buffer.reserve(51);

    #ifdef ENABLE_WEBGUI
    pinMode(PIN_SENS_WEBGUI_ENABLE, INPUT); // 36 does not have an internal pullup.
    attachInterrupt(PIN_SENS_WEBGUI_ENABLE, webgui_button_isr, CHANGE);

    webgui_buffer.reserve(51);
    bool success = webgui.begin(&debug);
    debug.print("[WebGUI] ", success ? "configured." : "ERROR: failed to set up webserver.");
    #endif

    
    properties.begin();
    
    #ifdef ENABLE_MOISTURE_SENSORS
    analogReadResolution(MS_ADC_RESOLUTION);
    moisture_sensors.begin(debug);
    #endif

    #ifdef ENABLE_WINDOW
    // Assign properties to sensors and actuators
    act_window.property_window_opening_duration(&window_open_duration);
    act_window.property_window_duration_margin(&window_duration_margin);
    // act_window.telemetry_window_endstop(&window_switch);
    
    act_window.begin();
    #endif

    #ifdef ENABLE_TEMP
    th_interior.property_reading_interval(&temp_measurement_interval);
    th_exterior.property_reading_interval(&temp_measurement_interval);
    th_interior.begin();
    th_exterior.begin();
    #endif

    #ifdef ENABLE_FEEDER
    act_feeder.set_properties(feeder_nozzle_extrude_pos, feeder_nozzle_retract_pos);
    act_feeder.begin();
    #endif

    debug.print("[Gardener] Ready.");

}

void loop()
{
    #ifdef ENABLE_TEMP
    th_interior.loop();
    th_exterior.loop();
    #endif

    #ifdef ENABLE_MOISTURE_SENSORS
    moisture_sensors.loop();
    #endif

    serial_input();
    #ifdef ENABLE_WEBGUI
    websocket_input();
    webgui_management();
    #endif

    #ifdef ENABLE_WINDOW
    decision_window();
    act_window.loop();
    #endif

    #ifdef ENABLE_FEEDER
    act_feeder.loop();
    #endif
    
    properties.save();
    // delay(1);
}

void parse_command(String& message)
{
    int i = message.indexOf('=');
    int j = message.indexOf(':');
    if(i != -1)
    {
        String key = message.substring(0, i);
        String value = message.substring(i + 1);
        // if(key == "plant")  plants.config_plant(value);
        properties.apply_setting(key, value);
    }
    else if(j != -1)
    {
        String key = message.substring(0, j);
        String value = message.substring(j + 1);
        /* #ifdef ENABLE_MOISTURE_SENSORS
        if(key == "read")
        {
            int addr = value.toInt();
            uint val = get_measurement(addr);
            debug.print("[Moisture] sensor resistance:", val);
        }
        #endif */

        #ifdef ENABLE_FEEDER
        if(key == "[Feeder] feed")
        {
            int k = value.indexOf(',');
            if(k != -1)
            {
                String position = value.substring(0, k);
                String duration = value.substring(k + 1);
                
                int pos = position.toInt();
                int dur = duration.toInt();
                if(pos >= 0 && dur >= 0)
                {
                    bool res = act_feeder.start_feed(pos, dur);
                    debug.print("[Feeder] ", res ? "starting feed" : "ERROR: feed already active.");
                }

            }
        }
        #endif
        
    }
    #ifdef ENABLE_FEEDER
    else if(message == "[Feeder] abort")
    {
        act_feeder.abort();
    }
    else if(message == "[Feeder] get_pos")
    {
        act_feeder.print_to_feeder("[Feeder] get_pos");
    }
    #endif
    else if(message == "print")
    {
        properties.print_to(debug);
    }
    else if(message == "print_tel") variables.print_to(debug);

    #ifdef ENABLE_WEBGUI
    else if(message == "[WebGUI] start")
    {
        webgui.start();
    }
    else if(message == "[WebGUI] end")
    {
        webgui.stop();
    }
    #endif
}

void serial_input()
{
    while(Serial.available() > 0)
    {
        char c = Serial.read();
        if(c == '\r' || c == '\n') // carriage return
        {
            debug.print("[Serial] processing line.");
            parse_command(serial_buffer);
            serial_buffer.clear();
        }
        else
        {
            serial_buffer += c;
        }
    }
}

#ifdef ENABLE_WEBGUI
void websocket_input()
{
    while(webgui.available() > 0)
    {
        char c = webgui.read();
        if(c == '\r' || c == '\n') // carriage return
        {
            debug.print("[WebGUI] processing line.");
            debug.print(webgui_buffer);
            parse_command(webgui_buffer);
            webgui_buffer.clear();
        }
        else
        {
            webgui_buffer += c;
        }
    }
}

void webgui_button_isr()
{
    webgui_btn_ts = millis();
    webgui_btn_state = true;
    detachInterrupt(PIN_SENS_WEBGUI_ENABLE);
}

void webgui_management()
{
    if(webgui_btn_state && (millis() - webgui_btn_ts) > 30)
    {
        webgui_btn_state = 0;
        attachInterrupt(PIN_SENS_WEBGUI_ENABLE, webgui_button_isr, CHANGE);
        webgui_des_state = digitalRead(PIN_SENS_WEBGUI_ENABLE);
        debug.print("[WebGUI] button pressed: ", webgui_des_state);
        if(!webgui_des_state && !webgui.running())
        {
            webgui.start();
        }
        else if(webgui_des_state && webgui.running())
        {
            webgui.stop();
        }
    }
    
}

/* void webgui_management()
{
    bool state = webgui_button.loop();
    // FALLING
    if(state != webgui_btn_state)
    {
        webgui_btn_state = state;
        if(!state)
        {
            debug.print("[WebGUI] button pressed.");
            bool des_state = webgui_des_state.get();
            webgui_des_state.set(!des_state);
        }
    }
    if(webgui_des_state.get() && !webgui.running())
    {
        webgui.start();
    }
    else if(!webgui_des_state.get() && webgui.running())
    {
        debug.print("[WebGUI] disabled by property.");
        webgui.stop();
        webgui_des_state.set(false);
    }
    else if(webgui.no_reason_to_live())
    {
        debug.print("[WebGUI] no reason to live.");
        webgui.stop();
        webgui_des_state.set(false);
    }
}
 */
#endif

#ifdef ENABLE_WINDOW
void decision_window()
{
    #ifdef ENABLE_TEMP
    static ulong last_check = 0;
    
    // Manually control window, using the 'window_manual_pos' property.
    if(window_manual || th_interior.get_error() || th_exterior.get_error())
    {
        if(window_manual_pos != act_window.get_position())
        {
            bool pos = window_manual_pos.get();
            debug.print("[Window]: ", pos ? "Opening" : "Closing");
            act_window.set_position(pos);
        }
    }
    else if(window_update_interval <= (millis() - last_check))
    {
        last_check = millis();
        // if temperature in greenhouse is higher than the upper limit.
        if(window_open_temp < temp_int.get() && !act_window.get_position())
        {
            debug.print("[Window]: Opening");
            act_window.set_position(true);      // open window
        }
        else if(window_close_temp > temp_int.get() && act_window.get_position())
        {
            debug.print("[Window]: Closing");
            act_window.set_position(false);
        }
    }
    #else
    if(window_manual_pos != act_window.get_position())
    {
        bool pos = window_manual_pos.get();
        debug.print("[Window]: ", pos ? "Opening" : "Closing");
        act_window.set_position(pos);
    }
    #endif
}
#endif

/* void decision_feeder()
{
    uint looper = 0;
    if(waterer.idle())
    {
        plant& p = plants[looper];
        if(p.updated && p.reading > p.threshold)
        {
            waterer.water_plant(p);
            p.updated = false;
        }
        looper++;
    }
}
 */
