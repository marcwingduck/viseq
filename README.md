# VisEQ

**Visual Equalizer DIY Build**

![VisEQ](http://marclieser.de/data/content/interests/viseq/viseq_header.jpg)

The build was inspired by my love for music. The idea came when I read an article about [LEDmePlay](http://mithotronic.de/ledmeplay_constructionmanual.html). 
This is my first build, so there should be room for improvement.

Here is an early video of how it looks like:

[![Visual Equalizer](https://img.youtube.com/vi/bxcpWqqqpuE/0.jpg)](https://www.youtube.com/watch?v=bxcpWqqqpuE)

A short push to the momentary pushbutton toggles between one, two and four column-wide bar mode.
A long press resets the start animation to be displayed again.
A pot is used to change colors.
A main switch toggles overall power, two smaller switches can cut off power to the Arduino and the LED matrix respectively.

## Libraries

* Adafruit GFX Library
* RGB matrix panel
* fix_fft

All libraries can be installed using the Arduino IDE's Libray Manager.

## Components

* [Arduino Uno (Rev3)](https://store.arduino.cc/arduino-uno-rev3)
* [Medium 16x32 RGB LED Matrix Panel](https://www.adafruit.com/product/420)
* [MAX4466 Electret Microphone Amplifier](https://www.adafruit.com/product/1063)
* [USB Micro-B Breakout Board](https://www.adafruit.com/product/1833)
* [Ikea RIBBA 23x23](https://www.ikea.com/de/de/catalog/products/00378403/)
* 1 Power Switch
* 2 Smaller Switches
* 1 Momentary Pushbutton
* 1 10K Linear Potentiometer

## Connections

![Connections](http://www.marclieser.de/data/content/interests/viseq/viseq_connections.jpg)
