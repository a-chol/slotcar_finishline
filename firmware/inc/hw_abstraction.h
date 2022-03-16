
#ifdef __cplusplus
#include <cstdint>

extern "C" {
  
#else
#include <stdint.h>
typedef int bool;
#endif

bool hw_init();

void toggleDbgLed();

void set_start_lights(uint8_t red, uint8_t green, uint8_t blue);
void set_start_lights3(uint8_t red1, uint8_t green1, uint8_t blue1, uint8_t red2, uint8_t green2, uint8_t blue2, uint8_t red3, uint8_t green3, uint8_t blue3);

typedef enum {
  Increment,
  Decrement,
  Stall
} EncoderStatus;

EncoderStatus get_encoder_status();

bool get_encoder_pressed();

uint16_t get_sensor_track_1();
uint16_t get_sensor_track_2();

void sleep_ms(int ms);

void print_dbg(const char* msg);
void fprint_dbg(const char* format, ...);

uint32_t get_clock_ms();

bool eeprom_write(uint32_t pOffset, uint8_t* pBuffer, uint32_t pSize);
bool eeprom_read(uint32_t pOffset, uint32_t pSize, uint8_t* pBuffer);

bool eeprom_clear();

typedef enum {
  Font_8 = 8,
  Font_10 = 10,
  Font_18 = 18,
  Font_26 = 26,
} FontSize;

void screenClear();
void screenUpdate();
void screenWrite(const char* text, FontSize size, uint8_t x, uint8_t y);
void screenLineDraw(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);
void screenClean(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);

#ifdef __cplusplus
}
#endif