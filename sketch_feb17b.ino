#include <KeyboardBLE.h>
#include <Arduino.h>















// Define the keyboard layout
char layout[4][5] = {
    {'q',      'w',               'f',              'p',    'b' },
    {'a',      'r',               's',              't',    'g' },
    {'z',      'x',               'c',              'd',    'v' },
    {KEY_ESC,  KEY_LEFT_SHIFT,    KEY_LEFT_CTRL,    ' ',    KEY_BACKSPACE },
};

// Define input and output pins
byte inputs[] = {20, 19, 18, 17, 16};   // Columns
byte outputs[] = {1, 2, 3, 4};           // Rows

volatile bool scanRequested = false;   // Flag set in ISR
bool pressed[4][5] = {false};            // Track key states

// Function to read battery percentage
int readBatteryPercentage() {
    const float conversion_factor = 3.3f / 4095.0f;
    const float voltage_divider_ratio = 3.0f;
    pinMode(25, OUTPUT);
    digitalWrite(25, HIGH);
    delayMicroseconds(50);
    int raw_adc = analogRead(29);
    digitalWrite(25, LOW);
    float vsys_voltage = raw_adc * conversion_factor * voltage_divider_ratio;
    return (int)(vsys_voltage * 10);
}

int bat;
unsigned long previousMillis = 0;
const unsigned long batteryInterval = 30000;

// Interrupt service routine (ISR) for input pins.
// Removed IRAM_ATTR for Pico W compatibility.
void inputISR() {
    scanRequested = true;
}

void setup() {
    Serial.begin(9600);
    analogReadResolution(12);
    bat = readBatteryPercentage();
    Serial.print("Battery Voltage: ");
    Serial.print(bat);
    Serial.println("%");
    KeyboardBLE.setBattery(bat);
    
    // Setup output pins (rows)
    for (byte i = 0; i < 4; i++) {
        pinMode(outputs[i], OUTPUT);
        digitalWrite(outputs[i], HIGH);
    }
    
    // Setup input pins (columns) with internal pull-ups and attach interrupts.
    for (byte i = 0; i < 5; i++) {
        pinMode(inputs[i], INPUT_PULLUP);
        attachInterrupt(digitalPinToInterrupt(inputs[i]), inputISR, CHANGE);
    }
    
    KeyboardBLE.begin();
//    delay(5000); // Allow time for BLE connection establishment.
    KeyboardBLE.setBattery(bat);
}

void loop() {
//    unsigned long currentMillis = millis();
//    if (currentMillis - previousMillis >= batteryInterval) {
//        previousMillis = currentMillis;
//        Serial.print(bat);
//        Serial.println("%");
//        digitalWrite(LED_BUILTIN, HIGH);
//        delay(200);
//        digitalWrite(LED_BUILTIN, LOW);
//    }
    
    // If no scan is requested, wait in a low-power state until an interrupt occurs.
    if (!scanRequested) {
      digitalWrite(LED_BUILTIN, HIGH);
//        delay(200);
//        digitalWrite(LED_BUILTIN, LOW);
//        delay(1000);
Serial.print("before");
        __wfi(); // Wait For Interrupt - puts the core in low power mode
Serial.print("after");
//        digitalWrite(LED_BUILTIN, HIGH);
//        delay(200);
        digitalWrite(LED_BUILTIN, LOW);
    }
    
    // Once an interrupt occurs and scanRequested is true, scan the matrix.
    for (byte row = 0; row < 4; row++) {
        digitalWrite(outputs[row], LOW);      // Activate the row
//        delayMicroseconds(2);                   // Allow signals to stabilize
        
        for (byte col = 0; col < 5; col++) {
            bool isPressed = (digitalRead(inputs[col]) == LOW);
            if (isPressed && !pressed[row][col]) {
                KeyboardBLE.press(layout[row][col]);
                pressed[row][col] = true;
            } else if (!isPressed && pressed[row][col]) {
                KeyboardBLE.release(layout[row][col]);
                pressed[row][col] = false;
            }
        }
        digitalWrite(outputs[row], HIGH);     // Deactivate the row
    }
    
    // Reset the flag after scanning
    scanRequested = false;
    
    // Small delay for any necessary debouncing (adjust as needed)
    delay(50);
}
