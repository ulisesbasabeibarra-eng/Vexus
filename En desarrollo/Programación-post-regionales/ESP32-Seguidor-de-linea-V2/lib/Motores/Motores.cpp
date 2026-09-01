#include "Motores.h"
#include <Arduino.h>

Motores::Motores(uint8_t IN1A, uint8_t IN1B, uint8_t IN2A, uint8_t IN2B) {
    ledcSetup(ledChannel, frequency, resolution);
    ledcAttachPin(IN1A, ledChannel);

    ledcSetup(ledChannel1, frequency, resolution);
    ledcAttachPin(IN1B, ledChannel1);

    ledcSetup(ledChannel2, frequency, resolution);
    ledcAttachPin(IN2A, ledChannel2);

    ledcSetup(ledChannel3, frequency, resolution);
    ledcAttachPin(IN2B, ledChannel3);
}

void Motores::moverMotores(int izq, int der) {
    if (izq >= 0) {
        if (izq > 4095) izq = 4095;
        ledcWrite(ledChannel, izq);
        ledcWrite(ledChannel1, 0);
    } else {
        izq = izq * (-1);
        if (izq > 4095) izq = 4095;
        ledcWrite(ledChannel, 0);
        ledcWrite(ledChannel1, izq);
    }
  
    if (der >= 0) {
        if (der > 4095) der = 4095;
        ledcWrite(ledChannel2, der);
        ledcWrite(ledChannel3, 0);
    } else {
        der = der * (-1);
        if (der > 4095) der = 4095;
        ledcWrite(ledChannel2, 0);
        ledcWrite(ledChannel3, der);
    }
}

