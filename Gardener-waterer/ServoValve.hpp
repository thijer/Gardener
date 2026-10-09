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
        ServoValve(uint8_t pin, int32_t pos_open, int32_t pos_closed, int32_t pos_overshoot = 0);
        void begin() {};
        void loop();
        void open()  { desired_state = 1; };
        void close() { desired_state = 0; };
        void set_params(int32_t open_pos, int32_t closed_pos, int32_t overshoot = 0) { pos_open = open_pos; pos_closed = closed_pos; pos_overshoot = overshoot; }
    protected:

    private:
        uint8_t pin;
        int32_t pos_open;       // Open position (deg)
        int32_t pos_closed;     // Closed position (deg)
        int32_t pos_overshoot;  // Number of degrees to overshoot open or close setpoints to compensate for servo not reaching those angles.

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

ServoValve::ServoValve(uint8_t pin, int32_t pos_open, int32_t pos_closed, int32_t pos_overshoot):
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