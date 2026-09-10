#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "Oled.h"
#include "Motores.h"
#include "Encoders.h"

Oled pantallita;
Motores motorcitos(4, 5, 40, 39);// IN1A, IN1B, IN2A, IN2B
Encoders myencoder;

#define boton1 43 //up - subir- derecha
#define boton2 44 //enter - calibrar 
#define boton3 42 //down - bajar - izquierda
#define led 38

#define TIME_PID 3

#define sig 3
#define pinA 16
#define pinB 19
#define pinC 20

#define eeprom_direccion 0x50

#define as5600_direccion 0x36

enum Estado {
  pantallazo,
  menu_principal,
  iniciar_carrera,
  comenzar, //es para que arranque el velocista
  calibracion,
  val_select,
  val_edit,
  EXTRA
};
Estado estadoActual = pantallazo;

// Variables de tiempo para Splash Screen
unsigned long tiempopantallazo = 0;

// Variables del Menú Principal
const int num_items = 6;
String menuItems[num_items] = {"Comenzar", "Calibracion", "Guardar Cal", "Cargar Cal", "Valores", "Extra"};
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
int max_rpm = 1000;
int baseSpeed = 370; //medido en rpm 5 de pwm en 255 equivale a 2 rpm aprox
bool anterior = 1; 

unsigned long lastTimePID = 0;

int PWM1 = baseSpeed;  //pwm de la izquierda
int PWM2 = baseSpeed;  //pwm de la derecha

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
//------- ENCODERS -------
int calcularPWM(float rpmDeseada, float rpmReal, int pwmBase){
  float KP= 0.05;
  float Error = rpmDeseada - rpmReal;
  float Correccion = Error * KP;
  int pwmFinal = pwmBase + Correccion;

return constrain(pwmFinal, 0, 4095);
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
      
      int velocidadIzquierda = constrain (baseSpeed - correccion, 0, max_rpm);
      int velocidadDerecha  = constrain (baseSpeed + correccion, 0, max_rpm);

      int pwmBaseIzq = map(velocidadIzquierda, 0, max_rpm, 0, 4095);
      int pwmBaseDer = map(velocidadDerecha, 0, max_rpm, 0, 4095);

      float rpmRealIzq = myencoder.calculoRPM1();
      float rpmRealDer = myencoder.calculoRPM2();

      int pwmFinalIzquierda = calcularPWM(velocidadIzquierda, rpmRealIzq, pwmBaseIzq);
      int pwmFinalDerecha  = calcularPWM(velocidadDerecha, rpmRealDer, pwmBaseDer);

      motorcitos.moverMotores(pwmFinalIzquierda, pwmFinalDerecha);
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
      estadoActual = val_edit;   //puede ir en vez de val_edit menu_principal
    }
  }
}

void ejecutarAccionMenu() {
  switch (menuIndex) {
      case 0: // Cronometro
      estadoActual = iniciar_carrera;
      break;
    case 1: // calibracion
      pantallita.mostrarMensaje("Calibrando...");
      calibrar();
      pantallita.mostrarMensaje("Calibracion OK");
      delay(1000);
      break;
    case 2: // guarda Calibracion
      pantallita.mostrarMensaje("Guardando...");
      {
        byte* datos = (byte*)umbrales; // Convertimos el arreglo de enteros a bytes
        for(unsigned int i = 0; i < sizeof(umbrales); i++) {
          Wire.beginTransmission(eeprom_direccion);
          Wire.write((i >> 8) & 0xFF);   // Parte alta de la dirección de memoria
          Wire.write(i & 0xFF);          // Parte baja de la dirección de memoria
          Wire.write(datos[i]);          // Dato a guardar
          Wire.endTransmission();
          delay(5); // Tiempo de escritura obligatorio del datasheet
        }
      }
      pantallita.mostrarMensaje("Cal Guardada!");
      delay(1500);
      break;
    case 3: // carga Calibracion
      pantallita.mostrarMensaje("Cargando...");
      {
        byte* datos = (byte*)umbrales;
        for(unsigned int i = 0; i < sizeof(umbrales); i++) {
          Wire.beginTransmission(eeprom_direccion);
          Wire.write((i >> 8) & 0xFF);
          Wire.write(i & 0xFF);
          Wire.endTransmission();
          
          Wire.requestFrom(eeprom_direccion, 1);
          if (Wire.available()) {
            datos[i] = Wire.read();
          }
        }
      }
      pantallita.mostrarMensaje("Datos Cargados!");
      delay(500);
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

void iniciarCuentaRegresiva() {
  for (int i = 5; i > 0; i--) {
    pantallita.display.clearDisplay();
    pantallita.display.setTextSize(3);
    pantallita.display.setCursor(50, 20);
    pantallita.display.print(i);
    pantallita.display.display();
    
    if (i <= 2){ 
    digitalWrite(led, HIGH); // Prende el LED en 2 y 1
    }
    else{ 
    digitalWrite(led, LOW);
    }
    delay(1000); // Espera 1 segundo
  }
  
  pantallita.display.clearDisplay();
  pantallita.display.setCursor(40, 20);
  pantallita.display.print("GO!");
  pantallita.display.display();
  
  digitalWrite(led, LOW);
  estadoActual = comenzar; // acá comienza
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

void setup() {
  Serial.begin(115200);
  pinMode(boton1, INPUT_PULLUP);
  pinMode(boton2, INPUT_PULLUP);
  pinMode(boton3, INPUT_PULLUP);
  pinMode(led, OUTPUT);

  digitalWrite(led, 0);

 //----Multiplexor----  
  pinMode(sig, INPUT);
  pinMode(pinA, OUTPUT);  
  pinMode(pinB, OUTPUT);
  pinMode(pinC, OUTPUT);  

  myencoder.begin();
  pantallita.begin();
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

      case iniciar_carrera:
      iniciarCuentaRegresiva();
      break;

      case comenzar:
      sprint(); 
      // frenar el robot si presiona --enter--
      if (enter) {
        motorcitos.moverMotores(0, 0);
        estadoActual = menu_principal;
        pantallita.mostrarMensaje("FRENADO");
        delay(500);
      }
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
      pantallita.display.print("Rami gay");
      pantallita.display.setTextSize(1);
      pantallita.display.setCursor(10, 50);
      pantallita.display.print("Enter salir");
      pantallita.display.display();
      break;
  }

}