#pragma once

#include <Arduino.h>

#define PIN_GPIO_0  0
#define PIN_GPIO_1  1
#define PIN_GPIO_2  2
#define PIN_GPIO_3  3
#define PIN_GPIO_4  4
#define PIN_GPIO_5  5
#define PIN_GPIO_9  9
#define PIN_GPIO_10 10
#define PIN_GPIO_12 12
#define PIN_GPIO_13 13
#define PIN_GPIO_14 14
#define PIN_GPIO_15 15
#define PIN_GPIO_16 16
#define PIN_GPIO_17 17
#define PIN_GPIO_18 18
#define PIN_GPIO_19 19
#define PIN_GPIO_21 21
#define PIN_GPIO_22 22
#define PIN_GPIO_23 23
#define PIN_GPIO_25 25
#define PIN_GPIO_26 26
#define PIN_GPIO_27 27
#define PIN_GPIO_32 32
#define PIN_GPIO_33 33
#define PIN_GPIO_34 34
#define PIN_GPIO_35 35
#define PIN_GPIO_36 36

#ifdef ESP8266

  #define PIN_D0      PIN_GPIO_16
  #define PIN_D1      PIN_GPIO_5
  #define PIN_D2      PIN_GPIO_4
  #define PIN_D3      PIN_GPIO_0
  #define PIN_D4      PIN_GPIO_2
  #define PIN_D5      PIN_GPIO_14
  #define PIN_D6      PIN_GPIO_12
  #define PIN_D7      PIN_GPIO_13
  #define PIN_D8      PIN_GPIO_15
  #define PIN_RX      PIN_GPIO_3
  #define PIN_TX      PIN_GPIO_1
  
  #define SETUP_LED   2
  #define SETUP_PIN   PIN_D1
  
  #define PIN_SDA     PIN_D2 
  #define PIN_SCL     PIN_D1
  
  #define PIN_TX      PIN_GPIO_1
  #define PIN_RX      PIN_GPIO_3
  
  #define PIN_TX2     PIN_GPIO_15
  #define PIN_RX2     PIN_GPIO_13
  
#endif // ESP8266

#ifdef ESP32

  #define PIN_D0      PIN_GPIO_16
  #define PIN_D1      PIN_GPIO_5
  #define PIN_D2      PIN_GPIO_4
  #define PIN_D3      PIN_GPIO_0
  #define PIN_D4      PIN_GPIO_2
  #define PIN_D5      PIN_GPIO_14
  #define PIN_D6      PIN_GPIO_12
  #define PIN_D7      PIN_GPIO_13
  #define PIN_D8      PIN_GPIO_15
  #define PIN_RX      PIN_GPIO_3
  #define PIN_TX      PIN_GPIO_1
  
  #define SETUP_LED   2
  #define SETUP_PIN   PIN_D1
  
  #define PIN_SDA     PIN_D2 
  #define PIN_SCL     PIN_D1
  
  #define PIN_TX      PIN_GPIO_1
  #define PIN_RX      PIN_GPIO_3
  
  #define PIN_TX2     PIN_GPIO_15
  #define PIN_RX2     PIN_GPIO_13

#endif  // ESP32


#ifdef ESP01 

  /*   D5/A0/ADC0/PB5 RESET  -|1*  8|-  VCC  
   *   D3/A3/ADC1/PB3        -|2   7|-  SCL/SCK    D2/A1/PB2
   *   D4/A2/ADC2/PB4        -|3   6|-  MISO       D1/PB1
   *                  GND    -|4   5|-  MOSI SDA   D0/PB0
   */
  
  #define PIN_GPIO0   0
  #define PIN_GPIO2   2
  
  #define PIN_LED     PIN_GPIO2
  
  #define PIN_SDA     PIN_GPIO0   
  #define PIN_SCL     PIN_GPIO2

#endif // ESP01

namespace app {

enum Result {
    RESULT_OK = 0,
    RESULT_FAILED,
    RESULT_NOENT
};

class PinBase {
public:
    PinBase(uint8_t pin): mPin(pin) {};
    ~PinBase() = default;
protected:
    uint8_t mPin;
};

} // namespace app