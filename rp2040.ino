#include "mbed.h"
#include "TFT_eSPI.h"
#include "FastLED.h"
#include "JC_Button.h"

// Potentiometers
#define NUM_POTENTIOMETERS 3
const PinName potPins[NUM_POTENTIOMETERS] = { A0, A1, A2 };
const char* potNames[NUM_POTENTIOMETERS] = { "Treble", "Bass", "Volume" };
const int changeThreshold = 40;

// LED BAR
#define NUM_LEDS 8
#define LED_PIN D12
CRGB leds[NUM_LEDS];

// Button
#define BUTTON_PIN D33
Button button(BUTTON_PIN, 25, true, true);
int buttonState = 0;

// Mbed OS Objects
AnalogIn pots[NUM_POTENTIOMETERS] = { AnalogIn(potPins[0]), AnalogIn(potPins[1]), AnalogIn(potPins[2]) };
DigitalOut ledOutput(LED_PIN);
InterruptIn buttonInterrupt(BUTTON_PIN);
TFT_eSPI tft = TFT_eSPI();
Thread readPotentiometersThread;
Thread updateDisplayThread;
Thread stateChangeThread;

struct DisplayData {
  int pwmValue;
  int potentiometerNumber;
};

// Create Mbed OS Queue for communication between tasks
Queue<DisplayData, NUM_POTENTIOMETERS> displayQueue;

void updateDisplay() {
  DisplayData displayData;
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);

  for (;;) {
    if (!buttonState) {
      if (displayQueue.try_get_for(Kernel::wait_for_u32_forever, &displayData)) {
        int i = displayData.potentiometerNumber;
        int xPos = i * 80;

        // Potentiometer Name
        tft.setCursor(xPos, 20);
        tft.print(potNames[displayData.potentiometerNumber]);

        // Potentiometer Value
        tft.setCursor(xPos, 40);
        char formattedValue[10];
        sprintf(formattedValue, "%3d %%", displayData.pwmValue * 100 / 255);
        tft.print(formattedValue);

        // Draw progress bar
        uint16_t barHeight = map(displayData.pwmValue, 0, 255, 0, tft.height() - 60);
        tft.fillRect(xPos, 60, 50, tft.height() - 60, TFT_BLACK);
        tft.fillRect(xPos, tft.height() - barHeight, 50, barHeight, ILI9341_GREEN);
      }
    } else {
      tft.fillScreen(TFT_BLACK);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.setTextSize(2);
      tft.setCursor(0, 20);
      tft.print("Frequency");
    }
  }
}

void readPotentiometers() {
  float previousValues[NUM_POTENTIOMETERS] = { 0 };

  for (int i = 0; i < NUM_POTENTIOMETERS; i++) {
    previousValues[i] = pots[i].read();
  }

  for (;;) {
    bool updateNeeded = false;
    DisplayData displayData;

    for (int i = 0; i < NUM_POTENTIOMETERS; i++) {
      float value = pots[i].read();
      int pwmValue = value * 255;

      if (abs(value - previousValues[i]) > (changeThreshold / 4095.0)) {
        displayData.pwmValue = pwmValue;
        displayData.potentiometerNumber = i;
        previousValues[i] = value;
        updateNeeded = true;
        uint8_t ledCount = pwmValue * NUM_LEDS / 255;

        for (int j = 0; j < NUM_LEDS; j++) {
          leds[j] = (j < ledCount) ? CRGB::White : CRGB::Black;
        }

        FastLED.show();
      }
    }

    if (updateNeeded) {
      displayQueue.put(&displayData);
    }

    ThisThread::sleep_for(100ms);
  }
}

void stateChange() {
  for (;;) {
    button.read();

    if (button.wasPressed()) {
      buttonState = !buttonState;
    }
    ThisThread::sleep_for(100ms);
  }
}

int main() {
  // Initialize TFT display
  tft.init();
  tft.setRotation(0);

  // Button Setup
  button.begin();

  // Create the read potentiometers task
  readPotentiometersThread.start(callback(readPotentiometers));

  // Create the update display task
  updateDisplayThread.start(callback(updateDisplay));

  // Create the state change task
  stateChangeThread.start(callback(stateChange));

  while (1) {
    // Do nothing, let the threads run
  }
}
