#ifndef TANKLEVELSENSOR_HPP
#define TANKLEVELSENSOR_HPP
#include "../debug.hpp"
#include "property.hpp"
#include "../config.h"

template<typename T>
constexpr T power(T num, int32_t exp)
{
    return (exp == 0) ? 1 : num * power(num, exp - 1);
}

class TankLevelSensor
{
    public:
        TankLevelSensor(
            RealProperty& volume,
            RealProperty& bottomlevel, 
            IntegerProperty& interval
        );
        ~TankLevelSensor(){}
        void begin(Debug& debugger);
        
        void loop();

    protected:
    
    private:
        void read();

        RealProperty& volume;
        RealProperty& bottomlevel;
        IntegerProperty& update_interval;
        Debug* debug;
        uint32_t last_update = 0;
        bool error = false;

        // Coefficients for volume calculation
        // Bottom half of double frustrum barrel model
        static constexpr float c0_0 = power(TL_BARREL_RADIUS_END, 2);
        static constexpr float c0_1 = (2.0 * TL_BARREL_RADIUS_END * (TL_BARREL_RADIUS_CENTER - TL_BARREL_RADIUS_END)) / TL_BARREL_HEIGHT;
        
        // Top half of double frustrum barrel model
        static constexpr float c1_0 = power(TL_BARREL_RADIUS_CENTER, 2);
        static constexpr float c1_1 = (-2.0 * TL_BARREL_RADIUS_CENTER * (TL_BARREL_RADIUS_CENTER - TL_BARREL_RADIUS_END)) / TL_BARREL_HEIGHT;
        
        // Last term is equal in both equations
        static constexpr float cx_2 = (4.0 * power(TL_BARREL_RADIUS_CENTER - TL_BARREL_RADIUS_END, 2)) / (3.0 * power(TL_BARREL_HEIGHT, 2));

        // Precalculated bottom half of barrel if measured level is higher than half the barrel's height.
        static constexpr float bottom_half = c0_0 * TL_BARREL_HALF + c0_1 * power(TL_BARREL_HALF, 2) + cx_2 * power(TL_BARREL_HALF, 3);       
        
};

TankLevelSensor::TankLevelSensor(RealProperty& volume, RealProperty& bottomlevel, IntegerProperty& interval):
    volume(volume),
    bottomlevel(bottomlevel),
    update_interval(interval)
{
}

void TankLevelSensor::begin(Debug &debugger = emptydebug)
{
    debug = &debugger;
    pinMode(PIN_ACT_LEVEL_TRIGGER, OUTPUT);
    pinMode(PIN_SENS_LEVEL_ECHO, INPUT);
    error = digitalRead(PIN_SENS_LEVEL_ECHO);
}

void TankLevelSensor::loop()
{
    if(millis() - last_update >= (uint32_t(update_interval.get()) * 1000ul))
    {
        last_update = millis();
        error = digitalRead(PIN_SENS_LEVEL_ECHO);
        if(!error)
        {
            debug->print("[LevelSensor]: Starting measurement.");
            read();
        }
        else
        {
            debug->print("[LevelSensor] ERROR: sensor not connected.");
        }
    }
}

void TankLevelSensor::read()
{
    // Send 10us pulse
    digitalWrite(PIN_ACT_LEVEL_TRIGGER, 1);
    delayMicroseconds(10);
    digitalWrite(PIN_ACT_LEVEL_TRIGGER, 0);

    // [BLOCKING] Wait for pulse
    uint32_t duration = pulseIn(PIN_SENS_LEVEL_ECHO, HIGH); // us
    float x = float(duration) * TL_SOUND_VELOCITY / 20000.0; // cm

    debug->print("[LevelSensor] duration (us): ", duration);
    debug->print("[LevelSensor] distance (cm): ", x);

    x = max(bottomlevel.get() - x, 0.0f);
    debug->print("[LevelSensor]    level (cm): ", x);

    float v = 0;
    if(x > TL_BARREL_HALF)
    {
        // Calculate volume of the top half of the double frustrum barrel 
        // model and add the precalculated lower half to it.
        x -= TL_BARREL_HALF;
        v = c1_0 * x + c1_1 * power(x, 2) + cx_2 * power(x, 3) + bottom_half; // cm^3
    }
    else if(x < TL_BARREL_HALF)
    {
        v = c0_0 * x + c0_1 * power(x, 2) + cx_2 * power(x, 3);
    }
    else v = bottom_half;

    // Convert cm^3 to dm^3 (L) and multiply with PI to get volume.
    v = v * PI / 1000.0;

    debug->print("[LevelSensor]    volume (L): ", v);
    volume.set(v);

}

#endif