# WeatherStationPico
A data-logging weather station that uses a LCD display for user output, a potentiometer for user input, a DS3231 real time clock module and the DFRobot Weather Station v3.

The weather station is currently very buggy, and do not expect top-quality code, as I have not had years of experience with writing code for the Raspberry Pi Pico.

# Modifying the Code

In order to use the code, you will need to create a Pico Project, named WeatherStationPico. I recommend using a tool like the Raspberry Pi Pico Project Generator:  https://github.com/raspberrypi/pico-project-generator. You will need to download the Pico SDK and Pico Extras as well.

Compile the project for the first time, without the code, and then add in all of the code, and replace your CMakeLists.txt with the one specified in the project, replacing /path/to with your path to your Pico SDK and Pico Extras path.

# Required Parts

A Computer to Program the RPI Pico     RPI PiHut ordering page (examples)
A 3D Printer (or an order from a 3D Printing company)
A roll of Filament (For your 3D printer)
**1** Raspberry Pi Pico               https://thepihut.com/products/raspberry-pi-pico?variant=41925332566211

**1** Half size Breadboard            https://thepihut.com/products/raspberry-pi-breadboard-half-size

**1** DFRobot Weather Station         https://thepihut.com/products/weather-station-kit-with-anemometer-wind-vane-rain-bucket

**1** Rotary Encoder                  (link needed)

**1** I2C LCD 16x2                    https://thepihut.com/products/i2c-16x2-arduino-lcd-display-module

**1** DS3231 RTC Module               https://tinyurl.com/ds3231-amazon

**~20** Female-to-Female Jumper Wires https://thepihut.com/products/female-female-2-54-to-2-0mm-jumper-wires-x-40

**~20** Male-to-Female Jumper Wires   https://thepihut.com/products/premium-female-male-extension-jumper-wires-20-x-12

