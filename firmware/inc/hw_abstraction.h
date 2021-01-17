
#ifdef __cplusplus
#include <cstdint>

extern "C" {
  
#else
#include <stdint.h>
typedef int bool;
#endif

void hw_init();

void toggleDbgLed();

void set_start_lights(uint16_t red, uint16_t green, uint16_t blue);

typedef enum {
  Increment,
  Decrement,
  Stall
} EncoderStatus;

EncoderStatus get_encoder_status();

bool get_encoder_pressed();

void sleep_ms(int ms);

void print_dbg(char* msg);
void fprint_dbg(char* format, ...);

typedef enum {
  Var_2players,
  Var_lapCount,
  Var_sensor1,
  Var_sensor2,
  Var_bestLap
} Variable;

uint32_t writeVariable(Variable var);
void readVariable(uint32_t value, Variable var);

#ifdef __cplusplus
}
#endif