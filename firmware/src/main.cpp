#include "hw_abstraction.h"

#include <instantFSM.h>
 using namespace ifsm;

 struct Settings {
   bool player2 = true;
   uint8_t lapCount = 20;
   uint8_t proxSensor1Set = 10;
   uint8_t proxSensor2Set = 10;
   uint32_t bestLap = 0;
 };

extern "C" void comptetour_main(){
  
  hw_init();
  sleep_ms(1000);
#if 0
  while (1){
    get_encoder_status();
    char buf[10] = {0};
    snprintf(buf,10,"%d\n", (TIM3->CNT>>2));
    print_dbg(buf);
    sleep_ms(1000);
  }
#else
  Settings settings;
  
  StateMachine lLogic(
    #include "Logic.h"
  );

  lLogic.enter();
  
  while (1){
    
    auto lEncoder = get_encoder_status();
    if (lEncoder == Increment){
      lLogic.pushEvent("encoder_up");
    }
    else if(lEncoder == Decrement){
      lLogic.pushEvent("encoder_down");
    }

    if (get_encoder_pressed()) {
      lLogic.pushEvent("encoder_press");
    }
    
    
  }
#endif
}

