/*
  MORSE POR LUZ INFRARROJA CON CONTROL REMOTO Y PANTALLA LCD
  Proyecto: Transmisión de información mediante luz invisible
  Teoría de Señales - Universidad de Manizales

  FUNCIONAMIENTO
   - El control remoto emite luz infrarroja (invisible) modulada a 38 kHz.
   - El receptor IR (VS1838B / TSOP1838) la detecta y el Arduino mide cuánto
     tiempo se mantiene presionado el botón:
       pulsación corta = punto (.)    pulsación larga = raya (-)
   - Tras una pausa se decodifica la letra y se muestra en la LCD 16x2.
   - Tras una pausa más larga se agrega un espacio entre palabras.

  BOTONES (configurables abajo; los valores son los del control típico de los
  kits de Arduino). Cualquier otro botón funciona como tecla Morse.
   BOTON_ESPACIO -> termina la letra y agrega un espacio
   BOTON_BORRAR  -> borra los símbolos en curso o, si no hay, el último carácter
   BOTON_LIMPIAR -> borra toda la pantalla
  Para conocer los códigos de otro control: abrir el Monitor Serie (9600 baudios)
  y presionar cada botón; se imprime su protocolo y su código.

  CONEXIONES
   Receptor IR (VS1838B / TSOP1838 / módulo KY-022):
     OUT -> D2    VCC -> 5V    GND -> GND
   LCD 16x2 con módulo I2C:
     SDA -> A4    SCL -> A5    VCC -> 5V    GND -> GND
   Buzzer ACTIVO (opcional): + -> D8, - -> GND
     (no usar buzzer pasivo con tone(): la librería IRremote ocupa el mismo
      temporizador)
   El LED de la placa (D13) parpadea cada vez que llega una señal IR.

  LIBRERÍAS (Arduino IDE -> Gestor de librerías)
   - IRremote (Armin Joachimsmeyer), versión 4.x
   - LiquidCrystal I2C (Frank de Brabander)
*/

#include <IRremote.hpp>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const byte PIN_IR     = 2;
const byte PIN_BUZZER = 8;

// Dirección I2C de la pantalla: 0x27 es la más común; algunas usan 0x3F.
const byte DIRECCION_LCD = 0x27;

// Tiempos en milisegundos
const unsigned long TIEMPO_SOLTAR = 160;  // sin señal este tiempo = botón soltado
const unsigned long UMBRAL_RAYA   = 300;  // presionado menos = punto, más = raya
const unsigned long PAUSA_LETRA   = 800;  // silencio que termina una letra
const unsigned long PAUSA_PALABRA = 2500; // silencio que agrega un espacio (0 = desactivado)

// Códigos de los botones de función (control de los kits de Arduino, protocolo NEC)
const uint16_t BOTON_ESPACIO = 0x43;  // >||
const uint16_t BOTON_BORRAR  = 0x44;  // |<<
const uint16_t BOTON_LIMPIAR = 0x45;  // CH-

struct Morse { char c; const char* codigo; };

const Morse TABLA[] = {
  {'A', ".-"},     {'B', "-..."},   {'C', "-.-."},   {'D', "-.."},    {'E', "."},
  {'F', "..-."},   {'G', "--."},    {'H', "...."},   {'I', ".."},     {'J', ".---"},
  {'K', "-.-"},    {'L', ".-.."},   {'M', "--"},     {'N', "-."},     {'O', "---"},
  {'P', ".--."},   {'Q', "--.-"},   {'R', ".-."},    {'S', "..."},    {'T', "-"},
  {'U', "..-"},    {'V', "...-"},   {'W', ".--"},    {'X', "-..-"},   {'Y', "-.--"},
  {'Z', "--.."},
  {'0', "-----"},  {'1', ".----"},  {'2', "..---"},  {'3', "...--"},  {'4', "....-"},
  {'5', "....."},  {'6', "-...."},  {'7', "--..."},  {'8', "---.."},  {'9', "----."},
  {'.', ".-.-.-"}, {',', "--..--"}, {'?', "..--.."}, {'!', "-.-.--"},
  {'/', "-..-."},  {'=', "-...-"},  {'-', "-....-"}, {'@', ".--.-."}
};
const byte N_TABLA = sizeof(TABLA) / sizeof(TABLA[0]);

LiquidCrystal_I2C lcd(DIRECCION_LCD, 16, 2);

// Mensaje decodificado
const byte MAX_MENSAJE = 64;
char mensaje[MAX_MENSAJE + 1];
byte largo = 0;

// Símbolos de la letra en curso
char simbolos[8];
byte nSimbolos = 0;
bool desbordado = false;

// Estado de la pulsación
bool presionado = false;
unsigned long tInicio = 0;
unsigned long tUltimaSenal = 0;
unsigned long tSoltado = 0;
bool espacioPendiente = false;

// ---------- Decodificación ----------

char decodificar() {
  if (nSimbolos == 0) return ' ';
  if (desbordado) return '*';
  simbolos[nSimbolos] = '\0';
  for (byte i = 0; i < N_TABLA; i++) {
    if (strcmp(TABLA[i].codigo, simbolos) == 0) return TABLA[i].c;
  }
  return '*';                   // secuencia que no corresponde a ningún carácter
}

// ---------- Pantalla ----------

void mostrarMensaje() {
  // Muestra los últimos 16 caracteres en la primera línea
  byte inicio = largo > 16 ? largo - 16 : 0;
  lcd.setCursor(0, 0);
  for (byte i = 0; i < 16; i++) {
    byte k = inicio + i;
    lcd.print(k < largo ? mensaje[k] : ' ');
  }
}

void mostrarSimbolos() {
  // Segunda línea: símbolos en curso y la letra que formarían, ej. ".-.     > R"
  lcd.setCursor(0, 1);
  for (byte i = 0; i < 8; i++) {
    lcd.print(i < nSimbolos ? simbolos[i] : ' ');
  }
  lcd.print(F("      "));
  lcd.setCursor(14, 1);
  if (nSimbolos > 0) {
    lcd.print('>');
    lcd.print(decodificar());
  } else {
    lcd.print(F("  "));
  }
}

// ---------- Edición del mensaje ----------

void agregarCaracter(char c) {
  if (c == ' ' && (largo == 0 || mensaje[largo - 1] == ' ')) return;
  if (largo == MAX_MENSAJE) {   // mensaje lleno: se descarta el carácter más antiguo
    memmove(mensaje, mensaje + 1, MAX_MENSAJE - 1);
    largo--;
  }
  mensaje[largo++] = c;
  mensaje[largo] = '\0';
  Serial.print(F("Texto: "));
  Serial.println(mensaje);
  mostrarMensaje();
}

void terminarLetra() {
  if (nSimbolos == 0) return;
  simbolos[nSimbolos] = '\0';
  char c = decodificar();
  Serial.print(F("Letra: "));
  Serial.print(simbolos);
  Serial.print(F(" = "));
  Serial.println(c);
  nSimbolos = 0;
  desbordado = false;
  agregarCaracter(c);
  mostrarSimbolos();
  espacioPendiente = true;
}

void borrar() {
  if (nSimbolos > 0) {
    nSimbolos = 0;
    desbordado = false;
  } else if (largo > 0) {
    mensaje[--largo] = '\0';
  }
  espacioPendiente = false;
  mostrarMensaje();
  mostrarSimbolos();
}

void limpiar() {
  largo = 0;
  mensaje[0] = '\0';
  nSimbolos = 0;
  desbordado = false;
  espacioPendiente = false;
  lcd.clear();
  Serial.println(F("Pantalla borrada"));
}

// ---------- Pulsaciones del control ----------

void nuevaPulsacion(unsigned long ahora) {
  presionado = true;
  tInicio = ahora;
  tUltimaSenal = ahora;
  espacioPendiente = false;
  digitalWrite(PIN_BUZZER, HIGH);
}

void soltar() {
  presionado = false;
  digitalWrite(PIN_BUZZER, LOW);
  tSoltado = tUltimaSenal;

  unsigned long duracion = tUltimaSenal - tInicio;
  char simbolo = (duracion < UMBRAL_RAYA) ? '.' : '-';
  if (nSimbolos < 7) simbolos[nSimbolos++] = simbolo;
  else desbordado = true;

  Serial.print(F("Pulsacion de "));
  Serial.print(duracion);
  Serial.print(F(" ms = "));
  Serial.println(simbolo);
  mostrarSimbolos();
}

void procesarSenal(unsigned long ahora) {
  IRData& datos = IrReceiver.decodedIRData;
  if (datos.protocol == UNKNOWN) return;   // ruido o protocolo no reconocido

  if (presionado) {                        // el botón sigue presionado
    tUltimaSenal = ahora;
    return;
  }
  if (datos.flags & IRDATA_FLAGS_IS_REPEAT) return;  // repetición de un botón ya atendido

  Serial.print(F("Boton: protocolo "));
  Serial.print(getProtocolString(datos.protocol));
  Serial.print(F(", codigo 0x"));
  Serial.println(datos.command, HEX);

  if (datos.command == BOTON_ESPACIO) {
    terminarLetra();
    agregarCaracter(' ');
    espacioPendiente = false;
  } else if (datos.command == BOTON_BORRAR) {
    borrar();
  } else if (datos.command == BOTON_LIMPIAR) {
    limpiar();
  } else {
    nuevaPulsacion(ahora);
  }
}

void revisarPausas(unsigned long ahora) {
  unsigned long silencio = ahora - tSoltado;
  if (nSimbolos > 0 && silencio >= PAUSA_LETRA) terminarLetra();
  if (espacioPendiente && PAUSA_PALABRA > 0 && silencio >= PAUSA_PALABRA) {
    agregarCaracter(' ');
    espacioPendiente = false;
  }
}

void setup() {
  pinMode(PIN_BUZZER, OUTPUT);
  Serial.begin(9600);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print(F("Morse por luz IR"));
  lcd.setCursor(0, 1);
  lcd.print(F("Presione el ctrl"));

  IrReceiver.begin(PIN_IR, ENABLE_LED_FEEDBACK);

  Serial.println(F("=== MORSE IR + LCD ==="));
  Serial.println(F("Corta = punto, larga = raya. Cada boton muestra su codigo aqui."));
}

void loop() {
  unsigned long ahora = millis();

  if (IrReceiver.decode()) {
    if (largo == 0 && nSimbolos == 0 && !presionado) lcd.clear();  // quita la bienvenida
    procesarSenal(ahora);
    IrReceiver.resume();
  }

  if (presionado && ahora - tUltimaSenal > TIEMPO_SOLTAR) soltar();
  if (!presionado) revisarPausas(ahora);
}
