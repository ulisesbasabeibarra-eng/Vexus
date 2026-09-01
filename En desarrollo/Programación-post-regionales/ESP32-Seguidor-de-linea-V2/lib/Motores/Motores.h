#ifndef MOTORES_H
#define MOTORES_H

#include <Arduino.h>

class Motores {
private:
    const uint8_t ledChannel = 0;
    const uint8_t ledChannel1 = 1;
    const uint8_t ledChannel2 = 2;
    const uint8_t ledChannel3 = 3;

    const int frequency = 20000;
    const int resolution = 12;

public:
    Motores(uint8_t IN1A, uint8_t IN1B, uint8_t IN2A, uint8_t IN2B);
    void moverMotores(int izq, int der);

    //MOTOR DERECHO IN1A-IN1B
    //MOTOR IZQUIERDO IN2A-IN2B

};

#endif