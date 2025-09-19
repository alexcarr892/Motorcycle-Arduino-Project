# Motorcycle-Arduino-Project

This project uses IR temp sensors pointed at motorcycle tires to read their temperatures. This shows the temps in real time on the LCD display and also graphs the temps over time on a web server hosted by the Arduino. This project is dependent on having a mobile hotspot for the Arduino to connect to, so the web server can be functional. Doing track days, it's helpful information to see how much heat you're getting in your tires to determine if you're in need of a more track-capable tire.


MATERIALS NEEDED:
ESP32S3 Development Board
MLX90614 IR temp sensor X2
TCA 9548A Multiplexer
16x2 LCD display
BreadBoard
Wire
Mobile Hotspot device (ex. Smartphone)
Arduino IDE

REFER TO THE WIRING DIAGRAM IMAGE TO CORRECTLY WIRE ALL THE DEVICES TOGETHER.

I did this project on a budget, but I would recommend using a total of 4 more temp sensors having 3 total sensors per tire, to get a more accurate reading of the tire since you're not riding on one part of the tire all the time. The Multiplexor allows for the addition of extra sensors, so the upgrade would be simple.

