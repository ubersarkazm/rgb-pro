/*
Copyright 2016 Ben Bair (bbairgames@gmail.com)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

Description:
Simple constant PWM driver for RGB LEDs (design for Arduino Pro Mini)
Fully controlled with 3 buttons and no UI (besides the PWM outputs)
*/

#include <EEPROM.h>

//EEPROM data write offset
#define EEPROM_DATA_OFFSET 0

//Function prototypes
void SaveState();
void LoadState();

//R,G,B
const int NUM_COLORS = 3;

//Time constants for button hold operations
const int HOLD_TIME_CLEAR = 1500;
const int HOLD_TIME_SAVE = 1000;

//This delay determines flash speed for a save operation
const int BLINK_DELAY = 100;

//Amount to use when incrementing PWM value
const int PWM_INCREMENT = 16;

//Input pin mappings
const int INPUTS[NUM_COLORS] = { 10, 11, 12 };

//Output pin mappings
const int OUTPUTS[NUM_COLORS] = { 3, 5, 6 };

//Current PWM values
int Values[NUM_COLORS] = { 0 };

//PWM values last update
int lastValues[NUM_COLORS] = { 0 };

//Current button states (pressed, not pressed)
int curButtonStates[NUM_COLORS] = { LOW };

//Previous button states
int lastButtonStates[NUM_COLORS] = { LOW };

//Flags indicating which buttons are held down
int buttonsHeld[NUM_COLORS] = { 0 };

//Button press timestamps for debouncing
long lastDebounceTimes[NUM_COLORS] = { 0 };

//Debounce delay in milliseconds
long debounceDelay = 30;

void setup()
{
    //Set the pin modes
    for (int i = 0; i < NUM_COLORS; ++i) {
        pinMode(INPUTS[i], INPUT);
        pinMode(OUTPUTS[i], OUTPUT);
    }

    //Write all PWM outputs to zero (off)
    for (int i = 0; i < NUM_COLORS; ++i) {
        analogWrite(OUTPUTS[i], 0);
    }

    //Load saved state from EEPROM
    LoadState();

    //Apply saved state
    for (int i = 0; i < NUM_COLORS; ++i) {
        analogWrite(OUTPUTS[i], Values[i]);
    }
}

void loop()
{
    unsigned long ts = millis();

    //Check channels one at a time
    for (int i = 0; i < NUM_COLORS; ++i) {
        int reading = digitalRead(INPUTS[i]);
        if (reading != lastButtonStates[i]) {
            lastDebounceTimes[i] = ts;
        }

        if ((ts - lastDebounceTimes[i]) > debounceDelay) {
            //Check for button state changes
            if (reading != curButtonStates[i]) {
                curButtonStates[i] = reading;

                //Increase the current channel value when button is pressed and released quickly
                if (curButtonStates[i] == LOW && buttonsHeld[i] == 0) {
                    if (Values[i] == 255)
                        Values[i] = 0;
                    else
                        Values[i] += PWM_INCREMENT;

                    Values[i] = min(Values[i], 255);
                }

                buttonsHeld[i] = 0;
            }
        }

        //If a button is held longer than HOLD_TIME_CLEAR, zero its channel
        //and indicate it was held down (to avoid incrementing above)
        if (curButtonStates[i] == HIGH && ts - lastDebounceTimes[i] > HOLD_TIME_CLEAR) {
            Values[i] = 0;
            buttonsHeld[i] = 1;
        }

        lastButtonStates[i] = reading;
    }

    //Apply the current state if there were any changes
    for (int i = 0; i < NUM_COLORS; ++i) {
        if (Values[i] != lastValues[i]) {
            analogWrite(OUTPUTS[i], Values[i]);
            lastValues[i] = Values[i];
        }
    }

    //If all 3 buttons are held for 1 second, save current state to EEPROM
    if (curButtonStates[0] == HIGH &&
        curButtonStates[1] == HIGH &&
        curButtonStates[2] == HIGH &&
        ts - lastDebounceTimes[0] > HOLD_TIME_SAVE &&
        ts - lastDebounceTimes[1] > HOLD_TIME_SAVE &&
        ts - lastDebounceTimes[2] > HOLD_TIME_SAVE)
    {
        //flash all channels to let user know save op happened
        for (int i = 0; i < NUM_COLORS; ++i) {
            analogWrite(OUTPUTS[i], 0);
        }
        delay(BLINK_DELAY);
        for (int i = 0; i < NUM_COLORS; ++i) {
            analogWrite(OUTPUTS[i], Values[i]);
        }
        delay(BLINK_DELAY);
        for (int i = 0; i < NUM_COLORS; ++i) {
            analogWrite(OUTPUTS[i], 0);
        }
        delay(BLINK_DELAY);

        for (int i = 0; i < NUM_COLORS; ++i) {
            analogWrite(OUTPUTS[i], Values[i]);
        }

        buttonsHeld[0] = buttonsHeld[1] = buttonsHeld[2] = 1;
        SaveState();
    }
}

//Save the global state (Values[]) to EEPROM
void SaveState()
{
    for (int i = 0; i < NUM_COLORS; i++)
    {
        EEPROM.write(i + EEPROM_DATA_OFFSET, Values[i]);
    }
}

//Load the global state (Values[]) from EEPROM
void LoadState()
{
    for (int i = 0; i < NUM_COLORS; i++)
    {
        Values[i] = lastValues[i] = EEPROM.read(i + EEPROM_DATA_OFFSET);
    }
}
