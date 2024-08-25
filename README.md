# WeatherStationPico
A data-logging weather station that uses a LCD display for user output, a potentiometer for user input, a DS3231 real time clock module and the DFRobot Weather Station v3.

The weather station is currently very buggy, and do not expect top-quality code, as I have not had years of experience with writing code for the Raspberry Pi Pico.

# Modifying the Code

In order to use the code, you will need to create a Pico Project, named WeatherStationPico. I recommend using a tool like the Raspberry Pi Pico Project Generator:  https://github.com/raspberrypi/pico-project-generator. You will need to download the Pico SDK and Pico Extras as well.

Compile the project for the first time, without the code, and then add in all of the code, and replace your CMakeLists.txt with the one specified in the project, replacing /path/to with your path to your Pico SDK and Pico Extras path.

# Required Parts

A Computer to Program the RPI Pico 

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

**~20** M2.5 Screws and Nuts          https://thepihut.com/products/adafruit-black-nylon-screw-and-stand-off-set-m2-5-thread

A way to power the Raspberry Pi Pico (5V power required). I am using a Solar Panel to power my Weather Station.

# 3D Printing

In order to use this project, you will have to print the project box. The STL files and FreeCAD files are provided in the 3dmodels file for you to 3D print. I recommend printing out of ABS of PETG as PLA may warp in sunlight, unless you plan to keep the project box in the shade (which is possible, as the cables are quite long).

# Wiring

**LCD** Wire the LCD VCC-5V pin to your Raspberry Pi Pico 5V VBUS pin. Wire the GND pin to one of your Raspberry Pi Pico GND pins. Wire the SDA pin of your I2C lcd to GPIO6 on the Raspberry Pi Pico. Wire SCL on your LCD to GPIO7 on your Raspberry Pi Pico. 

**Rotary Encoder** Wire the CLK pin of your Rotary Encoder to GPIO pin two. Wire the DT pin to GPIO3. Wire the SW pin to GPIO4. Attach the + to your VBUS pin and the GND pin to one of the Raspberry Pi Pico's Ground pins.

**Weather Station** Wire the Weather Station Board TX pin to the Raspberry Pi Pico GPIO1. Wire the Weather Station Board RX pin to Raspberry Pi Pico GPIO0. Attach the 5V pin to VBUS, and the GND pin to one of the Raspberry Pi Pico's GND pins.

**DS3231 Module** Wire the I2C SDA pn to GPIO6 and the SCL pin to GPIO7. Attach the + pin to 5V and the GND pin to one of the Raspberry Pi Pico GND pins.

# Assembly

Once you have 3D printed the top lid of the case, you can screw in the LCD display with M2.5 screws to fix it in place. You will have to superglue the Rotary Encoder in place to the model. 

Wire all of the wires that exit the Weather Station board to exit through the holes in the top half of the project box. 

Once you have done this, plug in the Raspberry Pi Pico in to your PC, and upload the UF2 file provided (builds/WeatherStation-1.1) to your Raspberry Pi Pico. Screw in the lid, and you are now recording your own weather!

# Operation

In order to erase all data stored on Flash, hold down the potentiometer until the text 'Erase' appears on the display. Leave it to erase, and then reapply power to the device. In order to view previous logs, hold the potentiometer until two dots appear on the screen, and then scroll through logs that are stored on the system using the rotary encoder. For debug, hold down the rotary encoder until about 6 dots appear, and let go. You must be using a serial monitor to access the contents via USB, like Minicom. The system will dump data in flash in blocks of 256 to serial to access.

You now have a working Weather Station!
