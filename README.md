# WeatherStationPico
A data-logging weather station that uses a LCD display for user output, a potentiometer for user input, a DS3231 real time clock module and the DFRobot Weather Station v3.

The weather station is currently very buggy, and do not expect top-quality code, as I have not had years of experience with writing code for the Raspberry Pi Pico.

# Modifying the Code

In order to use the code, you will need to create a Pico Project, named WeatherStationPico. I recommend using a tool like the Raspberry Pi Pico Project Generator:  https://github.com/raspberrypi/pico-project-generator. You will need to download the Pico SDK and Pico Extras as well.

Compile the project for the first time, without the code, and then add in all of the code, and replace your CMakeLists.txt with the one specified in the project, replacing /path/to with your path to your Pico SDK and Pico Extras path.

