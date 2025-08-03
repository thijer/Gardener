#ifndef MOISTURE_SENSOR_H
#define MOISTURE_SENSOR_H

#include "config.h"

uint32_t get_measurement(uint8_t address)
{
    const uint32_t vcc = 3200;
    uint32_t sample1[READINGS / 2];
    uint32_t sample2[READINGS / 2];
    double resistance[READINGS / 2];

    digitalWrite(PIN_SENS_MOISTURE_ADDR_0, (address >> 0) & 1);
    digitalWrite(PIN_SENS_MOISTURE_ADDR_1, (address >> 1) & 1);
    digitalWrite(PIN_SENS_MOISTURE_ADDR_2, (address >> 2) & 1);
    digitalWrite(PIN_SENS_MOISTURE_ADDR_3, (address >> 3) & 1);
    digitalWrite(PIN_SENS_MOISTURE_ENABLE, 0);
    
    for(uint8_t i = 0; i < READINGS / 2; i++)
    {
        pinMode(PIN_SENS_MOISTURE_P0, OUTPUT);
        pinMode(PIN_SENS_MOISTURE_P1, ANALOG);
        
        digitalWrite(PIN_SENS_MOISTURE_P0, 1);
        delayMicroseconds(50);
        sample1[i] = analogReadMilliVolts(PIN_SENS_MOISTURE_P1);
        digitalWrite(PIN_SENS_MOISTURE_P0, 0);
        
        delayMicroseconds(1000);
        
        // reverse polarization
        pinMode(PIN_SENS_MOISTURE_P0, ANALOG);
        pinMode(PIN_SENS_MOISTURE_P1, OUTPUT);
        
        digitalWrite(PIN_SENS_MOISTURE_P1, 1);
        delayMicroseconds(50);
        sample2[i] = analogReadMilliVolts(PIN_SENS_MOISTURE_P0);
        digitalWrite(PIN_SENS_MOISTURE_P1, 0);
        
        delayMicroseconds(1000);
    }
    
    digitalWrite(PIN_SENS_MOISTURE_ENABLE, 1);
    
    
    uint32_t average = 0;
    // #ifdef SERIAL_DEBUG
    //     Serial.println("Sample1\tSample2");
    // #endif
    for(uint8_t i = 0; i < READINGS / 2; i++) 
    {
        average += sample2[i] + sample1[i] + 2; // Add 1 for each measurement to avoid division by zero
        // Average += (((R1 * VCC) / Mean) - R1) * 2.0 / READINGS;
        #ifdef DEBUG
            DEBUG.print(sample1[i]);
            DEBUG.print("\t");
            DEBUG.print(sample2[i]);
            // Serial.print("\t");
            // Serial.print(Mean);
            DEBUG.println();
        #endif
    }
    #ifdef DEBUG
        DEBUG.print("Vsupply:\t");
        DEBUG.print(vcc);
        DEBUG.print("\tMoisture:\t");
        DEBUG.println(average);
    #endif


    return uint32_t((FIXED_RESISTOR * vcc) / (average / READINGS)) - uint32_t(FIXED_RESISTOR);
}



#endif