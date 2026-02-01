#ifndef FEEDER_HPP
#define FEEDER_HPP
#include "Arduino.h"
#include "config.h"
// #include "helpers.hpp"
#include "debug.hpp"
#include <deque>

// Simple structure that merges the two feeding parameters in a single variable.
class FeedCommand
{
    public:
    FeedCommand(uint32_t position, uint32_t duration):
        position(position),
        duration (duration)
        {}
    uint32_t position = 0;
    uint32_t duration = 0;
};

// FEEDER_STATES should be exactly equal to the FEEDER_STATES definition in Gardener-waterer/Gardener-waterer.ino
#define FEEDER_STATES(P) \
    P(IDLE) \
    P(WAITING) \
    P(MOVE_RIGHT) \
    P(MOVE_LEFT) \
    P(EXTRUDING_NOZZLE) \
    P(PREPARE_RETRACTION) \
    P(RETRACTING_NOZZLE) \
    P(RETURNING_TO_ZERO_1) \
    P(RETURNING_TO_ZERO_2) \
    P(ERROR)

class Feeder
{
    public:
        Feeder(
            HardwareSerial& port,
            uint8_t pin_port_tx,
            uint8_t pin_port_rx,
            IntegerProperty& nozzle_extrude_pos, 
            IntegerProperty& nozzle_retract_pos
        ):
            port(port),
            pin_port_tx(pin_port_tx),
            pin_port_rx(pin_port_rx),
            nozzle_extrude_pos(nozzle_extrude_pos),
            nozzle_retract_pos(nozzle_retract_pos)
        {}
        void loop();
        void begin(Debug& debugger = emptydebug);
        bool start_feed(uint32_t position, uint32_t duration);
        void abort();
        template<typename T, typename... Args>
        void print_to_feeder(T t, Args... args);

        void print_to_feeder();

    private:
        const uint8_t pin_port_rx;
        const uint8_t pin_port_tx;
        HardwareSerial& port;
        Debug* debug;

        IntegerProperty& nozzle_extrude_pos;
        IntegerProperty& nozzle_retract_pos;
        
        String buffer;

        enum STATE {
            FEEDER_STATES(GENERATE_STATE_ENUM)
        };
        STATE state = STATE::IDLE;
        static const uint32_t n_keys;
        static const char* const STATE_KEYS[];
        
        uint32_t feed_position = 0;
        uint32_t feed_duration = 0;
        bool active = false;
        bool command_set = false;
        
        uint32_t last_state_change = 0;
        
        std::deque<FeedCommand> command_queue;
        uint32_t last_command = 0;
        
        void serial_input();
        void parse_state(String& val);
        void set_state(STATE newstate);
};

const char* const Feeder::STATE_KEYS[] = {
    FEEDER_STATES(GENERATE_STATE_STRING)
};

const uint32_t Feeder::n_keys = sizeof(Feeder::STATE_KEYS) / sizeof(*Feeder::STATE_KEYS); 

void Feeder::abort()
{
    print_to_feeder("[Feeder] abort");
}

void Feeder::begin(Debug& debugger)
{
    debug = &debugger;
    buffer.reserve(51);
    buffer = "";
    port.begin(9600, SERIAL_8N1, pin_port_rx, pin_port_tx);
    delay(10);
    debug->print("[Feeder] started.");

    uint32_t pos = uint32_t(nozzle_extrude_pos.get());
    print_to_feeder("[Feeder] nozzle_extrude_pos:", pos);

    pos = uint32_t(nozzle_retract_pos.get());
    print_to_feeder("[Feeder] nozzle_retract_pos:", pos);
}

void Feeder::loop()
{
    serial_input();

    if(
        command_queue.size() > 0 &&                             // There are commands in the queue
        (state == STATE::IDLE || state == STATE::WAITING) &&    // The Waterer is not doing anything.
        millis() - last_command > 2000                          // The last command was sent more than 2s ago, 
        // so the Waterer had plenty of time to react to the command and let the Feeder know it is working.
    )
    {
        last_command = millis();
        FeedCommand cmd = command_queue.front();
        print_to_feeder("[Feeder] feed:", cmd.position, ",", cmd.duration);
        // No command reception verification taking place for now.
        command_queue.pop_front();
    }
    
    if(nozzle_extrude_pos.is_updated())
    {
        uint32_t pos = uint32_t(nozzle_extrude_pos.get());
        print_to_feeder("[Feeder] nozzle_extrude_pos:", pos);
    }
    if(nozzle_retract_pos.is_updated())
    {
        uint32_t pos = uint32_t(nozzle_retract_pos.get());
        print_to_feeder("[Feeder] nozzle_retract_pos:", pos);
    }
}

bool Feeder::start_feed(uint32_t position, uint32_t duration)
{
    FeedCommand cmd(position, duration);
    command_queue.push_back(cmd);
    return true;
}

void Feeder::serial_input()
{
    while(port.available() > 0)
    {
        char c = port.read();
        if(c == '\r' || c == '\n')
        {
            debug->print(buffer);
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
            STATE newstate = static_cast<STATE>(i);
            set_state(newstate);
            return;
        }
    }
    debug->print("[Feeder] ERROR: state ", val, " not found.");
}

void Feeder::set_state(STATE newstate)
{
    last_state_change = millis();
    state = newstate;
}

template<typename T, typename... Args>
void Feeder::print_to_feeder(T t, Args... args)
{
    port.print(t);
    print_to_feeder(args...);
}

void Feeder::print_to_feeder()
{
    port.print('\n');
}

#endif