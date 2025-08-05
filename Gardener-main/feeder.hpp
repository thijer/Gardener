#ifndef FEEDER_HPP
#define FEEDER_HPP
#include "Arduino.h"
// #include "helpers.hpp"
#include "debug.hpp"

class Feeder
{
    public:
        Feeder(
            HardwareSerial& port,
            uint8_t pin_port_tx,
            uint8_t pin_port_rx,
            uint8_t pin_act_valve, 
            uint8_t pin_act_pump, 
            Debug& output
        ):
            port(port),
            pin_port_tx(pin_port_tx),
            pin_port_rx(pin_port_rx),
            pin_act_valve(pin_act_valve),
            pin_act_pump(pin_act_pump),
            debug(output)
        {}
        void loop();
        void begin();
        bool start_feed(uint32_t position, uint32_t duration);
        void abort();
        template<typename T, typename... Args>
        void print_to_feeder(T t, Args... args);

        void print_to_feeder();

    private:
        const uint8_t pin_act_valve;
        const uint8_t pin_act_pump;
        const uint8_t pin_port_rx;
        const uint8_t pin_port_tx;
        HardwareSerial& port;
        Debug& debug;

        String buffer;

        enum STATE {
            IDLE,
            WAITING,
            MOVE_RIGHT,
            MOVE_LEFT,
            EXTRUDING_NOZZLE,
            PREPARE_RETRACTION,
            RETRACTING_NOZZLE,
            RETURNING_TO_ZERO_1,
            RETURNING_TO_ZERO_2,
            ERROR
        };
        STATE state = STATE::IDLE;
        
        const char* STATE_KEYS[10] = {
            "IDLE",
            "WAITING",
            "MOVE_RIGHT",
            "MOVE_LEFT",
            "EXTRUDING_NOZZLE",
            "PREPARE_RETRACTION",
            "RETRACTING_NOZZLE",
            "RETURNING_TO_ZERO_1",
            "RETURNING_TO_ZERO_2",
            "ERROR"
        };

        const uint32_t n_keys = sizeof(STATE_KEYS) / sizeof(*STATE_KEYS);

        uint32_t feed_position = 0;
        uint32_t feed_duration = 0;
        bool active = false;
        bool command_set = false;

        ulong last_state_change = 0;
        
        // template<typename... Args>
        // void print_to_feeder(Args... args);

        
        void serial_input();
        void parse_state(String& val);
        void set_state(STATE newstate);
        void state_transitions();
};

void Feeder::abort()
{
    digitalWrite(pin_act_pump, 1);
    digitalWrite(pin_act_valve, 1);
    print_to_feeder("[Feeder] abort");
}

void Feeder::begin()
{
    // pinMode(pin_port_en, OUTPUT);
    pinMode(pin_act_valve, OUTPUT);
    pinMode(pin_act_pump, OUTPUT);
    digitalWrite(pin_act_pump, 1);
    digitalWrite(pin_act_valve, 1);
    
    buffer.reserve(51);
    buffer = "";
    port.begin(9600, SERIAL_8N1, pin_port_rx, pin_port_tx);
    debug.print("[Feeder] started.");
}

void Feeder::loop()
{
    serial_input();
    state_transitions();
}

bool Feeder::start_feed(uint32_t position, uint32_t duration)
{
    if(active) return false;
    feed_position = position;
    feed_duration = duration;
    command_set = true;
    return true;
}

void Feeder::serial_input()
{
    while(port.available() > 0)
    {
        // debug.print("[Feeder] received stuff");
        char c = port.read();
        if(c == '\r' || c == '\n') // carriage return
        {
            debug.print(buffer);
            if(false) ;
            else
            {
                int i = buffer.indexOf(':');
                if(i != -1)
                {
                    String key = buffer.substring(0, i);
                    String value = buffer.substring(i + 2);
                    
                    if(key == "[Feeder] state_verbose")
                    {
                        parse_state(value);
                    }
                    else if(key == "[Feeder] state")
                    {
                        int s = value.toInt();
                        if(i >= 0 && i < n_keys)
                        {
                            set_state(static_cast<STATE>(i));
                        }
                    }
                }
            }
            buffer = "";
        }
        else
        {
            buffer += c;
        }
    }
}

void Feeder::parse_state(String& val)
{
    for(uint32_t i = 0; i < n_keys; i++)
    {
        if(val == STATE_KEYS[i])
        {
            // debug.print("[Feeder] state ", val, " found.");
            STATE newstate = static_cast<STATE>(i);
            set_state(newstate);
            return;
        }
    }
    debug.print("[Feeder] ERROR: state ", val, " not found.");
}

void Feeder::set_state(STATE newstate)
{
    if(newstate == STATE::EXTRUDING_NOZZLE)
    {
        digitalWrite(pin_act_valve, 0);
        digitalWrite(pin_act_pump, 0);    
    }
    else if(newstate == STATE::WAITING)
    {
        feed_position = 0;
        feed_duration = 0;
        active = false;
        command_set = false;
        digitalWrite(pin_act_pump, 1);
        digitalWrite(pin_act_valve, 1);
    }
    else if(newstate == STATE::PREPARE_RETRACTION)
    {
        digitalWrite(pin_act_pump, 1);
        digitalWrite(pin_act_valve, 1);
    }
    else if(newstate == STATE::IDLE)
    {
        feed_position = 0;
        feed_duration = 0;
        active = false;
        command_set = false;
        digitalWrite(pin_act_pump, 1);
        digitalWrite(pin_act_valve, 1);
    }
    else if(
        newstate == STATE::RETRACTING_NOZZLE || 
        newstate == STATE::RETURNING_TO_ZERO_1 || 
        newstate == STATE::RETURNING_TO_ZERO_2
    ){
        // active = false;
        digitalWrite(pin_act_pump, 1);
        digitalWrite(pin_act_valve, 1);
    }
    last_state_change = millis();
    state = newstate;
    // debug.print("[Feeder] newstate is ", state);
            
}

void Feeder::state_transitions()
{
    if(state == STATE::IDLE)
    {
        if(!active && command_set)
        {
            debug.print("[Feeder] command sent.");
            print_to_feeder("[Feeder] feed: ", feed_position, ",", feed_duration);
            active = true;
        }
    }
    else if(state == STATE::MOVE_RIGHT)
    {
        
    }
    else if(state == STATE::MOVE_LEFT)
    {
        
    }
    else if(state == STATE::WAITING)
    {
        if(!active && command_set)
        {
            print_to_feeder("[Feeder] feed: ", feed_position, ",", feed_duration);
            active = true;
        }
        else if(((millis() - last_state_change) >= (1000 * 60)) && !active)
        {
            // Timeout, return to base
            active = true;
            print_to_feeder("[Feeder] abort");
        }
    }
    else if(state == STATE::EXTRUDING_NOZZLE)
    {
       
    }
    else if(state == STATE::RETRACTING_NOZZLE)
    {

    }
    else if(state == STATE::RETURNING_TO_ZERO_1)
    {
        
    }
    else if(state == STATE::RETURNING_TO_ZERO_2)
    {
        
    }
}

template<typename T, typename... Args>
void Feeder::print_to_feeder(T t, Args... args)
{
    port.print(t);
    print_to_feeder(args...);
}

void Feeder::print_to_feeder()
{
    port.println();
}

#endif