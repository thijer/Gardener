#ifndef SERVOVALVE_HPP
#define SERVOVALVE_HPP

#include <Servo.h>

#define POS_OPEN 135
#define POS_CLOSED 45
#define POS_OVERSHOOT 20

class ServoValve: private Servo
{
    public:
        ServoValve(uint8_t pin);
        ServoValve(uint8_t pin, uint8_t pos_open, uint8_t pos_closed, uint8_t pos_overshoot);
        void begin() {};
        void loop();
        void open()  { desired_state = 1; };
        void close() { desired_state = 0; };
    protected:

    private:
        uint8_t pin;
        uint8_t pos_open;
        uint8_t pos_closed;
        uint8_t pos_overshoot;

        uint32_t last_state = 0;

        bool desired_state = 0; // 0 = closed, 1 = open.
        enum STATE {
            UNKNOWN,
            CLOSED,
            OPENING,
            OPEN,
            CLOSING
        } state = UNKNOWN;
};

ServoValve::ServoValve(uint8_t pin):
    pin(pin),
    pos_open(POS_OPEN),
    pos_closed(POS_CLOSED),
    pos_overshoot(POS_OVERSHOOT)
{}

ServoValve::ServoValve(uint8_t pin, uint8_t pos_open, uint8_t pos_closed, uint8_t pos_overshoot):
    pin(pin),
    pos_open(pos_open),
    pos_closed(pos_closed),
    pos_overshoot(pos_overshoot)
{}

void ServoValve::loop()
{
    if(state == STATE::UNKNOWN)
    {
        last_state = millis();
        state = STATE::CLOSING;
        Servo::attach(pin);
        Servo::write(pos_closed - pos_overshoot);
    }

    else if(state == STATE::CLOSED)
    {
        if(desired_state)
        {
            last_state = millis();
            state = STATE::OPENING;
            Servo::attach(pin);
            Servo::write(pos_open + pos_overshoot);        
        }
    }
    
    else if(state == STATE::OPENING)
    {
        if(millis() - last_state >= 1000)
        {
            last_state = millis();
            state = STATE::OPEN;
            Servo::detach();
        }
    }
    
    else if(state == STATE::OPEN)
    {
        if(!desired_state)
        {
            last_state = millis();
            state = STATE::CLOSING;
            Servo::attach(pin);
            Servo::write(pos_closed - pos_overshoot);        
        }
    }
    
    else if(state == STATE::CLOSING)
    {
        if(millis() - last_state >= 1000)
        {
            last_state = millis();
            state = STATE::CLOSED;
            Servo::detach();   
        }
    }
    
    
}




#endif