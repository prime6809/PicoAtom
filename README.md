# PicoAtom
Simple Acorn Atom emulator running on the Raspbery Pi Pico.

Using an ILI9341 display, SD card and PS/2 keyboard.

Connections :

Pico pin | Purpose
-------------------
1 | Serial Tx for debug
2 | Serial Rx for debug
3 | Serial GND
4	 |		Display SCK
5  |		Display MOSI
6	 |		Display MISO
7	 |		Display CS
8	 |		GND
9	 |		Display CD (Command/Data)
 10			Display reset
* 11			PS/2 Clock **NOTE** Via 5.0V -> 3.3V level shifter
* 12			PS/2 Data **NOTE** Via 5.0V -> 3.3V level shifter
* 13			GND
* 14			SD SCK
* 15			SD MOSI
* 16			SD MISO
* 17			SD CS
* 18			GND
* 19			Speaker out
* 20

* 21
* 22
* 23			GND
* 24
* 25
* 26
* 27
* 28			GND
* 29
* 30
* 31
* 32
* 33			GND
* 34
* 35			
* 36			3.3V output to Level shifter, Display and SD card
* 37
* 38			GND
* 39			5V output to PS/2 (and level shifter).
* 40

All pins not explicitly specified are no connection.

It is suggested that a bidirectional level shifter is uded between the PS/2 keyboard and the Pico.
This allows the Pico to send commands to the keyboard such as toggling LEDs etc.
