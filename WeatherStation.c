#include "pico/sleep.h"
#include <math.h>

#include "hardware/rtc.h"
#include "pico/multicore.h"

#include "hardware/uart.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"
#include "Recorder.h"
#include "lcd.h"
#include "RotaryEncoder.h"
#include "hardware/rtc.h"
#include "hardware/clocks.h"
#include "hardware/rosc.h"
#include "hardware/structs/scb.h"

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
#define SQW_PIN 16
#define I2C_PORT i2c1

#define MENUS 9

#define ENABLE_DORMANT true                  // Turn this ON if you are battery or solar powering your WeatherStation. However, Dormant mode is untested and is part of Pico Extras, so is unstable.
#define ENABLE_PROTOTYPE_LED false           // The onboard LED draws around 2 mA, and this is off to save that power
#define TURN_LCD_OFF_AFTER_TIME_EXPIRE false // I am not sure if turning this on reduces power, so it is by default off

int timeUntilLcdOffStart = 0;

int time_since_interrupt();

const int ADDR = 0x27;

const int NUMBER_POT_STATES = 9;

volatile uint scb_orig;
volatile uint clock0_orig;
volatile uint clock1_orig;

static const char default_format[] = "%a %b %d %Y";

int time_until_change = 5;
int intLen(int val)
{
  if (val <= 0)
  {
    return 1;
  }
  if (val <= 10)
  {
    return 1;
  }
  return (int)((ceil(log10(val))) * sizeof(char));
}

int shortLen(short val)
{
  if (val <= 0)
  {
    return 0;
  }
  return (int)((ceil(log10(val))) * sizeof(char));
}
bool display_flash_storage()
{
  lcd_home();
  char str[intLen(posInFlash / 20971)];
  sprintf(str, "%d", posInFlash / 20971);
  lcd_write_chars("Flash %: ", 9);
  lcd_write_chars(str, intLen(posInFlash / 16875));
  lcd_write_chars("%", 1);
  return true;
}

bool display_temp()
{
  lcd_home();
  int temperature = (((int)getTemperatureFromCompression()) - 32) * 5 / 9;
  char str[intLen(temperature)];
  sprintf(str, "%d", temperature);
  lcd_write_chars("Temperature: ", 13);
  lcd_write_chars(str, intLen(temperature));
  lcd_write_char('C');
  return true;
}
bool display_humid()
{
  lcd_home();
  int humidity = getHumidityFromCompression();
  char str[intLen(humidity) * sizeof(char)];
  sprintf(str, "%d", humidity);
  lcd_write_chars("Humidity: ", 10);
  lcd_write_chars(str, intLen(humidity) * sizeof(char));
  lcd_write_char('%');
  return true;
}

bool display_windSpeedAvg()
{
  lcd_home();
  int avgSpeed = getAvgSpeedFromCompression();
  char str[intLen(avgSpeed)];
  sprintf(str, "%d", avgSpeed);
  lcd_write_chars("Avg Spd: ", 9);
  lcd_write_chars(str, intLen(avgSpeed));
  lcd_write_chars("mph", 3);
  return true;
}
bool display_windSpeedFastest()
{
  lcd_home();
  int windSpeed = getFastestSpeedFromCompression();
  char str[intLen(windSpeed)];
  sprintf(str, "%d", windSpeed);
  lcd_write_chars("Fastest Speed: ", 15);
  lcd_home();
  shiftCursor(40);
  lcd_write_chars(str, intLen(windSpeed) * sizeof(char));
  lcd_write_chars("mph", 3);
  return false;
}
bool display_windRotation()
{
  lcd_home();
  int windRotation = getRotationFromCompression();
  char str[intLen(windRotation) * sizeof(char)];
  sprintf(str, "%d", windRotation);
  lcd_write_chars("Wind Rot: ", 10);
  lcd_write_chars(str, intLen(windRotation) * sizeof(char));

  return true;
}
bool display_airPressure()
{
  lcd_home();
  int airPressure = getAirPressureFromCompression() / 10;
  char str[intLen(airPressure) * sizeof(char)];
  sprintf(str, "%d", airPressure);
  lcd_write_chars("Pressure: ", 10);
  lcd_write_chars(str, intLen(airPressure) * sizeof(char));

  return true;
}
bool display_rainfall_hour()
{
  lcd_home();
  int rainfall = getRainfallOneHourFromCompression() * 25;
  char str[intLen(rainfall)];
  sprintf(str, "%d", rainfall);
  lcd_write_chars("1h Rain: ", 9);
  lcd_write_chars(str, intLen(rainfall));
  lcd_write_chars("mm", 2);
  return true;
}
bool display_rainfall_day()
{
  lcd_home();
  int rainfall = getRainfallOneDayFromCompression() * 25;
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

void initPins()
{
  stdio_uart_init();
  // stdio_usb_init();

  i2c_init(I2C_PORT, 50 * 1000);
  uart_init(UART_ID, BAUD_RATE);

  // LED

  uart_set_hw_flow(UART_ID, false, false);
  uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);
  uart_set_fifo_enabled(UART_ID, false);
}

void updateLCD(int pot_state)
{
  if (lastPotState != pot_state)
  {
    lastPotState = pot_state;
    lcd_clear_screen();
  }
  lcd_home();
  // lcd_write_chars("Yay! ", 5);
  // busy_wait_ms(2);
  bool displayTime = true;

  switch (pot_state)
  {
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

  if (displayTime)
  {
    lcd_home();
    char timeChars[intLen(wstime)];
    shiftCursor(40);
    sprintf(timeChars, "%d", wstime);
    lcd_write_chars("Time: ", 6);
    lcd_write_chars(timeChars, intLen(wstime));
  }
  // restore_interrupts(ints);
}

const int TIME_TURN_OFF = 10; // Turn off at 8:00 PM
const int TIME_TURN_ON = 7;   // turn back on at 7:00 AM

int scroll(int rotPos, int scrollBy, int numberOfFlashLogs)
{
  int rotPosition = rotPos;
  while (getRotaryEncoderSWPushState())
  {
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

void mainLoop()
{
  int index = 0;
  bool takingData = false;

  lcd_backlight_on();
  lcd_clear_screen();
  unsigned int record = 0;
  bool hasResultBeenTaken = false;
  timeUntilLcdOffStart = getUnixTime();
  while (true)
  {

    char ch = uart_getc(UART_ID);

    if (ch == 'c')
    {

      takingData = true;
      index = 0;
    }

    if (takingData)
    {

      printf("%c", ch);
      buffer[index] = ch;
      index++;
      if (index == 35)
      {
        hasResultBeenTaken = true;
        record++;
        wstime = getUnixTime();
        updateLCD(pot_state);
        time_until_change -= 1;
        if (time_until_change == 0)
        {
          pot_state += 1;
          pot_state = pot_state % NUMBER_POT_STATES;
          time_until_change = 5;
        }

        index = 0;
        takingData = false;

        recordDataOntoArray();
        wstime = 0;
        if (getUnixTime() - timeUntilLcdOffStart > 30) // Only record if off to prevent uneven records
        {
          compressData(record); // This is to take a few recordings in order to set all values to 0 that should be 0
        }
      }
      if (getUnixTime() - timeUntilLcdOffStart > 30 && isBacklightOn())
      {
        lcd_backlight_off();
#if TURN_LCD_OFF_AFTER_TIME_EXPIRE
        lcd_turn_screen_off();
#endif
        sleep_ms(10);
      }
      else if (!isBacklightOn() && getUnixTime() - timeUntilLcdOffStart < 30)
      {
#if TURN_LCD_OFF_AFTER_TIME_EXPIRE
        lcd_turn_screen_on();
#endif
        lcd_backlight_on();
        sleep_ms(10);
      }
    }

    uint8_t potentiometerTime = 0;
    if (getRotaryEncoderSWPushState())
    {
      if (!isBacklightOn())
      {
        timeUntilLcdOffStart = getUnixTime();
        while (getRotaryEncoderSWPushState())
        {
          tight_loop_contents();
        }
      }
      lcd_clear_screen();
      sleep_ms(500);
      lcd_home();
    }
    while (getRotaryEncoderSWPushState())
    {
      potentiometerTime++;
      sleep_ms(1000);

      lcd_write_char('.');
      if (potentiometerTime == 10)
      {
        lcd_clear_screen();
        lcd_home();
        lcd_write_chars("Erase", 5);

        lcd_backlight_on();
        printf("Erase\n");
        sleep_ms(3000);
        for (int i = 0; i < PICO_FLASH_SIZE_BYTES - RESERVED; i += 4096)
        {
          printf(".");

          lcd_home();
          shiftCursor(40);
          char str[intLen(i / 4096)];
          sprintf(str, "%d", i / 4096);
          lcd_write_chars(str, intLen(i / 4096));
          lcd_write_chars(" pages", 6);
          uint32_t interrupts = save_and_disable_interrupts();
          flash_range_erase(i + RESERVED, 4096);
          restore_interrupts(interrupts);
        }
        lcd_clear_screen();
        lcd_home();
        lcd_write_chars("Please restart", 14);
        printf("\nDone! Please restart now");
        while (true)
        {
          tight_loop_contents();
        };
      }
    }
    if (potentiometerTime > 5)
    {
      lcd_clear_screen();
      lcd_home();
      sleep_ms(10);
      lcd_write_chars("Calc Climate", 12);
      sleep_ms(10);
      long temp = 0;
      long humidity = 0;
      long airPressure = 0;
      long windDir = 0;
      long avgWindSpeed = 0;
      long fastWindSpeed = 0;
      long rainfallOneHour = 0;
      long rainfall24Hour = 0;
      int position = 0;
      int numberOfFlashLogs = getNumberOfFlashLogs();
      while (position < numberOfFlashLogs)
      {
        if (readFlashLog(position + 4096) == TEMP_CHAR)
        {
          position++;
          dataRecorded[3] = readFlashLog(position + 4096);
        }
        else if (readFlashLog(position + 4096) == HUMIDITY_CHAR)
        {
          position++;
          dataRecorded[10] = readFlashLog(position + 4096);
        }
        else if (readFlashLog(position + 4096) == WIND_DIR_CHAR)
        {
          position++;
          dataRecorded[0] = readFlashLog(position + 4096);
        }
        else if (readFlashLog(position + 4096) == WIND_SPEED_CHAR)
        {
          position++;
          dataRecorded[1] = readFlashLog(position + 4096);
        }
        else if (readFlashLog(position + 4096) == WIND_TOP_SPEED_CHAR)
        {
          position++;
          dataRecorded[2] = readFlashLog(position + 4096);
        }
        else if (readFlashLog(position + 4096) == RAINFALL_ONE_HOUR_CHAR)
        {
          position += 2;
          dataRecorded[4] = readFlashLog(position + 4096 - 1);
          dataRecorded[5] = readFlashLog(position + 4096);
        }
        else if (readFlashLog(position + 4096) == RAINFALL_24_HOUR_CHAR)
        {
          position += 2;
          dataRecorded[6] = readFlashLog(position + 4096 - 1);
          dataRecorded[7] = readFlashLog(position + 4096);
        }
        else if (readFlashLog(position + 4096) == AIR_PRESSURE_CHAR)
        {
          position += 2;
          dataRecorded[8] = readFlashLog(position + 4096 - 1);
          dataRecorded[9] = readFlashLog(position + 4096);

        } else {
          position++;
        }

        temp += getTemperatureFromCompression();
        humidity += getHumidityFromCompression();
        airPressure += getAirPressureFromCompression();
        windDir += getAirPressureFromCompression();
        avgWindSpeed = getAvgSpeedFromCompression();
        fastWindSpeed = getFastestSpeedFromCompression();
        rainfallOneHour += getRainfallOneHourFromCompression();
        rainfall24Hour += getRainfallOneDayFromCompression();
        


      }

      temp = temp / numberOfFlashLogs;
      dataRecorded[3] = (char)temp;
      humidity = humidity / numberOfFlashLogs;
      dataRecorded[10] = (char)humidity;
      airPressure = airPressure / numberOfFlashLogs;
      dataRecorded[8] = (airPressure & 0b0000000011111111);
      dataRecorded[9] = (airPressure & 0b1111111100000000) >> 8;
      windDir = windDir / numberOfFlashLogs;
      dataRecorded[0] = windDir;
      avgWindSpeed = avgWindSpeed / numberOfFlashLogs;
      dataRecorded[1] = avgWindSpeed;
      fastWindSpeed = fastWindSpeed / numberOfFlashLogs;
      dataRecorded[2] = readFlashLog(position + 4096);
      rainfallOneHour = rainfallOneHour / numberOfFlashLogs;
      dataRecorded[4] = (rainfallOneHour & 0b0000000011111111);
      dataRecorded[5] = (rainfallOneHour & 0b1111111100000000) >> 8;
      rainfall24Hour = rainfall24Hour / numberOfFlashLogs;
      dataRecorded[6] = (rainfall24Hour & 0b0000000011111111);
      dataRecorded[7] = (rainfall24Hour & 0b1111111100000000) >> 8;

      updateLCD(pot_state);
      lcd_backlight_on();
      timeUntilLcdOffStart = getUnixTime();
      while (!getRotaryEncoderSWPushState()) {
        tight_loop_contents();
      }
    }

    else if (potentiometerTime > 1)
    {
      int numberOfFlashLogs = getNumberOfFlashLogs();
      int rotPosition = 0;
      bool exit = false;
      while (!exit)
      {
        rotPosition = scroll(rotPosition, 43200, numberOfFlashLogs);
        rotPosition = scroll(rotPosition, 1440, numberOfFlashLogs);
        rotPosition = scroll(rotPosition, 60, numberOfFlashLogs);
        rotPosition = scroll(rotPosition, 1, numberOfFlashLogs);
        if (getRotaryEncoderSWPushState())
        {
          lcd_clear_screen();
          lcd_home();
          updateLCD(pot_state);
          while (getRotaryEncoderSWPushState())
          {
          };
          sleep_ms(100);
          while (!getRotaryEncoderSWPushState())
          {
          };
          exit = true;
          break;
        }
      }
    }

#if ENABLE_DORMANT
    if (hasResultBeenTaken)
    {
      hasResultBeenTaken = false;

#if ENABLE_PROTOTYPE_LED
      gpio_put(LED_PIN, 1);
#endif

      scb_orig = scb_hw->scr;
      clock0_orig = clocks_hw->sleep_en0;
      clock1_orig = clocks_hw->sleep_en1;

      sleep_run_from_xosc();

      i2c_writeRTCdata(0xE, 0b00000001); // enable alarm
      for(int i = 0; i < 60; ++i) { // Sleep for 60 secs or until SW down
        if(getRotaryEncoderSWPushState() || getUnixTime() - timeUntilLcdOffStart <= 30) {
          break;
        }
        sleep_goto_dormant_until_pin(SQW_PIN, true, true);
        sleep_goto_dormant_until_pin(SQW_PIN, true, false);
      }


      // sleep_goto_dormant_until_pin(SQW_PIN, true, true);
      // while(!awake) {}

      // Re-enable ring Oscillator control
      rosc_write(&rosc_hw->ctrl, ROSC_CTRL_ENABLE_LSB);

      i2c_writeRTCdata(0xE, 0b00000000); // disable alarm

      // reset procs back to default
      scb_hw->scr = scb_orig;
      clocks_hw->sleep_en0 = clock0_orig;
      clocks_hw->sleep_en1 = clock1_orig;

      // stdio_usb_init();
      stdio_uart_init();

      i2c_init(I2C_PORT, 50 * 1000);
      uart_init(UART_ID, BAUD_RATE);

#if ENABLE_PROTOTYPE_LED
      gpio_put(LED_PIN, 0);
#endif
    }
#endif
  }
}

int main()
{
  gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
  gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
  gpio_init(LED_PIN);
  gpio_init(SQW_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);
  gpio_set_dir(SQW_PIN, GPIO_IN);

  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
  gpio_pull_up(I2C_SDA);
  gpio_pull_up(I2C_SCL);
  gpio_pull_up(SQW_PIN);

  initPins();
  init_lcd();

  // DS3231 SQW Alarm for Waiting
  i2c_writeRTCdata(0x7, 0b10000000);
  i2c_writeRTCdata(0x8, 0b10000000);
  i2c_writeRTCdata(0x9, 0b10000000);
  i2c_writeRTCdata(0xA, 0b10000000);
  i2c_writeRTCdata(0xE, 0b00000001);

  lcd_write_chars("!!!!", 4);
  recorder_init();
  findPosInFlash();
  if (!isFormatted())
  {
    lcd_clear_screen();
    lcd_home();
    lcd_write_chars("Erasing data", 12);
    if (isFormatted(0))
    {
      eraseBlock(0);
    }
    writePrimarySignature();
    sleep_ms(1000);
    lcd_clear_screen();
    lcd_home();
  }

  init_rotaryEncoder();
  mainLoop();
}
