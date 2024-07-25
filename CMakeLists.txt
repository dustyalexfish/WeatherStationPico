# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
set(PICO_SDK_PATH "/path/to/pico-sdk") // Change this to your path to the Pico SDK
set(PICO_EXTRAS_PATH "/path/to/pico-extras") #Pico Extras is required for Deep Slee which I plan to add in the future

set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)

include(path/to/include/sdk_import)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.4.0")
  message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.4.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()


include(/path/to/pico-extras) # Change this to your Pico Extras include file

project(WeatherStationPico C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(WeatherStationPico WeatherStation.c )

pico_set_program_name(WeatherStationPico "WeatherStation")
pico_set_program_version(WeatherStationPico "0.1")

pico_enable_stdio_uart(WeatherStationPico 1)
pico_enable_stdio_usb(WeatherStationPico 1)

# Add the standard library to the build
target_link_libraries(WeatherStationPico
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(WeatherStationPico PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts or any other standard includes, if required
)

# Add any user requested libraries
target_link_libraries(WeatherStationPico 
        pico_stdlib
        hardware_rtc
        pico_multicore
        hardware_i2c
        hardware_flash
)

pico_add_extra_outputs(WeatherStationPico)

