#include <Servo.h>

#define RIGHT 0
#define LEFT !RIGHT
#define RIGHT_1 RIGHT
#define RIGHT_2 !RIGHT
#define LEFT_1 LEFT
#define LEFT_2 !LEFT

#define MAX_STEPS 3000
#define POS_ERROR_MARGIN 3
#define RETRACTION_DURATION 1000            // ms

// PINS
#define MOTOR_1 12
#define MOTOR_2 11
#define ENDSTOP 3
#define STEPCOUNTER 2
#define SERVO 9

class Debounce
{
    public:
        Debounce(uint32_t pin, uint32_t delay):
            pin(pin),
            delay(delay),
            last_state_change(0)
        {}

        void init()
        {
            pinMode(pin, INPUT_PULLUP); 
            state = digitalRead(pin) ? DB_STATE::ON : DB_STATE::OFF;
        }

        void loop()
        {
            if(state == DB_STATE::OFF)
            {
                if(digitalRead(pin))
                {
                    state = DB_STATE::DB_ON;
                    last_state_change = millis();
                    // PRINT("State = DB_ON");
                }
            }
            else if(state == DB_STATE::DB_ON && (millis() - last_state_change) >= delay)
            {
                if(digitalRead(pin))
                {
                    state = DB_STATE::ON;      
                    // PRINT("State = ON");
                }
                else 
                {
                    state = DB_STATE::OFF;
                    // PRINT("State = OFF");
                }
            }
            else if(state == DB_STATE::ON)
            {
                if(!digitalRead(pin))
                {
                    state = DB_STATE::DB_OFF;
                    last_state_change = millis();
                    // PRINT("State = DB_OFF");
                }
            }
            else if(state == DB_STATE::DB_OFF && (millis() - last_state_change) >= delay)
            {
                if(!digitalRead(pin))
                {
                    state = DB_STATE::OFF;
                    // PRINT("State = OFF");
                }
                else 
                {
                    state = DB_STATE::ON;
                    // PRINT("State = ON");
                }
            }
        }

        operator bool()
        {
            return state == DB_STATE::ON || state == DB_STATE::DB_ON;
        }

    private:
        const uint32_t pin;
        const uint32_t delay;
        uint32_t last_state_change;

        enum DB_STATE {
            OFF,
            DB_ON,
            ON,
            DB_OFF
        } state;
};

Servo nozzle;
Debounce endstop(ENDSTOP, 30);

const uint32_t steps_per_revolution = 12;
uint32_t nozzle_extend_pos = 160;
uint32_t nozzle_retract_pos = 60;

unsigned long last_state_transition = 0; // Timestamp of last state transition
int32_t desired_pos = 0;      // 
volatile int32_t current_pos = 0;
uint32_t feed_duration = 0;

bool pos_unknown = false;
bool direction;
bool extend_nozzle = false;
bool abort_action = false;

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

const char* STATE_KEYS[] = {
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


template<typename... Args>
void PRINT(Args... args);

template<typename T, typename... Args>
void PRINT(T t, Args... args)
{
    Serial.print(t);
    PRINT(args...);
}

template<> void PRINT()
{
    Serial.print('\n');
}

void move_to_pos(int32_t pos, uint32_t duration = 0)
{
    // Each revolution moves the nozzle by 1.25mm
    double steps = double(pos) / 1.25 * double(steps_per_revolution);
    desired_pos = int32_t(round(steps));
    feed_duration = duration * 1000U;
    // direction = desired_pos > current_pos ? RIGHT : LEFT;
    PRINT("[Feeder] moving to: ", desired_pos);
}

uint32_t get_revolutions() { return current_pos / steps_per_revolution; }
uint32_t get_steps() { return current_pos; }

void state_transitions()
{
    if(state == STATE::IDLE)
    {
        if(desired_pos > 0)
        {
            nozzle.attach(SERVO);
            set_state(STATE::MOVE_RIGHT);
        }
        else if(desired_pos == 0 && feed_duration > 0)
        {
            nozzle.attach(SERVO);
            set_state(STATE::EXTRUDING_NOZZLE);
        }
    }

    else if(state == STATE::MOVE_RIGHT)
    {
        if(abort_action)                            set_state(STATE::RETURNING_TO_ZERO_1);
        else if((desired_pos - current_pos) < POS_ERROR_MARGIN)
        {
            // Reached desired position.
            if(feed_duration > 0)                   set_state(STATE::EXTRUDING_NOZZLE);
            else                                    set_state(STATE::WAITING);
        }
    }
    else if(state == STATE::MOVE_LEFT)
    {
        if(endstop)                                 set_state(STATE::RETURNING_TO_ZERO_2);
        else if(abort_action)                       set_state(STATE::RETURNING_TO_ZERO_1);
        else if((current_pos - desired_pos) < POS_ERROR_MARGIN)
        {
            if(feed_duration > 0)                   set_state(STATE::EXTRUDING_NOZZLE);
            else                                    set_state(STATE::WAITING);
        }
    }
    else if(state == STATE::WAITING)
    {
        if(abort_action)                            set_state(STATE::RETURNING_TO_ZERO_1);
        else if(desired_pos <= 0)                   set_state(STATE::RETURNING_TO_ZERO_1);
        else if(abs(desired_pos - current_pos)  > POS_ERROR_MARGIN)
        {
            if(desired_pos < current_pos)           set_state(STATE::MOVE_LEFT);
            else                                    set_state(STATE::MOVE_RIGHT);
        }
        else if(feed_duration > 0)                  set_state(STATE::EXTRUDING_NOZZLE);
    }
    else if(state == STATE::EXTRUDING_NOZZLE)
    {
        // Stay in this state until extend nozzle command is lifted.
        if(((millis() - last_state_transition) >= feed_duration) || abort_action) set_state(STATE::PREPARE_RETRACTION);
    }
    else if(state == STATE::PREPARE_RETRACTION)
    {
        // Indicate to main control that the valve and/or pump need to be shut off before retracting the nozzle.
        if(((millis() - last_state_transition) >= 1000)) set_state(STATE::RETRACTING_NOZZLE);
    }
    else if(state == STATE::RETRACTING_NOZZLE)
    {
        // Wait 1s to give servo time to retract the nozzle.
        if(millis() - last_state_transition >= RETRACTION_DURATION) set_state(STATE::WAITING);
    }
    else if(state == STATE::RETURNING_TO_ZERO_1)
    {
        // Move to base until endstop is pressed.
        if(endstop)                                 set_state(STATE::RETURNING_TO_ZERO_2);
    }
    else if(state == STATE::RETURNING_TO_ZERO_2)
    {
        // Move from base until endstop is released.
        if(!endstop)                                set_state(STATE::IDLE);
    }
}

void set_state(STATE newstate)
{
    if(newstate == STATE::IDLE)                     
    {
        digitalWrite(MOTOR_1, 0);       
        digitalWrite(MOTOR_2, 0);       
        current_pos = 0; 
        desired_pos = 0; 
        feed_duration = 0; 
        abort_action = false; 
        nozzle.detach();
    }
    else if(newstate == STATE::MOVE_RIGHT)         
    {
        digitalWrite(MOTOR_1, RIGHT_1); 
        digitalWrite(MOTOR_2, RIGHT_2); 
        direction = RIGHT;
    }
    else if(newstate == STATE::MOVE_LEFT)          
    {
        digitalWrite(MOTOR_1, LEFT_1);  
        digitalWrite(MOTOR_2, LEFT_2);  
        direction = LEFT;
    }
    else if(newstate == STATE::WAITING)            
    {
        digitalWrite(MOTOR_1, 0);       
        digitalWrite(MOTOR_2, 0);       
        feed_duration = 0;
    }
    else if(newstate == STATE::EXTRUDING_NOZZLE)   
    {
        digitalWrite(MOTOR_1, 0);       
        digitalWrite(MOTOR_2, 0);       
        nozzle.write(nozzle_extend_pos);
    }
    else if(newstate == STATE::PREPARE_RETRACTION) 
    {
        digitalWrite(MOTOR_1, 0);       
        digitalWrite(MOTOR_2, 0);       
    }
    else if(newstate == STATE::RETRACTING_NOZZLE)  
    {
        digitalWrite(MOTOR_1, 0);       
        digitalWrite(MOTOR_2, 0);       
        nozzle.write(nozzle_retract_pos);
    }
    else if(newstate == STATE::RETURNING_TO_ZERO_1)
    {
        digitalWrite(MOTOR_1, LEFT_1);  
        digitalWrite(MOTOR_2, LEFT_2);  
        direction = LEFT;
    }
    else if(newstate == STATE::RETURNING_TO_ZERO_2)
    {
        digitalWrite(MOTOR_1, RIGHT_1); 
        digitalWrite(MOTOR_2, RIGHT_2); 
        direction = RIGHT;
    }
    
    last_state_transition = millis();
    state = newstate;
    PRINT("[Feeder] state_verbose: ", STATE_KEYS[state]);
}

void stepcounter()
{
    if(direction == RIGHT) current_pos++;
    else current_pos--;
}

void serial_input()
{
    while(Serial.available() > 0)
    {
        char c = Serial.read();
        if(c == '\r' || c == '\n') // carriage return
        {
            // PRINT("[Serial]: processing line.");
            
            if(buffer == "[Feeder] abort")
            {
                // quit doing whatever is being done now.
                PRINT("[Feeder] aborting.");
                abort_action = true;
            }
            else if(buffer == "[Feeder] get_pos")
            {
                PRINT("[Feeder] position: ", get_steps());
            }
            else if(buffer == "[Feeder] get_state_verbose")
            {
                PRINT("[Feeder] state_verbose: ", STATE_KEYS[state]);
            }
            else if(buffer == "[Feeder] get_state")
            {
                PRINT("[Feeder] state: ", state);
            }
            else
            {
                int i = buffer.indexOf(':');
                if(i != -1)
                {
                    String key = buffer.substring(0, i);
                    String value = buffer.substring(i + 1);
                    
                    if(key == "[Feeder] set_pos")
                    {
                        if(state == STATE::IDLE || state == STATE::WAITING)
                        {
                            // Calibrate specific position by index.
                            int32_t target = int32_t(value.toInt());
                            if(target >= 0) move_to_pos(target);
                            else PRINT("[Feeder] ERROR: target must be larger than zero.");
                        }
                        else PRINT("[Feeder] ERROR: not currently in IDLE or WAITING state.");
                    }
                    else if(key == "[Feeder] feed")
                    {
                        if(state == STATE::IDLE || state == STATE::WAITING)
                        {
                            int k = value.indexOf(',');
                            if(k != -1)
                            {
                                String position = value.substring(0, k);
                                String duration = value.substring(k + 1);
                                
                                int pos = position.toInt();
                                int dur = duration.toInt();
                                if(pos >= 0 && dur >= 0) move_to_pos(pos, dur);
                                else PRINT("[Feeder] ERROR: position and duration should be larger or equal than zero.");
                            }
                            else PRINT("[Feeder] ERROR: command format incorrect.");
                        }
                        else PRINT("[Feeder] ERROR: not currently in IDLE or WAITING state.");
                    }
                    else if(key == "[Feeder] nozzle_extrude_pos")
                    {
                        uint32_t pos = uint32_t(value.toInt());
                        nozzle_extend_pos = pos;
                        PRINT("[Feeder] nozzle_extrude_pos updated.");
                    }
                    else if(key == "[Feeder] nozzle_retract_pos")
                    {
                        uint32_t pos = uint32_t(value.toInt());
                        nozzle_retract_pos = pos;
                        PRINT("[Feeder] nozzle_retract_pos updated.");
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
};

void setup()
{
	attachInterrupt(digitalPinToInterrupt(STEPCOUNTER), stepcounter, RISING);
    
    pinMode(MOTOR_1, OUTPUT);
    pinMode(MOTOR_2, OUTPUT);
    digitalWrite(MOTOR_1, 0);
    digitalWrite(MOTOR_2, 0);
    
    buffer.reserve(51);
    buffer = "";

    Serial.begin(9600);
    delay(1000);

    nozzle.attach(SERVO);
    nozzle.write(nozzle_retract_pos);
    endstop.init();

    if(!endstop) set_state(STATE::RETURNING_TO_ZERO_1);
    else set_state(STATE::IDLE);

    // PRINT("[Feeder] ready");
}

void loop()
{
	endstop.loop();
    serial_input();
    state_transitions();
}
