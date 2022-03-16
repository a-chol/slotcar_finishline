#include "hw_abstraction.h"

#include <array>
#include <tuple>
#include <algorithm>

#include <instantFSM.h>
 using namespace ifsm;

 struct Settings {
   uint16_t proxSensor1Set = 600;
   uint16_t proxSensor2Set = 600;
   uint32_t bestLap = 0;
   bool player2 = true;
   uint8_t lapCount = 20;
   uint8_t lightIntensity = 100;
 };

 struct RunData {
   int8_t lapCountJ1;
   int8_t lapCountJ2;
   uint32_t startTimeMs;
   bool track1Sensor;
   bool track2Sensor;
 };

 std::array<std::tuple<uint32_t, std::string>, 5> gDeferredEvents;

 void addDeferredEvent(const std::string& pEvent, uint32_t pDeadline) {
   for (auto& lEventLine : gDeferredEvents) {
     if (std::get<0>(lEventLine) == static_cast<uint32_t>(-1)) {
       std::get<0>(lEventLine) = get_clock_ms()+pDeadline;
       std::get<1>(lEventLine) = pEvent;
       break;
     }
   }
 }

 void checkDeferredEvents(StateMachine& pLogic) {
   for (auto& lEventLine : gDeferredEvents) {
     if (std::get<0>(lEventLine) <= get_clock_ms()) {
       pLogic.pushEvent(std::get<1>(lEventLine));
       std::get<0>(lEventLine) = static_cast<uint32_t>(-1);
       std::get<1>(lEventLine).clear();
     }
   }
 }
 
 void updateScreen(RunData& runData) {
   static RunData cacheData;
   static char buffer[10];
   if (cacheData.startTimeMs != runData.startTimeMs) {
     screenClear();
     screenLineDraw(5, 20, 123, 20);
     screenLineDraw(64, 20, 64, 64);
     cacheData.lapCountJ1 = runData.lapCountJ1 + 1;
     cacheData.lapCountJ2 = runData.lapCountJ2 + 1;
   }

   if (cacheData.lapCountJ1 != runData.lapCountJ1) {
     screenClean(15, 32, 45, 60);
     snprintf(buffer, 10, "%02d", runData.lapCountJ1);
     screenWrite(buffer, Font_26, 15, 32);
   }
   if (cacheData.lapCountJ2 != runData.lapCountJ2) {
     screenClean(80, 32, 115, 60);
     snprintf(buffer, 10, "%02d", runData.lapCountJ2);
     screenWrite(buffer, Font_26, 80, 32);
   }

   screenClean(2, 5, 100, 18);
   uint32_t lTime = get_clock_ms() - runData.startTimeMs;

   snprintf(buffer, 10, "%02d:%02d:%02d", (lTime / 60000), ((lTime / 1000) % 60), (lTime % 1000));
   screenWrite(buffer, Font_10, 32, 5);

   screenUpdate();
   cacheData = runData;
 }

typedef enum {
  Var_2players,
  Var_lapCount,
  Var_sensor1Trhsld,
  Var_sensor2Trhsld,
  Var_bestLap,
  Var_lightIntensity
} Variable;

uint32_t getOffsetForVar(Variable var){
  uint32_t lOffset = 0;
  Settings lRef;
  switch (var){
    case Var_2players:
    lOffset = (uint32_t)(&lRef.player2) - (uint32_t)(&lRef);
    break;
    case Var_lapCount:
    lOffset = (uint32_t)(&lRef.lapCount) - (uint32_t)(&lRef);
    break;
    case Var_sensor1Trhsld:
    lOffset = (uint32_t)(&lRef.proxSensor1Set) - (uint32_t)(&lRef);
    break;
    case Var_sensor2Trhsld:
    lOffset = (uint32_t)(&lRef.proxSensor2Set) - (uint32_t)(&lRef);
    break;
    case Var_bestLap:
    lOffset = (uint32_t)(&lRef.bestLap) - (uint32_t)(&lRef);
    break;
    case Var_lightIntensity:
    lOffset = (uint32_t)(&lRef.lightIntensity) - (uint32_t)(&lRef);
    break;
    default:
      return 0xFFFFFFFF;
  }
  
  return lOffset;
}

template <typename T>
void writeVariable(Variable var, T value){
  uint32_t lOffset = getOffsetForVar(var);
  if (lOffset == 0xFFFFFFFF){
    return;
  }
  fprint_dbg("write var %d at %d\n", value, lOffset);
  eeprom_write(lOffset, reinterpret_cast<uint8_t*>(&value), sizeof(T));
}

template <typename T>
uint16_t readVariable(Variable var, T* value){
  uint32_t lOffset = getOffsetForVar(var);
  if (lOffset == 0xFFFFFFFF){
    return 0;
  }
  return eeprom_read(lOffset, sizeof(T), reinterpret_cast<uint8_t*>(value));
}

void loadOrInitSettings(Settings& pSettings){
  return;
  if (!readVariable(Var_sensor1Trhsld, &pSettings.proxSensor1Set)){
    writeVariable(Var_sensor1Trhsld, pSettings.proxSensor1Set);
    writeVariable(Var_sensor2Trhsld, pSettings.proxSensor2Set);
    writeVariable(Var_bestLap, pSettings.bestLap);
    writeVariable(Var_2players, pSettings.player2);
    writeVariable(Var_lapCount, pSettings.lapCount);
    writeVariable(Var_lightIntensity, pSettings.lightIntensity);
    return;
  }
  readVariable(Var_sensor2Trhsld, &pSettings.proxSensor2Set);
  readVariable(Var_bestLap, &pSettings.bestLap);
  readVariable(Var_2players, &pSettings.player2);
  readVariable(Var_lapCount, &pSettings.lapCount);
  readVariable(Var_lightIntensity, &pSettings.lightIntensity);
}

extern "C" void comptetour_main(){
  sleep_ms(1000);
  
  // Settings settings;
  // uint8_t val = 0;
  // if (eeprom_read(0, sizeof(val), &val)){
    // fprint_dbg("read val %u\n", val);
  // } else {
    // val = 12;
    // eeprom_write(0, &val, sizeof(val));
    // print_dbg("wrote val\n");
  // }
  // val = 0;
  // eeprom_read(0, sizeof(val), &val);
  // fprint_dbg("read val %u\n", val);
  
  // while (1){
    // sleep_ms(1000);
  // }
  
#if 1

  std::fill(std::begin(gDeferredEvents), std::end(gDeferredEvents), std::make_tuple(-1, ""));

  Settings settings;
  RunData runData;

  runData.track1Sensor = false;
  runData.track2Sensor = false;
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

    auto lSensor1 = get_sensor_track_1();
    if ((lSensor1>settings.proxSensor1Set) != runData.track1Sensor){
      runData.track1Sensor = (lSensor1>settings.proxSensor1Set);
      lLogic.pushEvent(runData.track1Sensor ? "track1_online" : "track1_offline");
    }
    
    auto lSensor2 = get_sensor_track_2();
    if ((lSensor2>settings.proxSensor2Set) != runData.track2Sensor){
      runData.track2Sensor = (lSensor2>settings.proxSensor2Set);
      lLogic.pushEvent(runData.track2Sensor ? "track2_online" : "track2_offline");
    }

    checkDeferredEvents(lLogic);
    
  }
#endif
}

