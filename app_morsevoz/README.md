# App MorseVoz (Android)

App nativa en **Kotlin** que capta la voz, la convierte a texto y lo envía por **Bluetooth LE** al módulo del Arduino transmisor, que lo emite en Morse por luz infrarroja.

## Por qué nativa y BLE
- **App Inventor no sirvió** en Android 16: no instalaba el APK y su Bluetooth fallaba.
- El módulo que se usa **no es un HC-05 clásico, es BLE** (aparece en la pestaña "Bluetooth LE", no en "Classic"). Por eso la app usa GATT sobre BLE y escribe en la característica **FFE1** del servicio **FFE0** (UART transparente); si no la encuentra, usa la primera característica escribible.

## Archivos
| Archivo | Qué es |
|---|---|
| `MorseVoz.apk` | App lista para instalar (debug) |
| `MainActivity.kt` | Lógica: permisos, conexión BLE, voz y envío |
| `activity_main.xml` | Interfaz (botones Conectar, Hablar, Enviar) |
| `AndroidManifest.xml` | Permisos (Bluetooth, escaneo, micrófono/internet) |

## Instalar el APK
Por USB con depuración activada:
```bash
adb install -r MorseVoz.apk
```
O copiar el `.apk` al celular e instalarlo desde "Mis archivos".

## Volver a compilar (sin Android Studio)
Requiere un JDK 17, el Android SDK (plataforma android-35, build-tools) y Gradle. Estructura de proyecto estándar; se construye con:
```bash
gradle assembleDebug
```
El APK queda en `app/build/outputs/apk/debug/app-debug.apk`.
(En este proyecto se compiló por línea de comandos con Gradle 8.9, AGP 8.6, Kotlin 1.9, compileSdk 35, targetSdk 34.)

## Uso
1. Emparejar el módulo en Ajustes → Bluetooth del celular.
2. Abrir MorseVoz → **Conectar al HC-05** (aceptar permiso) → "Conectado ✅".
3. **🎤 Hablar** → decir la frase → **Enviar**.
4. El mensaje sale en Morse por el LED IR y aparece en la LCD del receptor.

> Necesita internet (wifi/datos) para el reconocimiento de voz de Android.
