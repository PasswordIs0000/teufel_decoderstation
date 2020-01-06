# Teufel Decoderstation

This project uses an Arduino Uno (with Ethernet shield) as an IR remote for the Teufel Decoderstation 5. Control is possible using one button by counting the clicks or using a simple webinterface in the browser.

Libraries used:
- Bounce2
- EasyWebServer
- Ethernet
- IRremote
- SPI

Hardware layout:
- PIN 3: IR LED for sending
- PIN 5: Button for user input
- PIN 9: IR receiver (38 kHz)
