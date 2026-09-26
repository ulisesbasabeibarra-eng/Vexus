#include <Arduino.h>
#include <Preferences.h>

#include "Motores.h"
Motores motorcitos(4, 15, 22, 23);// IN1A, IN1B (Izquierdo) IN2A, IN2B (derecho)
Preferences preferences;

#define BOTTOM 17
#define LED 2
#define TIME_PID 3

int Sensor[8] = {39,34,35,32,33,25,26,27};

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
int baseSpeed = 95;
bool anterior = 1; 

unsigned long lastTimePID = 0;
unsigned long lastDisplayTime = 0;

// === NUEVAS VARIABLES PARA EL BOTÓN ===
bool ultimoEstadoBoton = HIGH;
unsigned long ultimoTiempoRebote = 0;
const unsigned long tiempoDebounce = 50; // 50 ms para evitar falsos toques

int PWM1 = baseSpeed;  //pwm de la izquierda
int PWM2 = baseSpeed;


void cargarCalibracion() {
    preferences.begin("robot", true);
    if (preferences.getBytesLength("umbrales") == sizeof(umbrales)) {
        preferences.getBytes("umbrales", umbrales, sizeof(umbrales));
        Serial.println("calibración cargada desde la flash");
    } else {
        Serial.println("no existen datos previos");
    }
    preferences.end();
}


void calibrar(){
    int blancos[8] = {0,0,0,0,0,0,0,0};
    int negro[8] = {0,0,0,0,0,0,0,0};
    digitalWrite(LED, 1);
    while(digitalRead(BOTTOM) == 1) { delay(1); }
    
    for(int x = 0; x < 8; x++){
        delayMicroseconds(20); 
        blancos[x] = analogRead(Sensor[x]);
    }
    
    delay(100);

    while(digitalRead(BOTTOM) == 0) { delay(1); }
    digitalWrite(LED, 0);
    delay(500);
    digitalWrite(LED, 1);
    
    while(digitalRead(BOTTOM)) { delay(1); }
    
    for(int x = 0; x < 8; x++){
        delayMicroseconds(20);
        negro[x] = analogRead(Sensor[x]);
    }
    
    for(int x = 0; x < 8; x++){
        umbrales[x] = (blancos[x] + negro[x]) / 2;
    }
    digitalWrite(LED, 0);
    delay(1000);
    digitalWrite(LED, 1);

    while(digitalRead(BOTTOM) == 1) { delay(1); }


    preferences.begin("robot", false);
    preferences.putBytes("umbrales", umbrales, sizeof(umbrales));
    preferences.end();
    
    Serial.println("Calibración guardada exitosamente.");
    
    // mini indicador visual de finalización (3 parpadeos)
    for(int i = 0; i < 4; i++) {
        digitalWrite(LED, HIGH);
        delay(100);
        digitalWrite(LED, LOW); 
        delay(100);
    }
}

void menuInicio() {
    // modo 0 = Calibrar y Guardar (Parpadeo Rápido)
    // modo 1 = Cargar Calibración Guardada (Parpadeo Lento)
    int modo = 0; 
    
    Serial.println("=== MENÚ DE INICIO ===");
    Serial.println("Presiona el botón para alternar modo. Tras 3s de inactividad se confirmará.");

    unsigned long ultimoTiempoBoton = millis();
    const unsigned long tiempoInactividad = 3000; // 3 segundos para confirmar
    unsigned long ultimoParpadeo = 0;
    bool estadoLed = false;

    while (millis() - ultimoTiempoBoton < tiempoInactividad) {
        // --- Gestión del parpadeo del LED según el modo ---
        unsigned long intervaloBlink = (modo == 0) ? 150 : 600; // Rápido = Calibrar, Lento = Cargar
        
        if (millis() - ultimoParpadeo >= intervaloBlink) {
            estadoLed = !estadoLed;
            digitalWrite(LED, estadoLed);
            ultimoParpadeo = millis();
        }

        // --- Lectura del botón ---
        if (digitalRead(BOTTOM) == LOW) {
            delay(50); 
            if (digitalRead(BOTTOM) == LOW) {
                modo = !modo; // cambia entre 0 y 1

                // Reinicia el contador de los 3 segundos de inactividad
                ultimoTiempoBoton = millis();

                // Espera a que se suelte el botón para evitar múltiples cambios con una sola pulsación
                while (digitalRead(BOTTOM) == LOW) { delay(10); }
            }
        }
    }

    // Apagar LED y dar confirmación visual de selección
    digitalWrite(LED, LOW);

    // Ejecuta la acción elegida
    if (modo == 0) {
        calibrar();
    } else {
        cargarCalibracion();
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

// === NUEVA FUNCIÓN PARA REVISAR EL BOTÓN ===
void revisarBoton() {
  // Si el botón se presiona (LOW porque es INPUT_PULLUP)
  if (digitalRead(BOTTOM) == LOW) { 
    delay(50); // Antirebote de hardware por software (espera a que se estabilice)
    
    if (digitalRead(BOTTOM) == LOW) { // Confirmamos que sigue presionado
      kp += 0.05;
      
      // Feedback visual: Cambia el estado del LED para saber que entró al IF
      digitalWrite(LED, !digitalRead(LED)); 
      
      // Imprime en el monitor serie
      Serial.print("¡Botón detectado! Nuevo kp: ");
      Serial.println(kp);
      
      // Bucle de espera: se queda aquí hasta que SUELTES el botón
      // Esto evita que sume +0.05 infinitamente mientras lo dejas hundido
      while(digitalRead(BOTTOM) == LOW) {
        delay(10); 
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(BOTTOM, INPUT_PULLUP);
  pinMode(LED, OUTPUT);


 // calibrar();
  digitalWrite(LED, 0);

 menuInicio();

  digitalWrite(LED, LOW);
  delay(3000);
  digitalWrite(LED, HIGH);
  delay(2000);
  digitalWrite(LED, LOW);
  
motorcitos.moverMotores(95, 95);
delay(2000);

motorcitos.moverMotores(0, 0);


}

void loop() {
  // Revisamos si el botón ha sido presionado en cada ciclo
  revisarBoton();

  for(int x = 0; x < 8; x++){
    estado_booleano[x] = analogRead(Sensor[x]) > umbrales[x]? 0 : 1;
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
      motorcitos.moverMotores(velocidadIzquierda, velocidadDerecha);
  }
  pos = 0;
}