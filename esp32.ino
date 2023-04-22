#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "SPI.h"
#include <TFT_eSPI.h>
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
// #include <FastLED.h>
// #include <JC_Button.h>

#define TFT_DC 2
#define TFT_CS 15
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
// TFT_eSPI tft = TFT_eSPI();

#define NUM_POTENTIOMETERS 3
const int potPins[NUM_POTENTIOMETERS] = { 34, 35, 32 };
const String potNames[NUM_POTENTIOMETERS] = { "Treble", "Bass", "Volume" };
const int changeThreshold = 40;

// LED BAR
#define NUM_LEDS 8
#define LED_PIN 12
// CRGB leds[NUM_LEDS];

// Button
// #define BUTTON_PIN 33
// Button button(BUTTON_PIN, 25, true, true);
int buttonState = 0;

struct DisplayData {
  int pwmValue;
  int potentiometerNumber;
};

QueueHandle_t displayQueue;

void updateDisplay(void* parameter) {
  DisplayData displayData[NUM_POTENTIOMETERS];
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setTextSize(2);

  for (;;) {
    if (!buttonState) {
      if (xQueueReceive(displayQueue, &displayData, portMAX_DELAY) == pdPASS) {
        for (int i = 0; i < NUM_POTENTIOMETERS; i++) {
          int xPos = i * 80;
          // Potentiometer Name
          tft.setCursor(xPos, 20);
          tft.print(potNames[displayData[i].potentiometerNumber]);

          // Potentiometer Value
          tft.setCursor(xPos, 40);
          char formattedValue[10];
          sprintf(formattedValue, "%3d %%", displayData[i].pwmValue * 100 / 255);
          tft.print(formattedValue);

          // Draw progress bar
          uint16_t barHeight = map(displayData[i].pwmValue, 0, 255, 0, tft.height() - 60);
          tft.fillRect(xPos, 60, 50, tft.height() - 60, ILI9341_BLACK);
          tft.fillRect(xPos, tft.height() - barHeight, 50, barHeight, ILI9341_GREEN);
        }
      }
    } 
    // else {
    //   tft.fillScreen(ILI9341_BLACK);
    //   tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    //   tft.setTextSize(2);
    //   tft.setCursor(0, 20);
    //   tft.print("Frequency");
    // }
  }
}

void readPotentiometers(void* parameter) {
  int previousValues[NUM_POTENTIOMETERS] = { 0 };

  for (int i = 0; i < NUM_POTENTIOMETERS; i++) {
    previousValues[i] = analogRead(potPins[i]);
  }

  for (;;) {
    bool updateNeeded = false;
    DisplayData displayData[NUM_POTENTIOMETERS];

    for (int i = 0; i < NUM_POTENTIOMETERS; i++) {
      int value = analogRead(potPins[i]);
      int pwmValue = map(value, 0, 4095, 0, 255);

      if (abs(value - previousValues[i]) > changeThreshold) {
        displayData[i] = { pwmValue, i };
        previousValues[i] = value;
        updateNeeded = true;
        // uint8_t ledCount = map(pwmValue, 0, 255, 0, NUM_LEDS);

        // for (int j = 0; j < NUM_LEDS; j++) {
        //   leds[j] = (j < ledCount) ? CRGB::White : CRGB::Black;
        // }

        // FastLED.show();
      }
    }

    if (updateNeeded) {
      xQueueSend(displayQueue, &displayData, portMAX_DELAY);
    }

    // Delay to avoid excessive CPU usage
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

// void stateChange(void* parameter) {
//   for (;;) {
//     button.read();

//     if (button.wasPressed()) {
//       buttonState = !buttonState;
//     }
//     vTaskDelay(pdMS_TO_TICKS(100));
//   }
// }


void setup() {
  Serial.begin(115200);

  // Pinmode
  for (int i = 0; i < NUM_POTENTIOMETERS; i++) {
    pinMode(potPins[i], INPUT);
  }
  // FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  // FastLED.setBrightness(10);

  // Initialize TFT display
  tft.begin();
  tft.setRotation(0);
  // tft.fillScreen(TFT_BLACK);

  // Button Setup
  // button.begin();

  // Create the display data queue
  displayQueue = xQueueCreate(1, sizeof(DisplayData[NUM_POTENTIOMETERS]));

  // Create the read potentiometers task
  xTaskCreate(readPotentiometers, "ReadPotentiometers", 4096, NULL, 1, NULL);

  // Create the update display task
  xTaskCreate(updateDisplay, "UpdateDisplay", 4096, NULL, 2, NULL);
}

void loop() {
  // Do nothing
}
