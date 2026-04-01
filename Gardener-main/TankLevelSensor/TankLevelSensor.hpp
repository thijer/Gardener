#ifndef TANKLEVELSENSOR_HPP
#define TANKLEVELSENSOR_HPP
#include "property.hpp"
#include "../Debug/Debug.hpp"
#include "../config.h"

template<typename T>
constexpr T power(T num, int32_t exp)
{
    return (exp == 0) ? 1 : num * power(num, exp - 1);
}

class TankModel
{
    public: virtual float calc_volume(float level) = 0;
};

template<float RADIUS_END = 20.0f, float RADIUS_CENTER = 30.0f, float HEIGHT = 100.0f>
class DoubleFrustrumBarrel: public TankModel
{   
    /*
       ___________
      /           \
     /             \
    /               \
    \               /
     \             /
      \___________/
    */
    private:
        // Coefficients for volume calculation
        // Bottom frustrum of the barrel
        static constexpr float c0_0 = power(RADIUS_END, 2);
        static constexpr float c0_1 = (2.0 * RADIUS_END * (RADIUS_CENTER - RADIUS_END)) / HEIGHT;
        
        // Top frustrum of the barrel
        static constexpr float c1_0 = power(RADIUS_CENTER, 2);
        static constexpr float c1_1 = (-2.0 * RADIUS_CENTER * (RADIUS_CENTER - RADIUS_END)) / HEIGHT;
        
        // Last term is equal in both equations
        static constexpr float cx_2 = (4.0 * power(RADIUS_CENTER - RADIUS_END, 2)) / (3.0 * power(HEIGHT, 2));
        
        // Precalculated bottom half of barrel if measured level is higher than half the barrel's height.
        static constexpr float bottom_half = c0_0 * (HEIGHT / 2) + c0_1 * power((HEIGHT / 2), 2) + cx_2 * power((HEIGHT / 2), 3);

    public:
        float calc_volume(float level)
        {
            float v = 0;
            if(level > (HEIGHT / 2))
            {
                // Calculate volume of the top half of the double frustrum barrel 
                // model and add the precalculated lower half to it.
                level -= (HEIGHT / 2);
                v = c1_0 * level + c1_1 * power(level, 2) + cx_2 * power(level, 3) + bottom_half; // cm^3
            }
            else if(level < (HEIGHT / 2))
            {
                v = c0_0 * level + c0_1 * power(level, 2) + cx_2 * power(level, 3);
            }
            else v = bottom_half;

            // Convert cm^3 to dm^3 (L) and multiply with PI to get volume.
            return v * PI / 1000.0;
        }
};

template<float RADIUS_END = 20.0f, float RADIUS_CENTER = 30.0f, float HEIGHT_FRUSTRUM = 20.0f>
class SingleFrustrumBarrel: public TankModel
{
    /*
     _______________
    |               |
    |               |
    |               |
    \               /
     \             /
      \___________/
    */
    private:
        // Coefficients for volume calculation
        // Bottom frustrumed section of the barrel
        static constexpr float c0_0 = power(RADIUS_END, 2);
        static constexpr float c0_1 = (2.0 * RADIUS_END * (RADIUS_CENTER - RADIUS_END)) / HEIGHT_FRUSTRUM;
        static constexpr float c0_2 = (power(RADIUS_CENTER - RADIUS_END, 2)) / (3.0 * power(HEIGHT_FRUSTRUM, 2));

        // Upper straight section
        static constexpr float c1_0 = power(RADIUS_CENTER, 2);
        
        // Precalculated bottom half of barrel if measured level is higher than half the barrel's height.
        static constexpr float bottom_half = c0_0 * HEIGHT_FRUSTRUM + c0_1 * power(HEIGHT_FRUSTRUM, 2) + c0_2 * power(HEIGHT_FRUSTRUM, 3);

    public:
        float calc_volume(float level)
        {
            float v = 0;
            if(level > HEIGHT_FRUSTRUM)
            {
                // Calculate volume of the top half of the double frustrum barrel 
                // model and add the precalculated lower half to it.
                level -= HEIGHT_FRUSTRUM;
                v = c1_0 * level + bottom_half; // cm^3
            }
            else if(level < HEIGHT_FRUSTRUM)
            {
                v = c0_0 * level + c0_1 * power(level, 2) + c0_2 * power(level, 3);
            }
            else v = bottom_half;

            // Convert cm^3 to dm^3 (L) and multiply with PI to get volume.
            return v * PI / 1000.0;
        }
};

class TankLevelSensor
{
    public:
        TankLevelSensor(
            RealProperty& volume,
            RealProperty& bottomlevel, 
            IntegerProperty& interval
        );
        ~TankLevelSensor(){}
        void begin(Debug& debugger = emptydebug);
        
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

        SingleFrustrumBarrel<TL_BARREL_RADIUS_END, TL_BARREL_RADIUS_CENTER, TL_BARREL_FRUSTRUM_HEIGHT> barrel;
};

TankLevelSensor::TankLevelSensor(RealProperty& volume, RealProperty& bottomlevel, IntegerProperty& interval):
    volume(volume),
    bottomlevel(bottomlevel),
    update_interval(interval)
{
}

void TankLevelSensor::begin(Debug& debugger)
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

    float v = barrel.calc_volume(x);

    debug->print("[LevelSensor]    volume (L): ", v);
    volume.set(v);

}

#endif