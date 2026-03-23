#include "Arduino.h"

// Enable this definition when the existing memory on the chip needs to be wiped.
// #define WIPE_MEMORY

#define ENABLE_THINGSBOARD
#define ENABLE_WEBGUI
#define ENABLE_OTA
#define ENABLE_WINDOW
#define ENABLE_FEEDER
#define ENABLE_TEMP
#define ENABLE_MOISTURE_SENSORS
#define ENABLE_LEVEL_SENSOR
#define ENABLE_WATERINGRULES
// #define ENABLE_WATERING_FIXED       // enable fixed quantity watering logic.
// #define ENABLE_WATERING_MOISTURE    // enable moisture controlled watering logic.

// Disable WEBGUI if thingsboard is enabled
#ifdef ENABLE_THINGSBOARD
    #undef ENABLE_WEBGUI
    #include "ArduinoJson.h"            // Include ArduinoJson before properties to ensure properties compile with ArduinoJson support.

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
        #define N_DEV_MOISTURE 12
    #else
        #define N_DEV_MOISTURE 0
    #endif
#endif

// Disable hard-coded logic
#ifdef ENABLE_WATERINGRULES
    #ifndef ENABLE_FEEDER           // Depends on feeder presence.
        #undef ENABLE_WATERINGRULES
    #else
        #undef ENABLE_WATERING_FIXED
        #undef ENABLE_WATERING_MOISTURE
    #endif
#endif

// Disable watering logic if the Feeder is unavailable.
#if defined(ENABLE_WATERING_FIXED) && !defined(ENABLE_FEEDER)
#undef ENABLE_WATERING_FIXED
#endif

// Disable moisture-based watering logic if the Feeder and moisture sensors are notavailable.
#if defined(ENABLE_WATERING_MOISTURE) && !(defined(ENABLE_FEEDER) && defined(ENABLE_MOISTURE_SENSORS))
#undef ENABLE_WATERING_MOISTURE
#endif


#include "config.h"
#include "property.hpp"
#include "propertystore.hpp"
#include "PropertyMemory.hpp"
#include "PropertyTextInterface.hpp"
#include "debug.hpp"

#ifdef ENABLE_THINGSBOARD
#include "WiFi.h"
#include "WiFiClient.h"
#include "WiFiManager/WiFiManager.hpp"
#include "DebugWebsocket/DebugWebsocket.hpp"
#include "tb_credentials.h"
#include "ThingGateway.hpp"

bool       tb_switch_flipped = 1;    // Set to 1 to to read the current switch state during startup.
uint32_t   tb_switch_ts = 0;
bool       tb_switch_state = false;


#define TB_DEVICES 1 + (N_DEV_MOISTURE + N_DEV_WATERINGRULES + N_DEV_PROPERTYRULES)

time_t timesource()
{
    time_t time_now;
    time(&time_now);
    return time_now * 1000ll; // Convert seconds to milliseconds with this sophisticated conversion.
}

WiFiManager manager(SSID, WPA2PSK);
WiFiClient client;
ThingGateway<TB_DEVICES> tb_gateway(client, tb_server, tb_accesstoken, "Gardener-gateway");
ThingDevice tb_device("Gardener", "Gardener-control");
DebugWebsocket socket(DEBUGSOCKET_PORT);
String         socket_buffer;
#endif

#ifdef ENABLE_WEBGUI
#include "webinterface/webinterface.hpp"
WebInterface        webgui(WEBGUI_PORT, PIN_ACT_WEBGUI_ACTIVE);
String              webgui_buffer;
volatile bool       webgui_btn_state = 1;           // Set to 1 to let webgui_management update the current state of the switch.
volatile uint32_t   webgui_btn_ts = 0;
bool                webgui_des_state = false;
#endif

#if defined(ENABLE_WEBGUI)
Debug debug({&webgui, &Serial});
#elif defined(ENABLE_THINGSBOARD)
Debug debug({&socket, &Serial});
#else
Debug debug({&Serial});
#endif

#ifdef ENABLE_OTA
#include "ArduinoOTA.h"
#include "ota_credentials.h"
void cb_ota_error(ota_error_t error);
void cb_ota_start();
void cb_ota_progress(uint32_t progress, uint32_t total);
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
Window act_window(PIN_ACT_WINDOW_0, PIN_ACT_WINDOW_1, PIN_SENS_WINDOW_ENDSTOP, window_open_duration, window_duration_margin);
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

TempHumSensor th_interior(PIN_SENS_TEMP_HUM_INTERIOR, temp_int, hum_int, temp_measurement_interval);
TempHumSensor th_exterior(PIN_SENS_TEMP_HUM_EXTERIOR, temp_ext, hum_ext, temp_measurement_interval);
#else
#define N_PROP_TEMP 0
#define N_VARS_TEMP 0
#endif

// TANK LEVEL SENSOR CONFIG
#ifdef ENABLE_LEVEL_SENSOR
#include "TankLevelSensor/TankLevelSensor.hpp"
RealProperty tl_volume("tank_volume");
RealProperty tl_bottomlevel("tank_b_level");
IntegerProperty tl_interval("tank_meas_int");

TankLevelSensor tanklevel(tl_volume, tl_bottomlevel, tl_interval);
#define N_PROP_LEVEL 2
#define N_VARS_LEVEL 1
#else
#define N_PROP_LEVEL 0
#define N_VARS_LEVEL 0
#endif 
 
// MOISTURE SENSOR CONFIG
#ifdef ENABLE_MOISTURE_SENSORS
#include "moisture_sensors/moisture_sensor_interface.hpp"
IntegerProperty moisture_measurement_interval("ms_meas_int", MS_UPDATE_INTERVAL);

#ifdef ENABLE_THINGSBOARD
#include "moisture_sensors/moisture_sensor_device.hpp"
#else
#include "moisture_sensors/moisture_sensor.hpp"
#endif
#define N_VARS_MOISTURE 12
#define N_PROP_MOISTURE 13

MoistureSensor moisture_sensor_00("m_sens_00",  0);
MoistureSensor moisture_sensor_01("m_sens_01",  1);
MoistureSensor moisture_sensor_02("m_sens_02",  2, false);
MoistureSensor moisture_sensor_03("m_sens_03",  3, false);
MoistureSensor moisture_sensor_04("m_sens_04",  4, false);
MoistureSensor moisture_sensor_05("m_sens_05",  5, false);
MoistureSensor moisture_sensor_06("m_sens_06",  6, false);
MoistureSensor moisture_sensor_07("m_sens_07",  7, false);
MoistureSensor moisture_sensor_08("m_sens_08",  8, false);
MoistureSensor moisture_sensor_09("m_sens_09",  9, false);
MoistureSensor moisture_sensor_10("m_sens_10", 10, false);
MoistureSensor moisture_sensor_11("m_sens_11", 11, false);

MoistureSensorInterface moisture_sensors(
    PIN_SENS_MOISTURE_ENABLE,
    PIN_SENS_MOISTURE_ADDR_0,
    PIN_SENS_MOISTURE_ADDR_1,
    PIN_SENS_MOISTURE_ADDR_2,
    PIN_SENS_MOISTURE_ADDR_3,
    PIN_SENS_MOISTURE_P0,
    PIN_SENS_MOISTURE_P1,
    {
        &moisture_sensor_00,
        &moisture_sensor_01,
        &moisture_sensor_02,
        &moisture_sensor_03,
        &moisture_sensor_04,
        &moisture_sensor_05,
        &moisture_sensor_06,
        &moisture_sensor_07,
        &moisture_sensor_08,
        &moisture_sensor_09,
        &moisture_sensor_10,
        &moisture_sensor_11
    },
    moisture_measurement_interval
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

Feeder act_feeder(PORT_FEEDER, PIN_FEEDER_TX, PIN_FEEDER_RX, feeder_nozzle_extrude_pos, feeder_nozzle_retract_pos);

bool start_feed(uint32_t position, uint32_t duration)
{
    return act_feeder.start_feed(position, duration);
}
#else
#define N_PROP_FEEDER 0
#define N_VARS_FEEDER 0
#endif

#ifdef ENABLE_WATERINGRULES
#include "WateringRules/WateringRules.hpp"

WateringRule section_l0("section_l0", "", 86400,   0, false);
WateringRule section_l1("section_l1", "", 86400,  25, false);
WateringRule section_l2("section_l2", "", 86400,  50, false);
WateringRule section_r0("section_r0", "", 86400,  75, false);
WateringRule section_r1("section_r1", "", 86400, 100, false);
WateringRule section_r2("section_r2", "", 86400, 125, false);

WateringRuleEngine engine(
    act_feeder,
    {
        &section_l0,
        &section_l1,
        &section_l2,
        &section_r0,
        &section_r1,
        &section_r2
    },
    debug
);

#ifdef ENABLE_THINGSBOARD
#include "ThingRuleEngine/ThingRuleEngine.hpp"
ThingRuleEngine tb_engine(engine, "Watering rule engine", "rule-engine");
#endif
// simple hack to prevent the compiler throwing a tantrum when there are as many comma's as input 
// arguments to `RuleEngine::set_variables`
IntegerProperty rule_engine_dummy("dummy");
#endif

#ifdef ENABLE_PROPERTYRULES
#include "PropertyRuleEngine/PropertyRuleEngine.hpp"
PropertyRuleEngine prop_engine;

#ifdef ENABLE_THINGSBOARD
#include "ThingRuleEngine/ThingRuleEngine.hpp"
ThingRuleEngine tb_prop_engine(prop_engine, "Property rule engine", "rule-engine");
#endif
#ifndef ENABLE_WATERINGRULES
IntegerProperty rule_engine_dummy("dummy");
#endif
#endif

#ifdef ENABLE_WATERING_FIXED
#include "wateringlogic/fixedquantities.hpp"
IntegerProperty gr0_feed_quantity("gr0_fd_quantity", 360);      // 6 min
IntegerProperty gr0_timeout("gr0_timeout", 24*60*60);           // daily
#define N_PROP_WATERING_FIXED 2
#define N_VARS_WATERING_FIXED 0
FixedQuantity group0("group0", 25, gr0_feed_quantity, gr0_timeout, act_feeder);
#else
#define N_PROP_WATERING_FIXED 0
#define N_VARS_WATERING_FIXED 0
#endif

#ifdef ENABLE_WATERING_MOISTURE
#include "wateringlogic/moisturelevels.hpp"
IntegerProperty gr1_feed_quantity("gr1_fd_quantity", 360);      // 6 min
IntegerProperty gr1_timeout("gr1_timeout", 24*60*60);           // daily
IntegerProperty gr1_threshold("gr1_threshold", 10000);          // 10 kOhm
#define N_PROP_WATERING_MOISTURE 3
#define N_VARS_WATERING_MOISTURE 0
MoistureLevels group1("group1", 50, gr1_feed_quantity, gr1_timeout, gr1_threshold, act_feeder);
#else
#define N_PROP_WATERING_MOISTURE 0
#define N_VARS_WATERING_MOISTURE 0
#endif


// MEASURED VARIABLES
#define N_PROP (N_PROP_WINDOW + N_PROP_TEMP + N_PROP_LEVEL + N_PROP_MOISTURE + N_PROP_FEEDER + N_PROP_WATERING_FIXED + N_PROP_WATERING_MOISTURE)
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
#ifdef ENABLE_LEVEL_SENSOR
    &tl_bottomlevel,
    &tl_interval,
#endif
#ifdef ENABLE_WEBGUI
#endif
#ifdef ENABLE_MOISTURE_SENSORS
    &moisture_measurement_interval,
    moisture_sensor_00.get_enabler(),
    moisture_sensor_01.get_enabler(),
    moisture_sensor_02.get_enabler(),
    moisture_sensor_03.get_enabler(),
    moisture_sensor_04.get_enabler(),
    moisture_sensor_05.get_enabler(),
    moisture_sensor_06.get_enabler(),
    moisture_sensor_07.get_enabler(),
    moisture_sensor_08.get_enabler(),
    moisture_sensor_09.get_enabler(),
    moisture_sensor_10.get_enabler(),
    moisture_sensor_11.get_enabler(),
#endif
#ifdef ENABLE_FEEDER
    &feeder_nozzle_extrude_pos,
    &feeder_nozzle_retract_pos,
#endif
#ifdef ENABLE_WATERING_FIXED
    &gr0_feed_quantity,
    &gr0_timeout,
#endif
#ifdef ENABLE_WATERING_MOISTURE
    &gr1_feed_quantity,
    &gr1_timeout,
    &gr1_threshold,
#endif
});

#define N_VARS (N_VARS_WINDOW + N_VARS_TEMP + N_VARS_LEVEL + N_VARS_MOISTURE + N_VARS_FEEDER + N_VARS_WATERING_FIXED + N_VARS_WATERING_MOISTURE)
PropertyStore<N_VARS> variables({
#ifdef ENABLE_TEMP
    &temp_int, 
    &hum_int,
    &temp_ext, 
    &hum_ext,
#endif
#ifdef ENABLE_LEVEL_SENSOR
    &tl_volume,
#endif
#ifdef WINDOW
    // &window_switch
#endif
#ifdef ENABLE_MOISTURE_SENSORS
    moisture_sensor_00.get_moisture(),
    moisture_sensor_01.get_moisture(),
    moisture_sensor_02.get_moisture(),
    moisture_sensor_03.get_moisture(),
    moisture_sensor_04.get_moisture(),
    moisture_sensor_05.get_moisture(),
    moisture_sensor_06.get_moisture(),
    moisture_sensor_07.get_moisture(),
    moisture_sensor_08.get_moisture(),
    moisture_sensor_09.get_moisture(),
    moisture_sensor_10.get_moisture(),
    moisture_sensor_11.get_moisture(),
#endif
});

PropertyMemory memory(properties);
PropertyTextInterface prop_interface(properties);
PropertyTextInterface vars_interface(variables);
String serial_buffer;

void setup()
{
    Serial.begin(115200);
    delay(5000);
    debug.print("[Gardener] Starting.");
    serial_buffer.reserve(51);
    
    #ifdef ENABLE_THINGSBOARD
    pinMode(PIN_SENS_WEBGUI_ENABLE, INPUT); // 36 does not have an internal pullup.
    manager.begin(debug);

    debug.print("[Gardener] configuring Thingsboard.");
    tb_device.add_shared_attributes(properties);
    tb_device.add_telemetry(variables);

    tb_gateway.add_devices({
        &tb_device,
        #ifdef ENABLE_MOISTURE_SENSORS
        &moisture_sensor_00,
        &moisture_sensor_01,
        &moisture_sensor_02,
        &moisture_sensor_03,
        &moisture_sensor_04,
        &moisture_sensor_05,
        &moisture_sensor_06,
        &moisture_sensor_07,
        &moisture_sensor_08,
        &moisture_sensor_09,
        &moisture_sensor_10,
        &moisture_sensor_11,
        #endif
        #if defined(ENABLE_WATERINGRULES)
        &tb_engine,
        #endif
        #if defined(ENABLE_PROPERTYRULES)
        &tb_prop_engine,
        #endif
    });
    tb_gateway.add_timesource(timesource);
    tb_gateway.begin();
    // debug.print("[Gardener] Thingsboard ", tb_gateway.connected() ? "connected." : "failure.");
    socket.begin();
    #endif

    #ifdef ENABLE_WEBGUI
    pinMode(PIN_SENS_WEBGUI_ENABLE, INPUT); // 36 does not have an internal pullup.
    attachInterrupt(PIN_SENS_WEBGUI_ENABLE, webgui_button_isr, CHANGE);

    webgui_buffer.reserve(51);
    bool success = webgui.begin(debug);
    debug.print("[WebGUI] ", success ? "configured." : "ERROR: failed to set up webserver.");
    #endif

    #ifdef ENABLE_OTA
    // OTA callbacks
    ArduinoOTA.onError(cb_ota_error);
    ArduinoOTA.onStart(cb_ota_start);
    ArduinoOTA.onProgress(cb_ota_progress);
    ArduinoOTA.setPasswordHash(passwordhash);
    ArduinoOTA.begin();
    #endif

    memory.begin();
    
    #ifdef ENABLE_MOISTURE_SENSORS
    moisture_sensors.begin(debug);
    #ifdef ENABLE_THINGSBOARD
    moisture_sensor_00.begin();
    moisture_sensor_01.begin();
    moisture_sensor_02.begin();
    moisture_sensor_03.begin();
    moisture_sensor_04.begin();
    moisture_sensor_05.begin();
    moisture_sensor_06.begin();
    moisture_sensor_07.begin();
    moisture_sensor_08.begin();
    moisture_sensor_09.begin();
    moisture_sensor_10.begin();
    moisture_sensor_11.begin();
    #endif
    #endif

    #ifdef ENABLE_WINDOW
    act_window.begin(debug);
    #endif

    #ifdef ENABLE_TEMP
    th_interior.begin(debug);
    th_exterior.begin(debug);
    #endif

    #ifdef ENABLE_LEVEL_SENSOR
    tanklevel.begin(debug);
    #endif

    #ifdef ENABLE_FEEDER
    act_feeder.begin(debug);
    #endif

    #ifdef ENABLE_WATERINGRULES
    engine.set_variables(
    #ifdef ENABLE_TEMP
        &temp_int, 
        &hum_int,
        &temp_ext, 
        &hum_ext,
    #endif
    #ifdef ENABLE_LEVEL_SENSOR
        &tl_volume,
    #endif
    #ifdef WINDOW
        // &window_switch
    #endif
    #ifdef ENABLE_MOISTURE_SENSORS
        moisture_sensor_00.get_moisture(),
        moisture_sensor_01.get_moisture(),
        moisture_sensor_02.get_moisture(),
        moisture_sensor_03.get_moisture(),
        moisture_sensor_04.get_moisture(),
        moisture_sensor_05.get_moisture(),
        moisture_sensor_06.get_moisture(),
        moisture_sensor_07.get_moisture(),
        moisture_sensor_08.get_moisture(),
        moisture_sensor_09.get_moisture(),
        moisture_sensor_10.get_moisture(),
        moisture_sensor_11.get_moisture(),
    #endif
        &rule_engine_dummy
    );
    engine.begin();
    #endif

    #ifdef ENABLE_PROPERTYRULES
    prop_engine.begin(debug);
    prop_engine.set_variables(
    #ifdef ENABLE_TEMP
        &temp_int, 
        &hum_int,
        &temp_ext, 
        &hum_ext,
    #endif
    #ifdef ENABLE_LEVEL_SENSOR
        &tl_volume,
    #endif
    #ifdef WINDOW
        // &window_switch
    #endif
    #ifdef ENABLE_MOISTURE_SENSORS
        moisture_sensor_00.get_moisture(),
        moisture_sensor_01.get_moisture(),
        moisture_sensor_02.get_moisture(),
        moisture_sensor_03.get_moisture(),
        moisture_sensor_04.get_moisture(),
        moisture_sensor_05.get_moisture(),
        moisture_sensor_06.get_moisture(),
        moisture_sensor_07.get_moisture(),
        moisture_sensor_08.get_moisture(),
        moisture_sensor_09.get_moisture(),
        moisture_sensor_10.get_moisture(),
        moisture_sensor_11.get_moisture(),
    #endif
        &rule_engine_dummy
    );
    #endif

    #ifdef ENABLE_WATERING_FIXED
    group0.begin(debug);
    #endif

    #ifdef ENABLE_WATERING_MOISTURE
    group1.add_sensors(
        {
            {moisture_sensor_00.get_moisture(), 1.0},
            {moisture_sensor_01.get_moisture(), 0.5}
        }
    );
    group1.begin(debug);
    #endif

    debug.print("[Gardener] Ready.");

}

void loop()
{
    #ifdef ENABLE_TEMP
    th_interior.loop();
    th_exterior.loop();
    #endif

    #ifdef ENABLE_LEVEL_SENSOR
    tanklevel.loop();
    #endif

    #ifdef ENABLE_MOISTURE_SENSORS
    moisture_sensors.loop();
    moisture_sensor_00.loop();
    moisture_sensor_01.loop();
    moisture_sensor_02.loop();
    moisture_sensor_03.loop();
    moisture_sensor_04.loop();
    moisture_sensor_05.loop();
    moisture_sensor_06.loop();
    moisture_sensor_07.loop();
    moisture_sensor_08.loop();
    moisture_sensor_09.loop();
    moisture_sensor_10.loop();
    moisture_sensor_11.loop();
    #endif

    serial_input();

    #ifdef ENABLE_THINGSBOARD
    tb_management();
    manager.loop();
    if(manager.connected())
    {
        tb_device.loop();
        tb_gateway.loop();
    }
    socket.loop();
    websocket_input();
    #endif

    #ifdef ENABLE_WEBGUI
    websocket_input();
    webgui_management();
    #endif

    #ifdef ENABLE_OTA
    ArduinoOTA.handle();
    #endif
    
    #ifdef ENABLE_WINDOW
    decision_window();
    act_window.loop();
    #endif

    #ifdef ENABLE_FEEDER
    act_feeder.loop();
    #endif

    #ifdef ENABLE_WATERINGRULES
    engine.loop();
    tb_engine.loop();
    #endif

    #ifdef ENABLE_PROPERTYRULES
    prop_engine.loop();
    tb_prop_engine.loop();
    #endif

    #ifdef ENABLE_WATERING_FIXED
    group0.loop();
    #endif

    #ifdef ENABLE_WATERING_MOISTURE
    group1.loop();
    #endif

    memory.save();
    // delay(1);
}

void parse_command(String& message)
{
    int i = message.indexOf('=');
    int j = message.indexOf(':');
    if(i != -1 && j == -1)
    {
        String key = message.substring(0, i);
        String value = message.substring(i + 1);
        // if(key == "plant")  plants.config_plant(value);
        prop_interface.apply_setting(key, value);
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
        
        #ifdef ENABLE_WATERINGRULES
        // {"test_rule":{"expression":"IF(((0.5 * m_sens_00) + (1.2 * m_sens_01)) / (0.5 + 1.2) > 1000, 360, 0)", "eval_interval": 10, "feeder_address": 25, "enabled": true}}
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, message);
        if(err)
        {
            debug.print("[Serial] ERROR parsing JSON.");
            debug.print(err.c_str());
        }
        else
        {
            debug.print("[Serial] processing rule.");
            engine.process_attributes(doc.as<JsonObject>());
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
        prop_interface.print_to(debug);
    }
    else if(message == "print_tel") vars_interface.print_to(debug);

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

    #ifdef ENABLE_THINGSBOARD
    if(message == "wifi")
    {
        debug.print("RSSI:   ", WiFi.RSSI());
        debug.print("Status: ", WiFi.status());
        /* 
        WL_NO_SHIELD = 255,
        WL_STOPPED = 254,
        WL_IDLE_STATUS = 0,
        WL_NO_SSID_AVAIL = 1,
        WL_SCAN_COMPLETED = 2,
        WL_CONNECTED = 3,
        WL_CONNECT_FAILED = 4,
        WL_CONNECTION_LOST = 5,
        WL_DISCONNECTED = 6 
        */
    }
    #endif

    #ifdef ENABLE_WATERINGRULES
    if(message == "rules")
    {
        engine.print();
    }
    #endif
    #ifdef ENABLE_PROPERTYRULES
    if(message == "rules")
    {
        prop_engine.print();
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

#ifdef ENABLE_THINGSBOARD
void tb_management()
{
    if(digitalRead(PIN_SENS_WEBGUI_ENABLE) != tb_switch_state && !tb_switch_flipped)
    {
        tb_switch_flipped = true;
        tb_switch_ts = millis();
    }
    else if(tb_switch_flipped && (millis() - tb_switch_ts) > 30ul)
    {
        tb_switch_flipped = 0;
        tb_switch_state = digitalRead(PIN_SENS_WEBGUI_ENABLE);
        debug.print("[Gardener] switch flipped: ", tb_switch_state);
        
        if(!tb_switch_state) tb_gateway.enable();
        else if(tb_switch_state) tb_gateway.disable();
    }
}

void websocket_input()
{
    while(socket.available() > 0)
    {
        char c = socket.read();
        if(c == '\r' || c == '\n') // carriage return
        {
            debug.print("[DebugSocket] processing line.");
            debug.print(socket_buffer);
            parse_command(socket_buffer);
            socket_buffer.clear();
        }
        else
        {
            socket_buffer += c;
        }
    }
}

#endif

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

#endif

#ifdef ENABLE_OTA
void cb_ota_error(ota_error_t error)
{
    if (error == OTA_AUTH_ERROR)
    {
        debug.print("[OTA] ERROR: Auth Failed");
    }
    else if (error == OTA_BEGIN_ERROR)
    {
        debug.print("[OTA] ERROR: Begin Failed");
    }
    else if (error == OTA_CONNECT_ERROR)
    {
        debug.print("[OTA] ERROR: Connect Failed");
    }
    else if (error == OTA_RECEIVE_ERROR)
    {
        debug.print("[OTA] ERROR: Receive Failed");
    } 
    else if (error == OTA_END_ERROR)
    {
        debug.print("[OTA] ERROR: End Failed");
    }
}

void cb_ota_start()
{
    debug.print("[OTA] Starting update.");
}

void cb_ota_progress(uint32_t progress, uint32_t total)
{
    debug.print("[OTA] Progress: ", (progress * 100) / total);
}
#endif

#ifdef ENABLE_WINDOW
void decision_window()
{
    #ifdef ENABLE_TEMP
    static uint32_t last_check = 0;
    
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
    else if((uint32_t(window_update_interval.get()) * 1000ul) <= (millis() - last_check))
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

