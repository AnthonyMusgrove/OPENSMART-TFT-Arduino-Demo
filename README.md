# OPENSMART-TFT-Arduino-Demo
Demonstration of the OPENSMART TFT 2.8" with Arduino Uno

Recently I purchased a very cheap OPENSMART TFT 2.8" touch screen from Wish.  The display IC controller is the ILI9320.  After a bit of research and experimenting (Because there was absolutely no documentation with it), it works. I just thought I'd share my code and experience incase anyone is interested.  For this particular board/IC, you need to use a modified version of the MCUFriend TFT library.

** You also need to install 'Adafruit GFX' library and the 'Adafruit Touch Screen' library via the Library Manager

CUSTOM LIBRARY 
Download as ZIP, and install it via Sketch -> Include Library -> Add .ZIP Library:
https://github.com/fdufnews/OPENSMART_TFT

For this demo, I am using an Arduino Nano R3, with the Open-Smart TFT 2.8" shield.  This includes a micro-sd card slot, which I will incorporate into this demo at a later stage.

