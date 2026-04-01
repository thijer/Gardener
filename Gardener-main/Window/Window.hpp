#ifndef WINDOW_HPP
#define WINDOW_HPP

#include "property.hpp"
#include "../Debug/Debug.hpp"
#include "../config.h"

#define WINDOW_STATES(P) \
    P(ERROR) \
    P(STARTUP) \
    P(CLOSED) \
    P(OPENING_1) \
    P(OPENING_2) \
    P(OPENED) \
    P(CLOSING) \
    P(HALTED) 

class Window
{
    public:
        enum STATE 
        {
            WINDOW_STATES(GENERATE_STATE_ENUM)
        };

        Window(uint8_t ctrl_1, uint8_t ctrl_2, uint8_t endstop, IntegerProperty& open_duration, IntegerProperty& duration_margin):
            pin_ctrl_1(ctrl_1),
            pin_ctrl_2(ctrl_2),
            pin_endstop(endstop),
            window_open_duration(open_duration),
            window_duration_margin(duration_margin)
        {};

        bool begin(Debug& debugger = emptydebug);
        void loop();
        void set_position(bool pos) { set_position(window_open_duration.get(), pos); }
        void set_position(uint32_t duration, bool direction);
        bool get_position() { return open_window; }
    
    private:
        static const char* const STATE_KEYS[]; 
            
        bool open_window = false;       // Desired state of the window, open = true, closed = false.
        STATE state = STATE::CLOSING;
    
        const uint8_t pin_ctrl_1;
        const uint8_t pin_ctrl_2;
        const uint8_t pin_endstop;
        uint32_t last_state_change;
        uint32_t open_duration = 0;
        bool direction = true;
        IntegerProperty& window_open_duration;
        IntegerProperty& window_duration_margin;
        // BooleanProperty* window_endstop;
        Debug* debug;

        void set_state(STATE new_state);

};

const char* const Window::STATE_KEYS[] = {
    WINDOW_STATES(GENERATE_STATE_STRING)
};

bool Window::begin(Debug& debugger)
{
    debug = &debugger;
    
    pinMode(pin_ctrl_1, OUTPUT);
    pinMode(pin_ctrl_2, OUTPUT);
    pinMode(pin_endstop, INPUT);

    digitalWrite(pin_ctrl_1, 0);
    digitalWrite(pin_ctrl_2, 0);

    debug->print("[Window]: Initialised.");
    set_state(STATE::CLOSING);
    return true;
}

void Window::set_position(uint32_t duration, bool direction)
{
    open_window = direction;
    open_duration = duration;
}

void Window::set_state(STATE new_state)
{
    if(new_state == STATE::STARTUP)
    {
        // Shouldn't occur
    }
    else if(new_state == STATE::CLOSING)
    {
        digitalWrite(PIN_ACT_WINDOW_0, 1);
        digitalWrite(PIN_ACT_WINDOW_1, 0);
    }
    else if(new_state == STATE::CLOSED)
    {
        digitalWrite(PIN_ACT_WINDOW_0, 0);
        digitalWrite(PIN_ACT_WINDOW_1, 0);
    }
    else if(new_state == STATE::OPENING_1)
    {
        digitalWrite(PIN_ACT_WINDOW_0, 0);
        digitalWrite(PIN_ACT_WINDOW_1, 1);
    }
    else if(new_state == STATE::OPENING_2)
    {
        digitalWrite(PIN_ACT_WINDOW_0, 0);
        digitalWrite(PIN_ACT_WINDOW_1, 1);
    }
    else if(new_state == STATE::OPENED)
    {
        digitalWrite(PIN_ACT_WINDOW_0, 0);
        digitalWrite(PIN_ACT_WINDOW_1, 0);
    }
    else if(new_state == STATE::HALTED)
    {
        digitalWrite(PIN_ACT_WINDOW_0, 0);
        digitalWrite(PIN_ACT_WINDOW_1, 0);
    }
    last_state_change = millis();
    state = new_state;
    debug->print("[Window] state: ", STATE_KEYS[new_state]);
}

void Window::loop()
{
    if(state == STATE::STARTUP)
    {
        if(!digitalRead(pin_endstop))       set_state(STATE::CLOSED);
        else                                set_state(STATE::CLOSING);
    }
    else if(state == STATE::CLOSING)
    {
        // if(open_window)                     set_state(STATE::HALTED);
        if(
            !digitalRead(pin_endstop) ||
            (millis() - last_state_change) >= uint32_t(window_open_duration.get() + window_duration_margin.get())
        )                                   set_state(STATE::CLOSED);
    }
    else if(state == STATE::CLOSED)
    {
        if(open_window)                     set_state(STATE::OPENING_1);
    }
    else if(state == STATE::OPENING_1)
    {
        if(!open_window)                    set_state(STATE::HALTED);
        else if(digitalRead(pin_endstop))   set_state(STATE::OPENING_2);
    }
    else if(state == STATE::OPENING_2)
    {
        if(!open_window)                    set_state(STATE::HALTED);
        else if((millis() - last_state_change) >= open_duration)   set_state(STATE::OPENED);
    }
    
    else if(state == STATE::OPENED)
    {
        if(!open_window)                    set_state(STATE::CLOSING);
    }
    else if(state == STATE::HALTED)
    {
        if(open_window)                     set_state(STATE::OPENING_1);
        else                                set_state(STATE::CLOSING);
    }
}

#endif