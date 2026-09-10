#ifndef OLED_H
#define OLED_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

class Oled {
private:

    static const unsigned char vexusImagen[];

public:
    Adafruit_SSD1306 display;

    Oled(); // constructor
    void begin();
    void mostrarMensaje(String texto);

};
#endif