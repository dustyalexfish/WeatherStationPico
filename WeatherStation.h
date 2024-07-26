#include <stdio.h>
#include <stdlib.h>
#pragma once

char dataRecorded[15];
char buffer[35];
unsigned int wstime = 2;

//Uncompresed, the Pico's flash can store this information for up to ~1 year
// However, the data is Compressed, so more time is expected
/* Data Recorded Structure

ROTATION (1 bytes)                           [0]
Highest Wind Speed (5minutes) (1 bytes)      [1]

Avg Wind Speed (1minute) (1 bytes)           [2]
Temperature (1 byte)                         [3]
Rainfall in 24h (2 bytes)                    [4-5]
Rainfall in 1hour (2 bytes)                  [6-7]
Air Pressure (2 bytes)                       [8-9]
Humidity (1 byte)                            [10]
Time (4 bytes)                               [11-14]


*/

unsigned int transCharToInt(char* _buffer, int _start, int _stop)        // Credit: DFRobot
{
  unsigned int _index;
  unsigned int result = 0;
  unsigned int num = _stop - _start + 1;
  unsigned int _temp[num];
  for (_index = _start; _index <= _stop; _index++) {
    _temp[_index - _start] = _buffer[_index] - '0';
    result = 10 * result + _temp[_index - _start];
  }
  return result;
}
int transCharToInt_T(char* _buffer)
{
  int result = 0;
  if (_buffer[13] == '-') {
    result = 0 - (((_buffer[14] - '0') * 10) + (_buffer[15] - '0'));
  } else {
    result = ((_buffer[13] - '0') * 100) + ((_buffer[14] - '0') * 10) + (_buffer[15] - '0');
  }
  return result;
}

void blankArray() {
  for(int i = 0; i < 15; ++i) {
    dataRecorded[i] = (char)0;
  }
}
void recordDataOntoArray() {
    unsigned int rainfallOneHour = transCharToInt(buffer, 17, 19);
    unsigned int rainfallOneDay = transCharToInt(buffer, 21, 23);
    unsigned int airPressure = transCharToInt(buffer, 28, 32);

    //dataRecorded[0] = (char) ((rotation & 0b1111111100000000) >> 8);
    //dataRecorded[1] = (char) (rotation);
    dataRecorded[0]  = (char) (transCharToInt(buffer, 1, 3)/45);                               // Wind Direction
    dataRecorded[1]  = (char) transCharToInt(buffer, 5, 7)   ;                               // Avg Wind Speed
    dataRecorded[2]  = (char) transCharToInt(buffer, 9, 11)  ;                               // Top Wind Speed
    dataRecorded[3]  = (char) transCharToInt_T(buffer     )  ;                               // Temperature
    dataRecorded[4]  = (char) ((rainfallOneHour & 0b1111111100000000) >> 8);                 // Rain in One Hour
    dataRecorded[5]  = (char) (rainfallOneHour  & 0b0000000011111111);                       // Rain in One Hour
    dataRecorded[6]  = (char) ((rainfallOneDay & 0b1111111100000000) >> 8);                  // Rain in One Day
    printf("rainfall1d: &d\n", rainfallOneDay);
    dataRecorded[7]  = (char) (rainfallOneDay  & 0b0000000011111111);                        // Rain in One Day
    dataRecorded[8]  = (char) ((airPressure & 0b1111111100000000) >> 8);                     // Air Pressure
    dataRecorded[9]  = (char) (airPressure  & 0b0000000011111111);                           // Air Pressure
    dataRecorded[10] = (char) transCharToInt(buffer, 25, 26);                                // Humidity
    dataRecorded[11] = (char) ((wstime & 0b11111111000000000000000000000000) >> 24);     // Time (Recordings since Start)
    dataRecorded[12] = (char) ((wstime & 0b00000000111111110000000000000000) >> 16);     // Time (Recordings since Start)
    dataRecorded[13] = (char) ((wstime & 0b00000000000000001111111100000000) >> 8);     // Time (Recordings since Start)
    dataRecorded[14] = (char) ( wstime & 0b00000000000000000000000011111111);           // Time (Recordings since Start)

}

char getRotationFromCompression()         {
    //return (((short) dataRecorded[0] >> 8) + ((short) dataRecorded[1]));
    return dataRecorded[0];
}

char getAvgSpeedFromCompression()         {
    return dataRecorded[1];
}

char getFastestSpeedFromCompression()     {
    return dataRecorded[2];
}

char getTemperatureFromCompression()      {
    return dataRecorded[3];
} 

unsigned short getRainfallOneHourFromCompression() {
    return ((((unsigned short) dataRecorded[4]) << 8) + ((unsigned short) dataRecorded[5]));
}
unsigned short getRainfallOneDayFromCompression() {
    return ((((unsigned short) dataRecorded[6]) << 8) + ((unsigned short) dataRecorded[7]));
}
unsigned short getAirPressureFromCompression()    {
    return ((((unsigned short) dataRecorded[8]) << 8) + ((unsigned short) dataRecorded[9]));
}
char getHumidityFromCompression()        {
    return dataRecorded[10];
} 
unsigned int getTimeFromCompresion() {
  return (((unsigned int) dataRecorded[11]) << 24) +
  (((unsigned int) dataRecorded[12]) << 16) +
  (((unsigned int) dataRecorded[13]) << 8) + 
  ((unsigned int) dataRecorded[14]);
}



void printWeatherInfo() {
    printf("\n--- Weather Station Board Data ---\n");
    printf("Wind Direction: %d\n", getRotationFromCompression());
    printf("Wind Speed: %dmph\n", getAvgSpeedFromCompression());
    printf("Top Wind Speed (in past five mins): %dmph\n", getFastestSpeedFromCompression());
    printf("Temperature: %.2f degrees C\n", (((float) getTemperatureFromCompression())-32)*5/9); // The result is in Farenheit, so it must be converted to Celcius
    printf("Humidity: %d%%\n", getHumidityFromCompression());
    printf("Rainfall 1hour: %5.1fmm\n", getRainfallOneHourFromCompression() * 25.40);
    printf("Rainfall 24hours: %5.1fmm\n", getRainfallOneDayFromCompression() * 25.40);
    printf("Air Pressure: %dhPa\n",  getAirPressureFromCompression()/10);
    printf("\nTime_: %drecords\n", getTimeFromCompresion());
    
}