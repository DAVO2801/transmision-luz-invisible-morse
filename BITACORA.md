# Bitácora del proyecto

**Transmisión de información mediante luz invisible (código Morse con control remoto infrarrojo)**
Teoría de Señales — Universidad de Manizales
Integrantes: Juan Felipe Orozco · Daniel Villegas Toro · Juan Miguel Gómez — Docente: Mateo Cardona Marín

Este documento resume todo lo que se ha hecho en el proyecto: cómo evolucionó la idea, cómo se arma, cómo se usa, los problemas que aparecieron y cómo explicarlo en la exposición.

---

## 1. Resumen

Un **control remoto infrarrojo** emite luz que el ojo humano no puede ver. Cada pulsación del control representa un símbolo del **código Morse** (corta = punto, larga = raya). Un **receptor infrarrojo** conectado a un **Arduino Uno** detecta la señal; el Arduino mide la duración de cada pulsación, la traduce a letras y muestra el mensaje en una **pantalla LCD 16x2**.

---

## 2. Evolución del proyecto

| Fecha | Qué se hizo |
|---|---|
| 05/10/2026 | Se revisó el marco teórico inicial: la propuesta era transmitir Morse con un **diodo láser infrarrojo** y recibirlo con un **fotodiodo PIN**. Se escribió el código del emisor y del receptor para esa versión (dos Arduinos) y se creó el repositorio. |
| 05/10/2026 | Se instaló el Arduino IDE y se comprobó que los programas compilan. |
| 07/10/2026 | Se replanteó el proyecto: usar **un solo Arduino** y un **control remoto IR** en lugar del láser, mostrando el mensaje en una **pantalla LCD**. |
| 08/10/2026 | Se escribió el nuevo programa (`morse_ir_lcd`), se actualizó el marco teórico (resumen, objetivos, definiciones, materiales y desarrollos) y el README. La versión con láser se conservó en `version_laser/`. |
| 19/10/2026 | Se armó el circuito físico (receptor, LCD, buzzer y LED) y se hicieron las primeras pruebas con mensajes. |

**¿Por qué se cambió del láser al control remoto?**

- Necesita un solo Arduino (más barato).
- No hay que alinear el haz con precisión.
- Los componentes vienen en la mayoría de kits de Arduino.
- La portadora de 38 kHz del control permite explicar modulación, filtrado y ruido, que son temas centrales de la materia.
- La demostración es más visual porque el mensaje aparece en pantalla.

Desventaja: se pierde la medición analógica de la relación señal/ruido que permitía el fotodiodo, porque el receptor IR entrega solo 0 o 1.

> **Pendiente:** confirmar con el profesor que acepta el cambio del láser por el control remoto.

---

## 3. Cómo funciona

```
 Mensaje → Código Morse → Control remoto → Aire → Receptor IR → Arduino → LCD
 (fuente)  (codificación)  (modulación)   (canal) (demodulación) (decodif.) (destino)
```

1. **Codificación:** cada letra se convierte en puntos y rayas. La información está en la *duración* de cada pulso.
2. **Modulación:** al presionar un botón, el LED infrarrojo del control (unos 940 nm) se enciende y apaga **38.000 veces por segundo** (portadora de 38 kHz). Hay portadora mientras el botón está presionado y no la hay cuando se suelta → modulación **OOK** (On-Off Keying).
3. **Canal:** la luz viaja por el aire. El sol y los bombillos agregan **ruido** infrarrojo.
4. **Demodulación:** el receptor VS1838B tiene un fotodiodo, un amplificador, un **filtro pasabanda de 38 kHz** y un demodulador. Solo responde a la luz que parpadea a 38 kHz, así que ignora la luz ambiente, y entrega una señal digital limpia.
5. **Decodificación:** el Arduino mide cuánto dura cada pulsación:
   - menos de 0,3 s → punto
   - más de 0,3 s → raya
   - 0,8 s sin pulsar → termina la letra
   - 2,5 s sin pulsar → espacio entre palabras
6. **Destino:** el mensaje aparece en la LCD.

Detalle técnico: mientras el botón se mantiene presionado, el control (protocolo NEC) repite su señal cada ~110 ms. El Arduino considera que se soltó cuando pasan 160 ms sin recibir nada.

---

## 4. Materiales

| Material | Cant. | Nota |
|---|---|---|
| Arduino Uno + cable USB | 1 | |
| Control remoto infrarrojo | 1 | El de un kit de Arduino o uno de TV |
| Receptor IR VS1838B / TSOP1838 (o módulo KY-022) | 1 | |
| Pantalla LCD 16x2 con módulo I2C | 1 | Debe tener la plaquita negra de 4 pines atrás |
| Protoboard | 1 | |
| Cables jumper macho-macho | ~10 | |
| Cables jumper **hembra-macho** | ~7 | Para la LCD y los módulos con pines macho |
| Módulo buzzer (opcional) | 1 | Se usó un módulo MH-FMG (GND, I/O, VCC) |
| LED + resistencia de 220 Ω (opcional) | 1 | Rojo-rojo-café; sirven también 330 Ω o 1 kΩ |

---

## 5. Montaje paso a paso

> **Regla de oro:** arma todo con el USB **desconectado**. Nunca muevas cables con el Arduino encendido.

### 5.1 Alimentación en la protoboard

La protoboard usada no tiene marcas de colores, así que se definió:

| Línea de la protoboard | Uso |
|---|---|
| **Primera fila de arriba** | **GND** (negativo) → cable al pin GND del Arduino |
| Segunda fila de arriba | **No usar** |
| Penúltima fila de abajo | **No usar** |
| **Última fila de abajo** | **5V** (positivo) → cable al pin 5V del Arduino |

Conviene marcar las filas con cinta o marcador. Las filas "no usar" no están conectadas a nada.

### 5.2 Receptor infrarrojo

Componente suelto, con **la burbuja mirando hacia uno** y las patas hacia abajo, de izquierda a derecha:

| Pata | Función | Va a |
|---|---|---|
| Izquierda | OUT | Pin **2** del Arduino |
| Centro | GND | Primera fila de arriba (GND) |
| Derecha | VCC | Última fila de abajo (5V) |

Ejemplo: patas en la fila j, columnas 10, 11 y 12, y un cable desde cada columna a su destino.
Si es el módulo KY-022, guiarse por las letras impresas: **S** → pin 2, **+** → 5V, **−** → GND.

⚠️ Invertir GND y VCC daña el receptor. Al conectar el USB, si se calienta, desconectar de inmediato.

### 5.3 Pantalla LCD (módulo I2C)

| Pin LCD | Va a |
|---|---|
| GND | Primera fila de arriba (GND) |
| VCC | Última fila de abajo (5V) |
| SDA | Pin **A4** del Arduino |
| SCL | Pin **A5** del Arduino |

### 5.4 Buzzer (módulo MH-FMG, opcional)

El módulo dice 高电平触发 ("se activa con nivel alto"): suena cuando el pin está en HIGH.

| Pin buzzer | Va a |
|---|---|
| GND | Primera fila de arriba (GND) |
| I/O | Pin **8** del Arduino |
| VCC | Última fila de abajo (5V) |

Prueba: con el Arduino encendido, desconectar I/O del pin 8 y tocar con él la fila de 5V. Pitido continuo = buzzer activo (funciona con el código). Solo un "clic" = pasivo (hay que ajustar el código).

### 5.5 LED externo (opcional)

El Arduino ya tiene un LED integrado ("L", pin 13) que parpadea con cada señal IR. Para uno más visible:

```
Pin 13 → resistencia 220 Ω → pata LARGA del LED
                              pata CORTA del LED → GND (primera fila de arriba)
```

Nunca conectar el LED sin resistencia. Si queda al revés no se daña, solo no prende.

### 5.6 Revisión antes de encender

- [ ] Ningún cable une la fila de 5V con la de GND.
- [ ] Receptor: OUT → pin 2, GND → GND, VCC → 5V.
- [ ] LCD: SDA → A4, SCL → A5.
- [ ] Buzzer: I/O → pin 8.

---

## 6. Software

1. Abrir **Arduino IDE 2**.
2. Instalar las librerías (Herramientas → Gestionar librerías): **IRremote** (Armin Joachimsmeyer, v4.x) y **LiquidCrystal I2C** (Frank de Brabander).
3. Abrir `morse_ir_lcd/morse_ir_lcd.ino`.
4. Elegir placa **Arduino Uno** y el puerto COM.
5. Presionar **Subir** (→).

Al arrancar, la LCD muestra:

```
Morse por luz IR
Presione el ctrl
```

El programa queda guardado en el Arduino: al desconectarlo y volverlo a conectar arranca solo. Para apagarlo basta con desconectar el USB. También funciona con un cargador de celular o un power bank.

---

## 7. Uso

| Acción | Cómo |
|---|---|
| Punto | Toque rápido |
| Raya | Mantener presionado ~0,5 s |
| Terminar letra | Esperar 1 s |
| Espacio | Botón **>\|\|** (o esperar 2,5 s) |
| Borrar | Botón **\|<<** |
| Limpiar pantalla | Botón **CH-** |

La segunda fila de la LCD muestra los símbolos en curso y la letra que formarían (ej. `.-.     >R`).
El Monitor Serie (9600 baudios) muestra el código de cada botón y la duración de cada pulsación.

### Ejemplos

| Palabra | Morse |
|---|---|
| SOS | `...` `---` `...` |
| HOLA | `....` `---` `.-..` `.-` |
| INFINITO | `..` `-.` `..-.` `..` `-.` `..` `-` `---` |

La LCD muestra los últimos 16 caracteres; los mensajes más largos se desplazan hacia la izquierda.

---

## 8. Problemas encontrados y soluciones

| Problema | Solución |
|---|---|
| La protoboard no tiene marcas de + y − | Se eligió la primera fila de arriba como GND y la última de abajo como 5V; las otras dos filas de borde no se usan. |
| La LCD y los módulos tienen pines macho y los cables eran macho-macho | Usar cables hembra-macho, o clavar los pines del módulo en la protoboard y sacar los cables desde esas columnas. |
| No se sabía si el buzzer era activo o pasivo | Prueba tocando I/O con 5V: pitido continuo = activo. |
| No se sabía cómo conectar el LED | Pin 13 → 220 Ω → pata larga; pata corta → GND. Es opcional. |
| La LCD enciende pero no muestra texto | Girar el potenciómetro azul de atrás (contraste); si sigue igual, cambiar `DIRECCION_LCD` de `0x27` a `0x3F`. |
| Los botones de función escriben Morse | El control usa otros códigos: verlos en el Monitor Serie y cambiar `BOTON_ESPACIO`, `BOTON_BORRAR` y `BOTON_LIMPIAR`. |
| Todo sale como punto o todo como raya | Ajustar `UMBRAL_RAYA` en el código. |
| Se usa buzzer pasivo con `tone()` | No funciona: la librería IRremote usa el mismo temporizador. Usar buzzer activo. |
| No se ve si el control emite | Mirar el LED del control con la cámara frontal del celular. |

---

## 9. Para la exposición

### Explicación corta (~30 s)

> "Nuestro proyecto transmite mensajes usando luz infrarroja, que el ojo humano no puede ver. Cada letra se codifica en Morse, y cada punto o raya se envía como una pulsación corta o larga del control remoto. El control no emite luz continua: la modula a 38 kHz, y el receptor, que tiene un filtro sintonizado en esa frecuencia, separa nuestra señal del ruido de la luz ambiente. El receptor demodula la señal y el Arduino mide la duración de cada pulso, la traduce a letras y muestra el mensaje en la pantalla."

### Demostración sugerida

1. Apuntar el control a la cámara de un celular: se ve el LED infrarrojo parpadear.
2. Escribir **SOS** con el control y mostrar cómo aparece en la LCD.
3. Tapar el receptor con la mano: la señal no llega (la luz necesita línea de vista).

### Conceptos de Teoría de Señales

| Concepto | En el proyecto |
|---|---|
| Señal portadora | 38 kHz del LED del control |
| Modulación OOK | Encender/apagar la portadora para formar pulsos |
| Filtro pasabanda | El receptor solo acepta 38 kHz |
| Demodulación | El receptor quita la portadora y deja los pulsos |
| Ruido y SNR | Sol y bombillos; la portadora ayuda a ignorarlos |
| Codificación de fuente | Morse: letras → duraciones |
| Medición en el tiempo | El Arduino mide duraciones con su reloj interno |

---

## 10. Pendientes

- [ ] Confirmar con el profesor el cambio de láser a control remoto.
- [ ] Agregar al Word fotos del control, el receptor IR y la LCD (se quitaron las del láser y el filtro).
- [ ] Verificar que existen las 3 referencias originales del marco teórico (Gómez & Rodríguez 2023; Haas & Chen 2022; Martínez et al. 2024).
- [ ] Hacer las pruebas del plan (precisión, alcance, ángulo, luz ambiental) y registrar resultados en la sección 6 del Word.
- [ ] Escribir las conclusiones (sección 7 del Word).

---

## 11. Archivos del repositorio

| Ruta | Contenido |
|---|---|
| `morse_ir_lcd/` | Programa principal (control IR + LCD) |
| `version_laser/` | Versión alternativa con láser IR y fotodiodo (dos Arduinos) |
| `docs/` | Marco teórico en Word |
| `README.md` | Descripción técnica, conexiones y materiales |
| `BITACORA.md` | Este documento |
