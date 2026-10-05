/*
  EMISOR - Transmisión de texto en código Morse con láser infrarrojo
  Proyecto: Transmisión de información mediante luz invisible (FSO, modulación OOK)
  Teoría de Señales - Universidad de Manizales

  USO
   - Abrir el Monitor Serie a 9600 baudios con el ajuste "Nueva línea".
   - Escribir un mensaje y presionar Enter: se transmite por el láser IR.
   - Comandos especiales:
       #ON   -> deja el láser encendido fijo (sirve para alinear y medir la SNR)
       #OFF  -> apaga el láser fijo
   - (Opcional) Un pulsador en D2 funciona como manipulador Morse manual.

  CONEXIONES
   D8  -> resistencia 1 kΩ -> base del transistor NPN (2N2222 o BC547)
          colector del transistor -> negativo (GND) del módulo láser IR
          emisor del transistor   -> GND del Arduino
          positivo (VCC) del módulo láser -> 5V
   D13 -> LED indicador (sirve el LED de la placa). Muestra lo que se transmite,
          porque el haz infrarrojo no se ve.
   D2  -> pulsador -> GND   (opcional; usa la resistencia pull-up interna)

  TIEMPOS MORSE (estándar), con UNIDAD = duración de un punto:
   punto = 1 u | raya = 3 u | entre símbolos = 1 u | entre letras = 3 u | entre palabras = 7 u
*/

const byte PIN_LASER = 8;
const byte PIN_LED   = 13;
const byte PIN_BOTON = 2;

// Duración de un punto en milisegundos. DEBE ser igual en el receptor.
// 60 ms equivale a unas 20 palabras por minuto (PPM = 1200 / UNIDAD).
const unsigned int UNIDAD = 60;

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

bool laserFijo = false;

void laser(bool encendido) {
  digitalWrite(PIN_LASER, encendido ? HIGH : LOW);
  digitalWrite(PIN_LED, encendido ? HIGH : LOW);
}

const char* buscarCodigo(char c) {
  c = toupper(c);
  for (byte i = 0; i < N_TABLA; i++) {
    if (TABLA[i].c == c) return TABLA[i].codigo;
  }
  return nullptr;
}

void transmitirCaracter(const char* codigo) {
  for (byte i = 0; codigo[i] != '\0'; i++) {
    laser(true);
    delay(codigo[i] == '.' ? UNIDAD : 3 * UNIDAD);
    laser(false);
    delay(UNIDAD);              // espacio entre símbolos: 1 unidad
  }
  delay(2 * UNIDAD);            // completa las 3 unidades entre letras
}

void transmitirMensaje(const String& mensaje) {
  for (unsigned int i = 0; i < mensaje.length(); i++) {
    char c = mensaje[i];
    if (c == ' ') {
      delay(4 * UNIDAD);        // 3 + 4 = 7 unidades entre palabras
      continue;
    }
    const char* codigo = buscarCodigo(c);
    if (codigo == nullptr) {    // tildes, ñ, etc. no existen en Morse internacional
      Serial.print(F("  [caracter omitido: "));
      Serial.print(c);
      Serial.println(']');
      continue;
    }
    Serial.print(F("  "));
    Serial.print((char)toupper(c));
    Serial.print(F("  "));
    Serial.println(codigo);
    transmitirCaracter(codigo);
  }
}

void setup() {
  pinMode(PIN_LASER, OUTPUT);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BOTON, INPUT_PULLUP);
  laser(false);

  Serial.begin(9600);
  Serial.println(F("=== EMISOR MORSE IR ==="));
  Serial.print(F("Unidad: "));
  Serial.print(UNIDAD);
  Serial.print(F(" ms (~"));
  Serial.print(1200 / UNIDAD);
  Serial.println(F(" palabras por minuto)"));
  Serial.println(F("Escriba un mensaje y presione Enter. #ON / #OFF = laser fijo."));
}

void loop() {
  if (Serial.available()) {
    String mensaje = Serial.readStringUntil('\n');
    mensaje.trim();

    if (mensaje.equalsIgnoreCase("#ON")) {
      laserFijo = true;
      Serial.println(F(">> Laser encendido fijo"));
    } else if (mensaje.equalsIgnoreCase("#OFF")) {
      laserFijo = false;
      Serial.println(F(">> Laser apagado"));
    } else if (mensaje.length() > 0) {
      laserFijo = false;
      Serial.print(F(">> Transmitiendo: "));
      Serial.println(mensaje);
      unsigned long inicio = millis();
      transmitirMensaje(mensaje);
      Serial.print(F(">> Listo en "));
      Serial.print(millis() - inicio);
      Serial.println(F(" ms"));
    }
  }

  // Láser fijo o manipulador manual: mientras se presiona el botón, el láser emite.
  laser(laserFijo || digitalRead(PIN_BOTON) == LOW);
}
