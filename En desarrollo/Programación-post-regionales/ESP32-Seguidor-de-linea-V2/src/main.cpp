#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>

#include "Oled.h"
#include "Motores.h"

Oled pantallita;

//MOTOR DERECHO 
#define IN1A 4
#define IN1B 5

//MOTOR IZQUIERDO 
#define IN2A 40
#define IN2B 39

#define boton1 43 //up - subir- derecha
#define boton2 44 //enter - calibrar 
#define boton3 42 //down - bajar - izquierda

#define led 38

#define TIME_PID 3

#define sig 3
#define pinA 16
#define pinB 19
#define pinC 20

enum Estado {
  pantallazo,
  menu_principal,
  cronometro,
  CALIBRACION,
  val_select,
  val_edit,
  EXTRA
};
Estado estadoActual = pantallazo;

// Variables de tiempo para Splash Screen
unsigned long tiempopantallazo = 0;

// Variables del Menú Principal
const int num_items = 6;
String menuItems[num_items] = {"Cronometro", "Calibracion", "Guardar Cal", "Cargar Cal", "Valores", "Extra"};
int menuIndex = 0;

// para el submenú de valores
int valorIndex = 0; 

// time cronómetro
unsigned long tiempoInicio = 0;
bool cronoActivo = false;


int Sensor[8] = {39,34,35,32,33,25,26,27}; // falta aclarar tema del multi

int umbrales[8] = {0,0,0,0,0,0,0,0};
bool estado_booleano[8] = {0,0,0,0,0,0,0,0};
uint8_t channel[8] = {4,5,6,7,8,9,10,11};

// === VARIABLES PID Y CONTROL ===
int pos = 0;
int poslast = 350;
float kp = 0.32; //aumenta la fuerza con la que el robot corrige el error, si se aumenta demasiado oscila
float ki = 0;
float kd = 6.45; //suavizante de la oscilación
float error = 0, error2 = 0, error3 = 0, error4 = 0, error5 = 0, error6 = 0;
float lastError = 0;
float integral = 0;
float derivative = 0;
float setpoint = 400;
int correccion = 0;
int baseSpeed = 1800;
bool anterior = 1; 

unsigned long lastTimePID = 0;
unsigned long lastDisplayTime = 0;

// === NUEVAS VARIABLES PARA EL BOTÓN ===
bool ultimoEstadoBoton = HIGH;
unsigned long ultimoTiempoRebote = 0;
const unsigned long tiempoDebounce = 50; // 50 ms para evitar falsos toques

// Configuración de PWM para control de motores
const int frequency = 20000;
const int resolution = 12;

// Canales PWM del ESP32
const int ledChannel = 0;
const int ledChannel1 = 1;
const int ledChannel2 = 2;
const int ledChannel3 = 3;

int PWM1 = baseSpeed;  //pwm de la izquierda
int PWM2 = baseSpeed;  //pwm de la derecha

void motores(int izq, int der) {
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

int calcularPID(int lectura) {
    error = setpoint - lectura;
    integral = error + error2 + error3 + error4 + error5 + error6;
    derivative = error - lastError;
    lastError = error;
    
    error6 = error5;
    error5 = error4;
    error4 = error3;
    error3 = error2;
    error2 = error;
    
    return (kp * error + ki * integral + kd * derivative);
}

//-------MULTIPLEXOR-------

void leermulti2(){
  for(int i= 0; i<8; i++){
    digitalWrite(pinA, i&0x01);
    digitalWrite(pinB, i&0x02);
    digitalWrite(pinC, i&0x04);
    Sensor[i]=analogRead(sig);
  }
}

//-------FIN MULTIPLEXOR-------

void calibrar(){
    int blancos[8] = {0,0,0,0,0,0,0,0};
    int negro[8] = {0,0,0,0,0,0,0,0};
    digitalWrite(led, 1);
    while(digitalRead(boton2) == 1) { delay(1); }
    
    leermulti2();
    for (int x = 0; x < 8; x++) {
        blancos[x] = Sensor[x];    
    delay(100);
    }

    while(digitalRead(boton2) == 0) { delay(1); }
    digitalWrite(led, 0);
    delay(500);
    digitalWrite(led, 1);
    
    while(digitalRead(boton2)) { delay(1); }
    
    leermulti2();
    for (int x = 0; x < 8; x++) {
        negro[x] = Sensor[x];
    }
    for(int x = 0; x < 8; x++){
        umbrales[x] = (blancos[x] + negro[x]) / 2;
    }
    digitalWrite(led, 0);
    delay(1000);
    digitalWrite(led, 1);

    while(digitalRead(boton2) == 1) { delay(1); }
}

void sprint(){
    // Revisamos si el botón ha sido presionado en cada ciclo
 // revisarBoton();

  leermulti2();
  for(int x = 0; x < 8; x++){
    estado_booleano[x] =Sensor[x] > umbrales[x]? 0 : 1;
  }
  
  if(estado_booleano[0] == 0 && estado_booleano[0] != estado_booleano[7]){
    for(int x = 0; x < 8; x++){
      pos += 100 * estado_booleano[x];
    }
    anterior = 1;
  }
  if(estado_booleano[7] == 0 && estado_booleano[0] != estado_booleano[7]){
    for(int x = 0; x < 8; x++){
      pos += 100 * !estado_booleano[x];
    }
    anterior = 0;
  }
  if(estado_booleano[0] == estado_booleano[7]){
    if(anterior){
      for(int x = 0; x < 8; x++){
        pos += 100 * estado_booleano[x];
      }
    }
    else{
      for(int x = 0; x < 8; x++){
        pos += 100 * !estado_booleano[x];
      }
    }
  }
  
  if(micros() - lastTimePID >= TIME_PID){
      correccion = calcularPID(pos);
      lastTimePID = micros();
      
      int velocidadIzquierda = constrain (baseSpeed - correccion, 0, 255);
      int velocidadDerecha  = constrain (baseSpeed + correccion, 0, 255);
      motores(velocidadIzquierda, velocidadDerecha);
  }
  pos = 0;
}

//-------OLED-------
void manejarValoresSelect(bool up, bool enter, bool down) {
  if (up) { 
    valorIndex--;
    if (valorIndex < 0) valorIndex = 3;
  }
  if (down) {
    valorIndex++;
    if (valorIndex > 3) valorIndex = 0;
  }
  if (enter) {
    if (valorIndex == 3) {
      estadoActual = menu_principal; 
    } else {
      estadoActual = menu_principal;   
    }
  }
}

void ejecutarAccionMenu() {
  switch (menuIndex) {
    case 0: // Cronometro
      cronoActivo = true;
      tiempoInicio = millis();
      digitalWrite(led, LOW);
      estadoActual = cronometro;
      break;
    case 1: // Calibracion
      pantallita.mostrarMensaje("Calibrando...");
      calibrar();
      pantallita.mostrarMensaje("Calibracion OK");
      delay(1000);
      break;
    case 2: // Guardar Calibracion
      EEPROM.put(0, 123); 
      pantallita.mostrarMensaje("Guardado EEPROM");
      delay(1500);
      break;
    case 3: // Cargar Calibracion
      int llave;
      EEPROM.get(0, llave);
      if (llave == 123) {
        pantallita.mostrarMensaje("Datos Cargados!");
      } else {
        pantallita.mostrarMensaje("No hay datos :(");
      }
      delay(1500);
      break;
    case 4: // Valores
      valorIndex = 0;
      estadoActual = val_select;
      break;
    case 5: // Extra
      estadoActual = EXTRA;
      break;
  }
}

void manejarMenuPrincipal(bool up, bool enter, bool down) {
  if (up) {
    menuIndex--;
    if (menuIndex < 0) menuIndex = num_items - 1;
  }
  if (down) {
    menuIndex++;
    if (menuIndex >= num_items) menuIndex = 0;
  }
  if (enter) {
  ejecutarAccionMenu();
  }
}

void manejarValoresEdit(bool up, bool enter, bool down) {
  if (enter) {
    estadoActual = val_select; 
    return;
  }
  if (valorIndex == 0) {
    if (up) baseSpeed += 100;
    if (down) baseSpeed -= 100;
  } 
  else if (valorIndex == 1) { 
    if (up) kp += 0.1;
    if (down) kp -= 0.1;
  } 
  else if (valorIndex == 2) { 
    if (up) kd += 0.5;
    if (down) kd -= 0.5;
  }
}

void dibujarMenuPrincipal() {
  pantallita.display.clearDisplay();
  pantallita.display.setTextSize(1);
  pantallita.display.setTextColor(WHITE);
  
  int inicio = max(0, menuIndex - 2);
  int fin = min(num_items, inicio + 4);
  
  for (int i = inicio; i < fin; i++) {
    int y = (i - inicio) * 16;
    if (i == menuIndex) {
      pantallita.display.setCursor(0, y);
      pantallita.display.print("> ");
      pantallita.display.print(menuItems[i]);
    } else {
      pantallita.display.setCursor(10, y);
      pantallita.display.print(menuItems[i]);
    }
  }
  pantallita.display.display();
}

void manejarCronometro() {
  if (!cronoActivo) return;

  unsigned long tiempoTranscurrido = millis() - tiempoInicio;
  int segundosRestantes = 5 - (tiempoTranscurrido / 1000);

  pantallita.display.clearDisplay();
  pantallita.display.setTextSize(3);
  pantallita.display.setCursor(50, 20);

  if (segundosRestantes > 0) {
    pantallita.display.print(segundosRestantes);
    if (segundosRestantes <= 2) {
      digitalWrite(led, HIGH);
    }
  } else {
    pantallita.display.print("GO!");
    pantallita.display.display();
    cronoActivo = false; 
    sprint();     
    estadoActual = menu_principal;
    digitalWrite(led, LOW); 
    delay(1000);
    return;
  }
  pantallita.display.display();
}


void dibujarValores() {
  pantallita.display.clearDisplay();
  pantallita.display.setTextSize(1);
  pantallita.display.setCursor(0, 0);
  
  if (estadoActual == val_select) {
    pantallita.display.print("Sel (Izq/Der)");
  } else {
    pantallita.display.print("Edit (Sub/Baj)");
  }

  pantallita.display.setCursor(0, 20);
  if (valorIndex == 0) pantallita.display.print(estadoActual == val_edit ? "[*" : "[ ");
  pantallita.display.print("Vel:"); pantallita.display.print(baseSpeed);
  if (valorIndex == 0) pantallita.display.print(estadoActual == val_edit ? "*]" : " ]");

  pantallita.display.setCursor(64, 20);
  if (valorIndex == 1) pantallita.display.print(estadoActual == val_edit ? "[*" : "[ ");
  pantallita.display.print("Kp:"); pantallita.display.print(kp, 1);
  if (valorIndex == 1) pantallita.display.print(estadoActual == val_edit ? "*]" : " ]");

  pantallita.display.setCursor(0, 40);
  if (valorIndex == 2) pantallita.display.print(estadoActual == val_edit ? "[*" : "[ ");
  pantallita.display.print("Kd:"); pantallita.display.print(kd, 1);
  if (valorIndex == 2) pantallita.display.print(estadoActual == val_edit ? "*]" : " ]");

  pantallita.display.setCursor(64, 40);
  if (valorIndex == 3) pantallita.display.print("[ ");
  pantallita.display.print("Volver");
  if (valorIndex == 3) pantallita.display.print(" ]");

  pantallita.display.display();
}

//-------FIN OLED MODIFICABLE-------

// === NUEVA FUNCIÓN PARA REVISAR EL BOTÓN ===
void revisarBoton() {
  // Si el botón se presiona (LOW porque es INPUT_PULLUP)
  if (digitalRead(boton2) == LOW) { 
    delay(50); // Antirebote de hardware por software (espera a que se estabilice)
    
    if (digitalRead(boton2) == LOW) { // Confirmamos que sigue presionado
      kp += 0.05;
      
      // Feedback visual: Cambia el estado del LED para saber que entró al IF
      digitalWrite(led, !digitalRead(led)); 
      
      // Imprime en el monitor serie
      Serial.print("¡Botón detectado! Nuevo kp: ");
      Serial.println(kp);
      
      // Bucle de espera: se queda aquí hasta que SUELTES el botón
      // Esto evita que sume +0.05 infinitamente mientras lo dejas hundido
      while(digitalRead(boton2) == LOW) {
        delay(10); 
      }
    }
  }
}


void setup() {
  Serial.begin(115200);
  pinMode(boton1, INPUT_PULLUP);
  pinMode(boton2, INPUT_PULLUP);
  pinMode(boton3, INPUT_PULLUP);
  pinMode(led, OUTPUT);
  calibrar();
  digitalWrite(led, 0);

//----Multiplexor----  
  pinMode(sig, OUTPUT);
  pinMode(pinA, OUTPUT);  
  pinMode(pinB, OUTPUT);
  pinMode(pinC, OUTPUT);  

  pantallita.begin();
  
  ledcSetup(ledChannel, frequency, resolution);
  ledcAttachPin(IN1A, ledChannel);

  ledcSetup(ledChannel1, frequency, resolution);
  ledcAttachPin(IN1B, ledChannel1);

  ledcSetup(ledChannel2, frequency, resolution);
  ledcAttachPin(IN2A, ledChannel2);

  ledcSetup(ledChannel3, frequency, resolution);
  ledcAttachPin(IN2B, ledChannel3);

/*  while(digitalRead(boton2)){}
  digitalWrite(led, LOW);
  delay(3000);
  digitalWrite(led, HIGH);
  delay(2000);
  digitalWrite(led, LOW);
*/  
}

void loop() {
  // Leemos botones (LOW es presionado)
  bool up = (digitalRead(boton1) == LOW);
  bool enter = (digitalRead(boton2) == LOW);
  bool down = (digitalRead(boton3) == LOW);

  if (up || enter || down) {
    delay(50);} // Debounce

  switch (estadoActual) {
    case pantallazo:
      // Verifica si pasaron 5 seg O si se presiona enter
      if ((millis() - tiempopantallazo >= 5000) || enter) {
        estadoActual = menu_principal;
        pantallita.display.clearDisplay();
      }
      break;

    case menu_principal:
      manejarMenuPrincipal(up, enter, down);
      dibujarMenuPrincipal();
      break;

    case cronometro:
      manejarCronometro();
      break;

    case val_select:
      manejarValoresSelect(up, enter, down);
      dibujarValores();
      break;

    case val_edit:
      manejarValoresEdit(up, enter, down);
      dibujarValores();
      break;

    case EXTRA:
      if (enter) estadoActual = menu_principal;
      pantallita.display.clearDisplay();
      pantallita.display.setCursor(10, 25);
      pantallita.display.setTextSize(2);
      pantallita.display.print("Hola");
      pantallita.display.setTextSize(1);
      pantallita.display.setCursor(10, 50);
      pantallita.display.print("Enter salir");
      pantallita.display.display();
      break;
  }

}