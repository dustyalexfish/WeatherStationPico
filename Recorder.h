#include "hardware/flash.h"
#include "WeatherStation.h"
#include "RTC.h"
#include <hardware/sync.h>

#define DATA_SIZE 256

char storageOnRAM[DATA_SIZE];

int RAMposition = 0;
int currentPosition = 0;
int posInFlash = 0;


const int RESERVED = 409600;
char lastTemp = 0;
char lastHumidity = 0;
char lastWindDirection = 0;
char lastWindSpeedAvg = 0;
char lastTopWindSpeed = 0;
unsigned short lastRainfall24Hours = 0;
unsigned short lastRainfall1Hour = 0;
unsigned short lastAirPressure = 0;

const char SIGNATURE_ONE = 136;
const char SIGNATURE_TWO = 92;

const char TEMP_CHAR = 't';
const char TIME_CHAR = 'T';
const char HUMIDITY_CHAR = 'h';
const char WIND_DIR_CHAR = 'd';
const char WIND_SPEED_CHAR = 's';
const char WIND_TOP_SPEED_CHAR = 'S';
const char RAINFALL_ONE_HOUR_CHAR = 'r';
const char RAINFALL_24_HOUR_CHAR = 'R';
const char AIR_PRESSURE_CHAR = 'p';
const char FINISHED_CHAR = 'F';
const char BLOCK_FINISHED = 'b';
const bool DISABLE_MEMORY_COMPRESSION = false; // This is for testing, to fill up flash with data.


char MEMBUFFER[256] = {SIGNATURE_ONE, SIGNATURE_TWO};
char SIGTWO[256] = {12, 63, 212, 74, 162, 53, 11};
char SIGTHREE[256] = {51, 52, 54, 214, 242, 31, 55};

char compressedData[25];
void eraseBlock(int block) {
    flash_range_erase(RESERVED+block, FLASH_SECTOR_SIZE);
}
void writePrimarySignature() {
    flash_range_program(RESERVED, MEMBUFFER, 256);
    flash_range_program(RESERVED+256, SIGTWO, 256);
    flash_range_program(RESERVED+512, SIGTHREE, 256);
}
char readFlashLog(int offset) {
    char *p = (char *)XIP_BASE+RESERVED+offset;
    return *p;
}
bool hasFlashBeenErased(int offset) {
    return (readFlashLog(offset) == (char)255);
}

void findPosInFlash() {
    posInFlash = 0;
    while(readFlashLog(posInFlash+4096) != (char)255) {
        printf("Trying block %d\n", posInFlash/256);
        posInFlash += 256;
    }
}
void blank_recorder() {
    RAMposition = 0;
    currentPosition = 0;
    for(int i = 0; i < DATA_SIZE; ++i) {
        storageOnRAM[i] = 0;
    }
}
void recorder_init() {
    blank_recorder();
}
bool isFormatted() {
    return (readFlashLog(0) == SIGNATURE_ONE && readFlashLog(1) == SIGNATURE_TWO);
}

int compressData() {
    unsigned int TIME = getUnixTime();
    printf("WSTIME: %d\n", TIME);
    int index = 0;
    printf("Req compressData()\n");
    char temp = getTemperatureFromCompression();
    if(temp != lastTemp || DISABLE_MEMORY_COMPRESSION) {
        lastTemp = temp;
        compressedData[index] = TEMP_CHAR;
        compressedData[index+1] = temp;
        index += 2;
    }
    char humidity = getHumidityFromCompression();
    if(humidity != lastHumidity || DISABLE_MEMORY_COMPRESSION) {
        lastHumidity = humidity;
        compressedData[index] = HUMIDITY_CHAR;
        compressedData[index+1] = humidity;
        index += 2;
    }
    char windDir = getRotationFromCompression();
    if(windDir != lastWindDirection || DISABLE_MEMORY_COMPRESSION) {
        lastWindDirection = windDir;
        compressedData[index] = WIND_DIR_CHAR;
        compressedData[index+1] = windDir;
        index += 2;
    }
    char windSpeedAvg = getAvgSpeedFromCompression();
    if(windSpeedAvg != lastWindSpeedAvg || DISABLE_MEMORY_COMPRESSION) {
        lastWindSpeedAvg = windSpeedAvg;
        compressedData[index] = WIND_SPEED_CHAR;
        compressedData[index+1] = windSpeedAvg;
        index += 2;
    }
    char topWindSpeed = getFastestSpeedFromCompression();
    if(windDir != lastWindDirection || DISABLE_MEMORY_COMPRESSION) {
        lastWindDirection = windDir;
        compressedData[index] = WIND_TOP_SPEED_CHAR;
        compressedData[index+1] = windDir;
        index += 2;
    }
    unsigned short rainOneHour = getRainfallOneHourFromCompression();
    if(lastRainfall1Hour != rainOneHour || DISABLE_MEMORY_COMPRESSION) {
        lastRainfall1Hour = rainOneHour;
        compressedData[index] = RAINFALL_ONE_HOUR_CHAR;
        compressedData[index+1]  = (char) ((rainOneHour & 0b1111111100000000) >> 8);
        compressedData[index+2]  = (char) (rainOneHour  & 0b0000000011111111);
        index += 3;
    }
    unsigned short rain24Hours = getRainfallOneHourFromCompression();
    if(lastRainfall24Hours != rain24Hours || DISABLE_MEMORY_COMPRESSION) {
        lastRainfall24Hours = rain24Hours;
        compressedData[index] = RAINFALL_24_HOUR_CHAR;
        compressedData[index+1]  = (char) ((rain24Hours & 0b1111111100000000) >> 8);
        compressedData[index+2]  = (char) (rain24Hours  & 0b0000000011111111);
        index += 3;
    }
    unsigned short airPressure = getAirPressureFromCompression();
    if((lastAirPressure != airPressure && airPressure < 400) || DISABLE_MEMORY_COMPRESSION) { // My Weather Station Module had problems with reporting the correct air pressure, so any low readings are discarded
        lastAirPressure = airPressure;
        compressedData[index] = AIR_PRESSURE_CHAR;
        compressedData[index+1]  = (char) ((airPressure & 0b1111111100000000) >> 8);
        compressedData[index+2]  = (char) (airPressure  & 0b0000000011111111);
        

        index += 3;
    }
    if(index > 0) {
        compressedData[index] = TIME_CHAR;
        compressedData[index+1] = (char) ((TIME & 0b11111111000000000000000000000000) >> 24);     // Time (Recordings since Start)
        compressedData[index+2] = (char) ((TIME & 0b00000000111111110000000000000000) >> 16);     // Time (Recordings since Start)
        compressedData[index+3] = (char) ((TIME & 0b00000000000000001111111100000000) >> 8);     // Time (Recordings since Start)
        compressedData[index+4] = (char) (TIME &  0b00000000000000000000000011111111);
        index += 5; 
    }
    if(true) {
        compressedData[index] = FINISHED_CHAR;
    }
    //Dump to flash cache
    for(int i = 0; i < 24; ++i) {
        storageOnRAM[RAMposition+i] = compressedData[i];
        
        if(compressedData[i] == FINISHED_CHAR) {
            break;
        }
    }
    printf("recording\n");
    storageOnRAM[index+1] = FINISHED_CHAR;
    if(RAMposition > DATA_SIZE-25) {
        printf("Flashing new DATA");
        uint32_t ints;
        for(int i = RAMposition; i < DATA_SIZE; ++i) {
            storageOnRAM[i] = BLOCK_FINISHED;
        }
        for(int i = 0; i < 256; ++i) {
            printf("%d ", readFlashLog(posInFlash+i+4096));
        }
        
        if(!hasFlashBeenErased(posInFlash+4096)) {
            printf("Erase required");
            ints = save_and_disable_interrupts();
            flash_range_erase(RESERVED+posInFlash+4096, FLASH_SECTOR_SIZE);
        } else {
            ints = save_and_disable_interrupts();
        }
        printf("\nSTART FLASH\n");
        printf("%d", RESERVED+posInFlash+4096);
        
        flash_range_program(RESERVED+posInFlash+4096, (uint8_t *)storageOnRAM, FLASH_PAGE_SIZE);
        restore_interrupts (ints);
        printf("END FLASH");
        blank_recorder();
        printf("Emptied buffer");
    
        for(int i = 0; i < 256; ++i) {
            printf("%d ", readFlashLog(posInFlash+i+4096));
        }
        posInFlash += 256;
        
        
    }
    
    RAMposition += index;

    printf("\nNew index: %d\n", RAMposition);
    return index;


}
int getNumberOfFlashLogs() {
  int position = 0;
  int numberOfFlashLogs = 0;
  while(position <= posInFlash) {

    if(readFlashLog(position+4096) == TEMP_CHAR) {
      position++;
    } else if(readFlashLog(position+4096) == HUMIDITY_CHAR) {
      position++;
    } else if(readFlashLog(position+4096) == WIND_DIR_CHAR) {
      position++;
    } else if(readFlashLog(position+4096) == WIND_SPEED_CHAR) {
      position++;
    } else if(readFlashLog(position+4096) == WIND_TOP_SPEED_CHAR) {
      position++;
    } else if(readFlashLog(position+4096) == RAINFALL_ONE_HOUR_CHAR) {
      position += 2;
    } else if(readFlashLog(position+4096) == RAINFALL_24_HOUR_CHAR) {
      position += 2;
    } else if(readFlashLog(position+4096) == AIR_PRESSURE_CHAR) {
      position += 2;
    } else if (readFlashLog(position+4096) == TIME_CHAR) {
      position += 4;
      numberOfFlashLogs++;
    }
    position++;
  }
  return numberOfFlashLogs;
}

//bool containsKey(int logPos, char key) {
//  int position = 0;
//  int numberOfFlashLogs = 0;
//  while(numberOfFlashLogs < logPos || readFlashLog(position+4096) == BLOCK_FINISHED) {
//
//    if(readFlashLog(position+4096) == TEMP_CHAR) {
//      position++;
//    } else if(readFlashLog(position+4096) == HUMIDITY_CHAR) {
//      position++;
//    } else if(readFlashLog(position+4096) == WIND_DIR_CHAR) {
//      position++;
//    } else if(readFlashLog(position+4096) == WIND_SPEED_CHAR) {
//      position++;
//    } else if(readFlashLog(position+4096) == WIND_TOP_SPEED_CHAR) {
//      position++;
//    } else if(readFlashLog(position+4096) == RAINFALL_ONE_HOUR_CHAR) {
//      position += 2;
//    } else if(readFlashLog(position+4096) == RAINFALL_24_HOUR_CHAR) {
//      position += 2;
//    } else if(readFlashLog(position+4096) == AIR_PRESSURE_CHAR) {
//      position += 2;
//    } else if(readFlashLog(position+4096) == TIME_CHAR) {
//      position += 4;
//      numberOfFlashLogs++;
//    }
//    position++;
//  }
//  while(true) {
//
//    if(readFlashLog(position+4096) == TEMP_CHAR) {
//      if(key == TEMP_CHAR) {
//        return true;
//      }
//      position++;
//      dataRecorded[3] = readFlashLog(position+4096);
//      printf("Temperature: %d\n", readFlashLog(position+4096));
//    } else if(readFlashLog(position+4096) == HUMIDITY_CHAR) {
//      if(key == HUMIDITY_CHAR) {
//        return true;
//      }
//      position++;
//      dataRecorded[10] = readFlashLog(position+4096);
//      printf("Humidity: %d\n", readFlashLog(position+4096));
//    } else if(readFlashLog(position+4096) == WIND_DIR_CHAR) {
//      if(key == WIND_DIR_CHAR) {
//        return true;
//      }
//      position++;
//      dataRecorded[0] = readFlashLog(position+4096);
//      printf("Wind Dir: %d\n", readFlashLog(position+4096));
//    } else if(readFlashLog(position+4096) == WIND_SPEED_CHAR) {
//      position++;
//      dataRecorded[1] = readFlashLog(position+4096);
//      printf("Wind Speed: %d\n", readFlashLog(position+4096));
//    } else if(readFlashLog(position+4096) == WIND_TOP_SPEED_CHAR) {
//      position++;
//      dataRecorded[2] = readFlashLog(position+4096);
//      printf("Wind Top Speed: %d\n", readFlashLog(position+4096));
//    } else if(readFlashLog(position+4096) == RAINFALL_ONE_HOUR_CHAR) {
//      position += 2;
//      dataRecorded[4] = readFlashLog(position+4096-1);
//      dataRecorded[5] = readFlashLog(position+4096);
//    } else if(readFlashLog(position+4096) == RAINFALL_24_HOUR_CHAR) {
//      position += 2;
//      dataRecorded[6] = readFlashLog(position+4096-1);
//      dataRecorded[7] = readFlashLog(position+4096);
//    } else if(readFlashLog(position+4096) == AIR_PRESSURE_CHAR) {
//      position += 2;
//      dataRecorded[8] = readFlashLog(position+4096-1);
//      dataRecorded[9] = readFlashLog(position+4096);
//    } else if(readFlashLog(position+4096) == TIME_CHAR) {
//      position += 4;
//      dataRecorded[11] = readFlashLog(position+4096-3);
//      dataRecorded[12] = readFlashLog(position+4096-2);
//      dataRecorded[13] = readFlashLog(position+4096-1);
//      dataRecorded[14] = readFlashLog(position+4096);
//      break;
//    }
//    position++;
//  }
//  return false;
//}

void getFlashLogContents(int logPos) {
  int position = 0;
  int numberOfFlashLogs = 0;
  blankArray();

  while(true) {

    if(readFlashLog(position+4096) == TEMP_CHAR) {
      position++;
      dataRecorded[3] = readFlashLog(position+4096);
      printf("Temperature: %d\n", readFlashLog(position+4096));
    } else if(readFlashLog(position+4096) == HUMIDITY_CHAR) {
      position++;
      dataRecorded[10] = readFlashLog(position+4096);
      printf("Humidity: %d\n", readFlashLog(position+4096));
    } else if(readFlashLog(position+4096) == WIND_DIR_CHAR) {
      position++;
      dataRecorded[0] = readFlashLog(position+4096);
      printf("Wind Dir: %d\n", readFlashLog(position+4096));
    } else if(readFlashLog(position+4096) == WIND_SPEED_CHAR) {
      position++;
      dataRecorded[1] = readFlashLog(position+4096);
      printf("Wind Speed: %d\n", readFlashLog(position+4096));
    } else if(readFlashLog(position+4096) == WIND_TOP_SPEED_CHAR) {
      position++;
      dataRecorded[2] = readFlashLog(position+4096);
      printf("Wind Top Speed: %d\n", readFlashLog(position+4096));
    } else if(readFlashLog(position+4096) == RAINFALL_ONE_HOUR_CHAR) {
      position += 2;
      dataRecorded[4] = readFlashLog(position+4096-1);
      dataRecorded[5] = readFlashLog(position+4096);
    } else if(readFlashLog(position+4096) == RAINFALL_24_HOUR_CHAR) {
      position += 2;
      dataRecorded[6] = readFlashLog(position+4096-1);
      dataRecorded[7] = readFlashLog(position+4096);
    } else if(readFlashLog(position+4096) == AIR_PRESSURE_CHAR) {
      position += 2;
      dataRecorded[8] = readFlashLog(position+4096-1);
      dataRecorded[9] = readFlashLog(position+4096);
    } else if(readFlashLog(position+4096) == TIME_CHAR) {
      position += 4;
      dataRecorded[11] = readFlashLog(position+4096-3);
      dataRecorded[12] = readFlashLog(position+4096-2);
      dataRecorded[13] = readFlashLog(position+4096-1);
      dataRecorded[14] = readFlashLog(position+4096);
      numberOfFlashLogs++;
      if(numberOfFlashLogs >= logPos) {
      break;
      }
    }
    position++;
  }
}


void printRecordedData() {
    char *p = (char *)XIP_BASE;
    char first_byte = *p;


    printf("\nRecorded Data: ");
    for(int i = 0; i < 24; ++i) {

        printf("%d ", compressedData[i]);
        if(compressedData[i] == FINISHED_CHAR) {
            break;
        }
    }
    printf("\n");
    printf("The data to be written to flash: ");
    for(int i = 0; i < RAMposition; ++i) {

        printf("%d ", storageOnRAM[i]);
    }
    printf("\n");
}