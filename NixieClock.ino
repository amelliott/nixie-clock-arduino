#include <Time.h>
#include <IntervalTimer.h>
#include "NixieClockDisplay.h"
#include "Encoder.h"
#include "Button.h"

#define INCREMENT_INCREASE_BEGIN_THRESHOLD 1500 // ms between 0's to change increment value
#define INCREMENT_INCREASE_END_THRESHOLD 300
#define INCREMENT_MAX 10
#define INCREMENT_MIN 1

#define SECS_IN_A_DAY 86400

#define DATE_LED_PIN 3
#define DISPLAY_BUTTON_PIN 2
#define DISPLAY_LED_PIN 6
#define SET_BUTTON_PIN 0
#define DATE_BUTTON_PIN 1
#define SET_ENCODER_PIN_A 4
#define SET_ENCODER_PIN_B 5

ClockDisplay * clockDisplay;
Encoder * setEncoder;
Button * setButton;
Button * dateButton;
Button * displayButton;
IntervalTimer displayRefresh;

byte inTimeMode = 1;
byte inSetMode = 0;
byte displayOn = 1;
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
  setEncoder = EncoderManager::registerEncoder(SET_ENCODER_PIN_A, SET_ENCODER_PIN_B);
  setButton = ButtonManager::registerButton(SET_BUTTON_PIN, 25, 0, &onSetButtonPressed);
  dateButton = ButtonManager::registerButton(DATE_BUTTON_PIN, 50, 0, &onDateButtonPressed);
  displayButton = ButtonManager::registerButton(DISPLAY_BUTTON_PIN, 75, 0, &onDisplayButtonPressed);
  
  // set up date button LED
  pinMode(DATE_LED_PIN, OUTPUT);
  turnOffDateLED();
  
  pinMode(DISPLAY_LED_PIN, OUTPUT);
  turnDisplayOn();
  
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
  analogWrite(DATE_LED_PIN, 128);
}

void turnOffDateLED()
{
  analogWrite(DATE_LED_PIN, 0);
}

void turnOnDisplayLED()
{
  analogWrite(DISPLAY_LED_PIN, 255);
}

void turnOffDisplayLED()
{
  analogWrite(DISPLAY_LED_PIN, 0);
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

void turnDisplayOff()
{
  Serial.println("Display off");
  displayOn = 0;
  clockDisplay->turnOff();
}

void turnDisplayOn()
{
  Serial.println("Display on");
  displayOn = 1;
  clockDisplay->turnOn();
}

void onDisplayButtonPressed()
{
  Serial.println("Display button pressed");
  if (inSetMode)
    return;
  if (displayOn)
  {
    turnOnDisplayLED();
    // turn off display
    turnDisplayOff();
    // turn off button interrupts but display button
    dateButton->onRelease = NULL;
    setButton->onRelease = NULL;
  }
  else
  {
    turnOffDisplayLED();
    // turn on display
    turnDisplayOn();
    // turn on interrupts
    dateButton->onRelease = &onDateButtonPressed;
    setButton->onRelease = &onSetButtonPressed;
    updateDisplay();
  }
}
