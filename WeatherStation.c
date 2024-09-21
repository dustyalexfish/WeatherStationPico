#include <math.h>

#include "pico/multicore.h"
#include "hardware/rtc.h"
#include "hardware/uart.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"
#include "Recorder.h"
#include "lcd.h"
#include "RotaryEncoder.h"


#define LED_PIN 25
#define UART_ID uart0
#define BAUD_RATE 9600
#define DATA_BITS 8
#define PARITY UART_PARITY_NONE

#define STOP_BITS 1
#define UART_TX_PIN 0
#define UART_RX_PIN 1
#define I2C_SDA 6
#define I2C_SDL 7
#define I2C_PORT i2c1

#define MENUS 9

int sleep_counter = 0;

int time_since_interrupt();

const int ADDR = 0x27;


const int NUMBER_POT_STATES = 9;

static const char default_format[] = "%a %b %d %Y";

int time_until_change = 5;
int intLen(int val) {
  if(val <= 0) {
    return 1;
  }
  if(val <= 10) {
    return 1;
  }
  return (int)((ceil(log10(val)))*sizeof(char));
}

int shortLen(short val) { 
  if(val <= 0) {
    return 0;
  }
  return (int)((ceil(log10(val)))*sizeof(char));
}
bool display_flash_storage() {
  lcd_home();
  char str[intLen(posInFlash)];
  sprintf(str, "%d", posInFlash);
  lcd_write_chars("Flash: ", 7);
  lcd_write_chars(str, intLen(posInFlash));
  lcd_write_chars("%", 1);
  return true;
}

bool display_temp() {
  lcd_home();
  int temperature = (((int) getTemperatureFromCompression())-32)*5/9;
  char str[intLen(temperature)];
  sprintf(str, "%d", temperature);
  lcd_write_chars("Temperature: ", 13);
  lcd_write_chars(str, intLen(temperature));
  lcd_write_char('C');
  return true;
}
bool display_humid() {
  lcd_home();
  int humidity = getHumidityFromCompression();
  char str[intLen(humidity)*sizeof(char)];
  sprintf(str, "%d", humidity);
  lcd_write_chars("Humidity: ", 10);
  lcd_write_chars(str, intLen(humidity)*sizeof(char));
  lcd_write_char('%');
  return true;
}

bool display_windSpeedAvg() {
  lcd_home();
  int avgSpeed = getAvgSpeedFromCompression();
  char str[intLen(avgSpeed)];
  sprintf(str, "%d", avgSpeed);
  lcd_write_chars("Avg Spd: ", 9);
  lcd_write_chars(str, intLen(avgSpeed));
  lcd_write_chars("mph", 3);
  return true;
}
bool display_windSpeedFastest() {
  lcd_home();
  int windSpeed = getFastestSpeedFromCompression();
  char str[intLen(windSpeed)];
  sprintf(str, "%d", windSpeed);
  lcd_write_chars("Fastest Speed: ", 15);
  lcd_home();
  shiftCursor(40);
  lcd_write_chars(str, intLen(windSpeed)*sizeof(char));
  lcd_write_chars("mph", 3);
  return false;
}
bool display_windRotation() {
  lcd_home();
  int windRotation = getRotationFromCompression();
  char str[intLen(windRotation)*sizeof(char)];
  sprintf(str, "%d", windRotation);
  lcd_write_chars("Wind Rot: ", 10);
  lcd_write_chars(str, intLen(windRotation)*sizeof(char));

  return true;
}
bool display_airPressure() {
  lcd_home();
  int airPressure = getAirPressureFromCompression()/10;
  char str[intLen(airPressure)*sizeof(char)];
  sprintf(str, "%d", airPressure);
  lcd_write_chars("Pressure: ", 10);
  lcd_write_chars(str, intLen(airPressure)*sizeof(char));

  return true;
}
bool display_rainfall_hour() {
  lcd_home();
  int rainfall = getRainfallOneHourFromCompression()*25;
  char str[intLen(rainfall)];
  sprintf(str, "%d", rainfall);
  lcd_write_chars("1h Rain: ", 9);
  lcd_write_chars(str, intLen(rainfall));
  lcd_write_chars("mm", 2);
  return true;
}
bool display_rainfall_day() {
  lcd_home();
  int rainfall = getRainfallOneDayFromCompression()*25;
  char str[intLen(rainfall)];
  sprintf(str, "%d", rainfall);
  lcd_write_chars("24h Rain: ", 10);
  lcd_write_chars(str, intLen(rainfall));
  lcd_write_chars("mm", 2);
  return true;
}




int pot_state = 0;
int tick_counter = 0;
int lastPotState = 0;

void updateLCD(int pot_state) {
  if(lastPotState != pot_state) {
    lastPotState = pot_state;
    lcd_clear_screen();
  }
  lcd_home();
  //lcd_write_chars("Yay! ", 5);
  //busy_wait_ms(2);
  bool displayTime = true;
  
  switch(pot_state) {
    case 0:
      displayTime = display_flash_storage();
      break;
    case 2:
      displayTime = display_humid();
      break;
    case 3:
      displayTime = display_windSpeedAvg();
      break;
    case 4:
      displayTime = display_windSpeedFastest();
      break;
    case 5:
      displayTime = display_windRotation();
      break;
    case 6:
      displayTime = display_airPressure();
      break;
    case 7:
      displayTime = display_rainfall_hour();
      break;
    case 8:
      displayTime = display_rainfall_day();
      break;
    default:
      displayTime = display_temp();
      break;
  }     
  
  if(displayTime) {
    lcd_home();
    char timeChars[intLen(wstime)];
    shiftCursor(40);
    sprintf(timeChars, "%d", wstime);
    lcd_write_chars("Time: ", 6);
    lcd_write_chars(timeChars, intLen(wstime));
  }
  //restore_interrupts(ints);
}

const int TIME_TURN_OFF = 10; // Turn off at 8:00 PM
const int TIME_TURN_ON = 7; // turn back on at 7:00 AM

int scroll(int rotPos, int scrollBy, int numberOfFlashLogs)
{
  int rotPosition = rotPos;
  while(getRotaryEncoderSWPushState()) {
    tight_loop_contents();
  }
  while (!getRotaryEncoderSWPushState())
  {
    getFlashLogContents(rotPosition);
    time_t rawtime = getTimeFromCompresion();
    struct tm ts;
    char buf[16];
    char buf2[16];
    for (int i = 0; i < 16; ++i)
    {
      buf[i] = ' ';
      buf2[i] = ' ';
    }
    // Format time, "ddd yyyy-mm-dd hh:mm:ss zzz"
    ts = *localtime(&rawtime);
    strftime(buf, sizeof(buf), "%a %d/%m/%Y", &ts);
    strftime(buf2, sizeof(buf2), "%H:%M:%S %Z", &ts);
    lcd_clear_screen();
    lcd_home();
    lcd_write_chars(buf, 16);
    shiftCursor(24);
    lcd_write_chars(buf2, 16);
    printf("%s\n", buf);

    while (true)
    {
      int update = update_rotaryEncoder();
      if (update == ENCODER_CW)
      {
        rotPosition += scrollBy;
        rotPosition = rotPosition % numberOfFlashLogs;
        break;
      }
      if (update == ENCODER_CCW)
      {
        rotPosition -= scrollBy;
        if (rotPosition <= -1)
        {
          rotPosition = numberOfFlashLogs - 1;
        }
        rotPosition = rotPosition % numberOfFlashLogs;
        break;
      }
      if (getRotaryEncoderSWPushState())
      {
        break;
      }
    }
  }
  return rotPosition;
}

void mainLoop() {
  int index = 0;
  bool takingData = false;

  lcd_backlight_on();
  lcd_clear_screen();
  unsigned int record = 0;
  
  while(true) {

      char ch = uart_getc(UART_ID);
      if(ch == 'c') {
          gpio_set_dormant_irq_enabled(UART_RX_PIN, GPIO_IRQ_EDGE_FALL, false); // Disable dormant mode
          sleep_counter++;
          takingData = true;
          index = 0;

      }
    
      if(takingData) { 
        
        
        buffer[index] = ch;
        index++;
        if(index == 35) {
          
          record++;
          wstime = getUnixTime();

          updateLCD(pot_state);
          time_until_change -= 1;
          if(time_until_change == 0) {
            pot_state += 1;
            pot_state = pot_state % NUMBER_POT_STATES;
            time_until_change = 5;
          }

          index = 0;
          takingData = false;
          
          recordDataOntoArray();
          printWeatherInfo();
          wstime = 0;
          if(record % 60 == 0) {
            compressData(record == 0);       
          }

          gpio_set_dormant_irq_enabled(UART_RX_PIN, GPIO_IRQ_EDGE_FALL, true); // Put the PICO into dormant mode 
        }
        if(sleep_counter > 30 && isBacklightOn()) {
          lcd_backlight_off();
          lcd_turn_screen_off();
          sleep_ms(10);
        } else if (!isBacklightOn() && sleep_counter < 30) {
          lcd_turn_screen_on();
          lcd_backlight_on();
          sleep_ms(10);
        }
      }

      uint8_t potentiometerTime = 0;
      if(getRotaryEncoderSWPushState()) {
        if(!isBacklightOn()) {
          sleep_counter = 0;
          while(getRotaryEncoderSWPushState()) {
            tight_loop_contents();
          }
        }
        lcd_clear_screen();
        sleep_ms(500);
        lcd_home();
      }
      while(getRotaryEncoderSWPushState()) {
          potentiometerTime++;
          sleep_ms(1000);

          lcd_write_char('.');
          if(potentiometerTime == 10) {
            lcd_clear_screen();
            lcd_home();
            lcd_write_chars("Erase", 5);

            lcd_backlight_on();
            printf("Erase\n");
            sleep_ms(3000);
            for(int i = 0; i < PICO_FLASH_SIZE_BYTES-RESERVED; i += 4096) {
              printf(".");
              
              lcd_home();
              shiftCursor(40);
              char str[intLen(i/4096)];
              sprintf(str, "%d", i/4096);
              lcd_write_chars(str, intLen(i/4096));
              lcd_write_chars(" pages", 6);
              uint32_t interrupts = save_and_disable_interrupts();
              flash_range_erase(i+RESERVED, 4096);
              restore_interrupts(interrupts);
            }
            lcd_clear_screen();
            lcd_home();
            lcd_write_chars("Please restart",14);
            printf("\nDone! Please restart now");
            while(true) {
              tight_loop_contents();
            };
          }
      }
      if(potentiometerTime > 5) {
        printf("Dumping output");
        int position = 0;
        lcd_clear_screen();
        lcd_home();
        lcd_write_chars("Dumping output", 14);

          lcd_home();
          int pos = 0;
          while(pos < posInFlash) {
            int flashLog = readFlashLog(pos);
            if((flashLog>65 && flashLog < 90) || (flashLog > 97 && flashLog < 122)) {
              printf("%c ", flashLog);
            } else {
              printf("%d ", flashLog);
            }
            if(pos % 16 == 0) {
              printf("\n");
            }
            pos += 1;
          }
          sleep_ms(500);
          printf("Done\n");
          while(!getRotaryEncoderSWPushState()) {
            tight_loop_contents(); // NOP instruction
          }
      } else if (potentiometerTime > 1) {
        int numberOfFlashLogs = getNumberOfFlashLogs();
        int rotPosition = 0;
        bool exit = false;
        while(!exit) {
          rotPosition = scroll(rotPosition, 43200, numberOfFlashLogs);
          rotPosition = scroll(rotPosition, 1440, numberOfFlashLogs);
          rotPosition = scroll(rotPosition, 60, numberOfFlashLogs);
          rotPosition = scroll(rotPosition, 1, numberOfFlashLogs);
          if(getRotaryEncoderSWPushState()) {
            lcd_clear_screen();
            lcd_home();
            updateLCD(pot_state);
            while(getRotaryEncoderSWPushState()) {};
            sleep_ms(100);
            while(!getRotaryEncoderSWPushState()) {};
            exit = true;
            break;
          }
        }
        
      } 

      
  }

}

int main() {

  



    //multicore_fifo_clear_irq();
    stdio_init_all();
    
    i2c_init(I2C_PORT, 50*1000);
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    uart_set_hw_flow(UART_ID, false, false);
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);
    uart_set_fifo_enabled(UART_ID, true);
    init_lcd();
    lcd_write_chars("!!!!", 4);
    recorder_init();
    findPosInFlash();
    if(!isFormatted()) {
      lcd_clear_screen();
      lcd_home();
      lcd_write_chars("Erasing data", 12);
      if(isFormatted(0)) {
        eraseBlock(0);
      }    
      writePrimarySignature();
      sleep_ms(1000);
      lcd_clear_screen();
      lcd_home();
    }
    gpio_put(LED_PIN, 1);

    init_rotaryEncoder();
    mainLoop();
    

}
