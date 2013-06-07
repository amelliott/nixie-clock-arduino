#include <Time.h>
#include <IntervalTimer.h>
#include "ClockDisplay.h"
#include "NixieClockDisplay.h"
#include "Encoder.h"
#include "Button.h"

#define INCREMENT_INCREASE_BEGIN_THRESHOLD 1500 // ms between 0's to change increment value
#define INCREMENT_INCREASE_END_THRESHOLD 300
#define INCREMENT_MAX 10
#define INCREMENT_MIN 1

#define SECS_IN_A_DAY 86400

#define DATE_LED 2

ClockDisplay * clockDisplay;
Encoder * setEncoder;
Button * setButton;
Button * dateButton;
IntervalTimer displayRefresh;

byte inTimeMode = 1;
byte inSetMode = 0;
byte timeIncrement = INCREMENT_MIN;
unsigned long lastZeroPassed = 4294967295;

time_t getTeensy3Time()
{
  return Teensy3Clock.get();
}

void setup(){
  // set clock provider
  setSyncProvider(getTeensy3Time);
  setSyncInterval(10);
  // set up display
  clockDisplay = new NixieClockDisplay(10, 11, 12, 9);
  // start refresh timer
  displayRefresh.begin(updateDisplay, 1000000);
  // register encoder & buttons
  setEncoder = EncoderManager::registerEncoder(4, 5);
  setButton = ButtonManager::registerButton(0, 25, 0, &onSetButtonPressed);
  dateButton = ButtonManager::registerButton(1, 50, 0, &onDateButtonPressed);
  // set up date button LED
  pinMode(DATE_LED, OUTPUT);
  turnOffDateLED();
  
  if (year() == 1970)
    setTime(hour(), minute(), second(), day(), month(), 2013);
  
  updateDisplay();
}

void updateDisplay() {
  if (inTimeMode)
    clockDisplay->displayTime();
  else
    clockDisplay->displayDate();
}

void loop() {
}

void updateTimeIncrement()
{
  if ((minute() % 10) == 0)
  {
    unsigned long time = millis();
    // check if need to increase
    if (timeIncrement == INCREMENT_MIN)
    {
      if ((time - lastZeroPassed) < INCREMENT_INCREASE_BEGIN_THRESHOLD)
        timeIncrement = INCREMENT_MAX;
    }
    else
    {
      if ((time - lastZeroPassed) > INCREMENT_INCREASE_END_THRESHOLD)
        timeIncrement = INCREMENT_MIN;
    }
    lastZeroPassed = time;
  }
}

void setTimeAdjustment(unsigned int adjustment)
{
  Teensy3Clock.set(now()+adjustment);
  setTime(now()+adjustment);
  updateDisplay();
}

void decreaseTime()
{
  updateTimeIncrement();
  setTimeAdjustment((-timeIncrement*60) - second());
}

void increaseTime()
{
  updateTimeIncrement();
  setTimeAdjustment(timeIncrement*60 - second());
}

void decreaseDate()
{
  setTimeAdjustment(-SECS_IN_A_DAY);
}

void increaseDate()
{
  setTimeAdjustment(SECS_IN_A_DAY);
}

void onSetButtonPressed()
{
  if (inSetMode)
  {
    Serial.println("Display mode");
    // deregister encoder interrupts
    setEncoder->onLeft = NULL;
    setEncoder->onRight = NULL;
    inSetMode = 0;
    // if setting time, set seconds = 0
    if (inTimeMode)
      setTimeAdjustment(-second());
    clockDisplay->stopBlink();
    updateDisplay();
  }
  else
  {
    Serial.println("Set mode");
    // register encoder interrupts
    if (inTimeMode)
    {
      setEncoder->onLeft = &decreaseTime;
      setEncoder->onRight = &increaseTime;
    }
    else
    {
      setEncoder->onLeft = &decreaseDate;
      setEncoder->onRight = &increaseDate;
    }
    inSetMode = 1;
    clockDisplay->startBlink();
    updateDisplay();
  }
}

void turnOnDateLED()
{
  analogWrite(DATE_LED, 128);
}

void turnOffDateLED()
{
  analogWrite(DATE_LED, 0);
}

void onDateButtonPressed()
{
  Serial.println("Date button pressed");
  // ignore it if in set mode
  if (inSetMode)
    return;
  if (inTimeMode)
  {
    // switch to date mode
    inTimeMode = 0;
    turnOnDateLED();
  }
  else
  {
    inTimeMode = 1;
    turnOffDateLED();
  }
  updateDisplay();
}
