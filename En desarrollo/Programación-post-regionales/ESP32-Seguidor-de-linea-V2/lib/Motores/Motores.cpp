#include "Motores.h"
#include <Arduino.h>

class Motores {
 private:
    // Canales PWM del ESP32
    const uint8_t ledChannel = 0;
    const uint8_t ledChannel1 = 1;
    const uint8_t ledChannel2 = 2;
    const uint8_t ledChannel3 = 3;

    const int frequency = 20000;
    const int resolution = 12;


 public:
  Motores(uint8_t IN1A, uint8_t IN1B, uint8_t IN2A, uint8_t IN2B){
    ledcSetup(ledChannel, frequency, resolution);
    ledcAttachPin(IN1A, ledChannel);

    ledcSetup(ledChannel, frequency, resolution);
    ledcAttachPin(IN2A, ledChannel1);

    ledcSetup(ledChannel, frequency, resolution);
    ledcAttachPin(IN2A, ledChannel2);

    ledcSetup(ledChannel, frequency, resolution);
    ledcAttachPin(IN2B, ledChannel3);
  }
  void moverMotores(int izq, int der);
};

void Motores::moverMotores(int izq, int der){
    if (izq >= 0) {
    if(izq > 4095){ izq = 4095; }
    ledcWrite(ledChannel, izq);
    ledcWrite(ledChannel1, 0);
  } else {
    izq = izq * (-1);
    if(izq > 4095){ izq = 4095; }
    ledcWrite(ledChannel, 0);
    ledcWrite(ledChannel1, izq);
  }
  
  if (der >= 0) {
    if(der > 4095){ der = 4095; }
    ledcWrite(ledChannel2, der);
    ledcWrite(ledChannel3, 0);
  } else {
    der = der * (-1);
    if(der > 4095){ der = 4095; }
    ledcWrite(ledChannel2, 0);
    ledcWrite(ledChannel3, der);
  }
}

