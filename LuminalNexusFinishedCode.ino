// Import the required libraries
#include "config.h"
#include <NeoPixelBrightnessBus.h>
#include <WiFiManager.h>

// Define constants and pin configurations
#define N_LEDS 24
#define LED_PIN 3
#define BOT 4

//////////////////LAMP ID////////////////////////////////////////////////////////////
int lampID = 2; // Lamp identifier
/////////////////////////////////////////////////////////////////////////////////////

// Initialize NeoPixel strip
NeoPixelBrightnessBus<NeoGrbFeature, NeoEsp8266Dma800KbpsMethod> strip(N_LEDS, LED_PIN);

// Adafruit initialization
AdafruitIO_Feed *lamp = io.feed("lamp"); // Change to your feed

// Declare and initialize variables
int recVal {0};
int sendVal {0};
const int max_intensity = 255; // Max intensity
int selected_color = 0;       // Index for color vector
int i_breath;
char msg[50]; // Custom messages for Adafruit IO

// Color definitions
RgbColor red(max_intensity, 0, 0);
RgbColor green(0, max_intensity, 0);
RgbColor blue(0, 0, max_intensity);
RgbColor purple(200, 0, max_intensity);
RgbColor cian(0, max_intensity, max_intensity);
RgbColor yellow(max_intensity, 200, 0);
RgbColor white(max_intensity, max_intensity, max_intensity);
RgbColor pink(255, 20, 30);
RgbColor orange(max_intensity, 50, 0);
RgbColor black(0, 0, 0);

// Array of colors
RgbColor colors[] = {
  red,
  orange,
  yellow,
  green,
  cian,
  blue,
  purple,
  pink,
  white,
  black
};

int state = 0; // Initial state

// Time variables
unsigned long RefMillis;
unsigned long ActMillis;
const int send_selected_color_time = 4000;
const int answer_time_out = 900000;
const int on_time = 900000;

// Disconnection timeout
unsigned long currentMillis;
unsigned long previousMillis = 0;
const unsigned long connection_time_out = 300000; // 5 minutes

// Long press detection
const int long_press_time = 2000;
int lastState = LOW;      // Previous state from the input pin
int currentState;         // Current reading from the input pin
unsigned long pressedTime = 0;
unsigned long releasedTime = 0;

void setup() {
  // Start the serial monitor for debugging and status
  Serial.begin(115200);

  // Activate neopixels
  strip.Begin();
  strip.Show(); // Initialize all pixels to 'off'

  // Configure WiFi settings
  wificonfig();

  // Set pin mode for capacitive sensor
  pinMode(BOT, INPUT);

  // Set ID values based on lampID
  if (lampID == 1) {
    recVal = 20;
    sendVal = 10;
  } else if (lampID == 2) {
    recVal = 10;
    sendVal = 20;
  }

  // Start connecting to Adafruit IO
  Serial.printf("\nConnecting to Adafruit IO with User: %s Key: %s.\n", IO_USERNAME, IO_KEY);
  AdafruitIO_WiFi io(IO_USERNAME, IO_KEY, "", "");
  io.connect();

  // Set up message handler for Adafruit IO
  lamp->onMessage(handle_message);

  // Wait for Adafruit IO connection
  while (io.status() < AIO_CONNECTED) {
    Serial.print(".");
    spin(6);
    delay(500);
  }
  turn_off();
  Serial.println();
  Serial.println(io.statusText());
  // Animation
  spin(3);  turn_off(); delay(50);
  flash(8); turn_off(); delay(100);
  flash(8); turn_off(); delay(50);

  // Get the status of our value in Adafruit IO
  lamp->get();
  sprintf(msg, "L%d: connected", lampID);
  lamp->save(msg);
}

void loop() {
  // Get current millis for timeout checks
  currentMillis = millis();
  // Run Adafruit IO tasks
  io.run();

  // State machine
  switch (state) {
      // Wait for button press
    case 0:
      currentState = digitalRead(BOT);
      if(lastState == LOW && currentState == HIGH)  // Button is pressed
      {
        pressedTime = millis();
      }
      else if(currentState == HIGH) {       // Button released, check for long press
        releasedTime = millis();
        long pressDuration = releasedTime - pressedTime;
        if( pressDuration > long_press_time )
        {        // Detected long press, transition to the next state
            state = 1;
        }
      }
      lastState = currentState;
      break;
        // Wait for button release and set initial color
    case 1:
      selected_color = 0;
      light_half_intensity(selected_color);
      state = 2;
      RefMillis = millis();

      // Wait for button release before transitioning to the next state
      while(digitalRead(BOT) == HIGH){}
      break;
      // Color selector state
    case 2:
      if (digitalRead(BOT) == HIGH) {
        selected_color++;
          // Button pressed, cycle through colors
        if (selected_color > 9)
          selected_color = 0;
        while (digitalRead(BOT) == HIGH) {
          delay(50);
        }
          // Set NeoPixels to the selected color and reset timer
        light_half_intensity(selected_color);
        // Reset timer each time it is touched
        RefMillis = millis();
      }
        // If a color is selected for a certain duration, transition to the next state
      ActMillis = millis();
      if (ActMillis - RefMillis > send_selected_color_time) {
        if (selected_color == 9) //  Cancel msg
          state = 8;
        else
          state = 3;
      }
      break;
      // Publish selected color to Adafruit IO
    case 3:
      sprintf(msg, "L%d: color send", lampID);
      lamp -> save(msg);
      lamp -> save(selected_color + sendVal);
      Serial.print(selected_color + sendVal);
      state = 4;
        // Flash and set NeoPixels to half intensity
      flash(selected_color);
      light_half_intensity(selected_color);
      delay(100);
      flash(selected_color);
      light_half_intensity(selected_color);
      break;
        // Set timer for waiting for an answer
    case 4:
      RefMillis = millis();
      state = 5;
      i_breath = 0;
      break;
        // Wait for an answer from Adafruit IO
    case 5:
      for (i_breath = 0; i_breath <= 314; i_breath++) {
        breath(selected_color, i_breath);
        ActMillis = millis();
        if (ActMillis - RefMillis > answer_time_out) {
            // Answer timeout, turn off and transition to the final state
          turn_off();
          lamp -> save("L%d: Answer time out", lampID);
          lamp -> save(0);
          state = 8;
          break;
        }
      }
      break;
      // Answer received from Adafruit IO
    case 6:
      Serial.println("Answer received");
        // Set NeoPixels to full intensity and update Adafruit IO
      light_full_intensity(selected_color);
      RefMillis = millis();
      sprintf(msg, "L%d: connected", lampID);
      lamp -> save(msg);
      lamp -> save(0);
      state = 7;
      break;
        // Lamp turned on, waiting for button press or timeout
    case 7:
      ActMillis = millis();
      // Send pulse if button is pressed
      if (digitalRead(BOT) == HIGH) {
        lamp -> save(420 + sendVal);
        pulse(selected_color);
      }
      // Turn off if timeout is reached
      if (ActMillis - RefMillis > on_time) {
        turn_off();
        lamp -> save(0);
        state = 8;
      }
      break;
      // Reset before state 0
    case 8:
      turn_off();
      state = 0;
      break;
        // Received message from Adafruit IO
    case 9:
      sprintf(msg, "L%d: msg received", lampID);
      lamp -> save(msg);
      RefMillis = millis();
      state = 10;
      break;
        // Send answer to Adafruit IO
    case 10:
      for (i_breath = 236; i_breath <= 549; i_breath++) {
        breath(selected_color, i_breath);
        if (digitalRead(BOT) == HIGH) {
          state = 11;
          break;
        }
        ActMillis = millis();
        if (ActMillis - RefMillis > answer_time_out) {
          // Answer timeout, turn off and transition to the final state
          turn_off();
          sprintf(msg, "L%d: answer time out", lampID);
          lamp -> save(msg);
          state = 8;
          break;
        }
      }
      break;
      // Send answer to Adafruit IO and transition to the ON state
    case 11:
      light_full_intensity(selected_color);
      RefMillis = millis();
      sprintf(msg, "L%d: answer sent", lampID);
      lamp -> save(msg);
      lamp -> save(1);
      state = 7;
      break;
    default:
        // Reset to initial state if unknown state is encountered
        state = 0;
      break;
  }

  // Check for WiFi disconnection and restart if necessary
  if ((currentMillis - previousMillis >= connection_time_out)) {
    if (WiFi.status() != WL_CONNECTED)
      ESP.restart();
    previousMillis = currentMillis;
  }
}

// Function to handle incoming messages from Adafruit IO'
//  Input: AdafruitIO_Data* data - received data from Adafruit IO feed
void handle_message(AdafruitIO_Data* data) {
  int reading = data->toInt(); // Convert received data to an INT
  if (reading == 66) {
    sprintf(msg, "L%d: rebooting", lampID);
    lamp->save(msg);
    lamp->save(0);
    ESP.restart();
  } else if (reading == 100) {
    sprintf(msg, "L%d: ping", lampID);
    lamp->save(msg);
    lamp->save(0);
  } else if (reading == 420 + recVal) {
    sprintf(msg, "L%d: pulse received", lampID);
    lamp->save(msg);
    lamp->save(0);
    pulse(selected_color);
  } else if (reading != 0 && reading / 10 != lampID) {
    // Is it a color msg?
    if (state == 0 && reading != 1) {
      state = 9;
      selected_color = reading - recVal;
    }
    // Is it an answer?
    if (state == 5 && reading == 1)
      state = 6;
  }
}

// Function to turn off NeoPixels
void turn_off() {
  strip.SetBrightness(max_intensity);
  for (int i = 0; i < N_LEDS; i++) {
    strip.SetPixelColor(i, black);
  }
  strip.Show();
}

// Function to set NeoPixels to 50% intensity
// Input: int ind - index of the color in the colors array
void light_half_intensity(int ind) {
  strip.SetBrightness(max_intensity / 2);
  for (int i = 0; i < N_LEDS; i++) {
    strip.SetPixelColor(i, colors[ind]);
  }
  strip.Show();
}

// Function to set NeoPixels to 100% intensity
// Input: int ind - index of the color in the colors array
void light_full_intensity(int ind) {
  strip.SetBrightness(max_intensity);
  for (int i = 0; i < N_LEDS; i++) {
    strip.SetPixelColor(i, colors[ind]);
  }
  strip.Show();
}

// Function to create a pulsating effect on NeoPixels
// Input: int ind - index of the color in the colors array
void pulse(int ind) {
  int i;
  int i_step = 5;
  for (i = max_intensity; i > 80; i -= i_step) {
    strip.SetBrightness(i);
    for (int i = 0; i < N_LEDS; i++) {
      strip.SetPixelColor(i, colors[ind]);
      strip.Show();
      delay(1);
    }
  }
  delay(20);
  for (i = 80; i < max_intensity; i += i_step) {
    strip.SetBrightness(i);
    for (int i = 0; i < N_LEDS; i++) {
      strip.SetPixelColor(i, colors[ind]);
      strip.Show();
      delay(1);
    }
  }
}

// Function to create a gradual color change animation on NeoPixels
// Input: int ind - index of the color in the colors array
void spin(int ind) {
  strip.SetBrightness(max_intensity);
  for (int i = 0; i < N_LEDS; i++) {
    strip.SetPixelColor(i, colors[ind]);
    strip.Show();
    delay(30);
  }
  for (int i = 0; i < N_LEDS; i++) {
    strip.SetPixelColor(i, black);
    strip.Show();
    delay(30);
  }
}

// Function to create a breathing effect on NeoPixels
// Input: int ind - index of the color in the colors array
//        int i - iteration variable for breathing effect
void breath(int ind, int i) {
  float MaximumBrightness = max_intensity / 2;
  float SpeedFactor = 0.02;
  float intensity;
  if (state == 5)
    intensity = MaximumBrightness / 2.0 * (1 + cos(SpeedFactor * i));
  else
    intensity = MaximumBrightness / 2.0 * (1 + sin(SpeedFactor * i));
  strip.SetBrightness(intensity);
  for (int ledNumber = 0; ledNumber < N_LEDS; ledNumber++) {
    strip.SetPixelColor(ledNumber, colors[ind]);
    strip.Show();
    delay(1);
  }
}

// Function to flash NeoPixels for a brief period
// Input: int ind - index of the color in the colors array
void flash(int ind) {
  strip.SetBrightness(max_intensity);
  for (int i = 0; i < N_LEDS; i++) {
    strip.SetPixelColor(i, colors[ind]);
  }
  strip.Show();

  delay(200);
}

// Function to indicate waiting for a WiFi connection with NeoPixels
void wait_connection() {
  strip.SetBrightness(max_intensity);
  for (int i = 0; i < 3; i++) {
    strip.SetPixelColor(i, yellow);
  }
  strip.Show();
  delay(50);
  for (int i = 3; i < 6; i++) {
    strip.SetPixelColor(i, red);
  }
  strip.Show();
  delay(50);
  for (int i = 6; i < 9; i++) {
    strip.SetPixelColor(i, blue);
  }
  strip.Show();
  delay(50);
  for (int i = 9; i < 12; i++) {
    strip.SetPixelColor(i, green);
  }
  strip.Show();
  delay(50);
}

// Function to configure WiFi settings using WiFiManager
// Input: WiFiManager* myWiFiManager - WiFiManager instance for configuration
void configModeCallback(WiFiManager* myWiFiManager) {
  Serial.println("Entered config mode");
  wait_connection();
}

// Function to configure WiFi and connect to the network
void wificonfig() {
  WiFi.mode(WIFI_STA);
  WiFiManager wifiManager;

  std::vector<const char*> menu = {"wifi", "info"};
  wifiManager.setMenu(menu);
  // Set dark theme
  wifiManager.setClass("invert");

  bool res;
  wifiManager.setAPCallback(configModeCallback);
  res = wifiManager.autoConnect("Lamp", "password");
