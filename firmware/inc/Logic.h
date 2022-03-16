
State("init", initialTag,
  State("init_hw", initialTag, 
    OnEntry([](StateMachine& pLogic) {
      bool ok = hw_init();
      print_dbg("init_hw\n");

      sleep_ms(1000);
      if (ok){
        pLogic.pushEvent("init_hw_ok");
      }
      else {
        pLogic.pushEvent("init_hw_nok");
      }
    }),
    Transition(OnEvent("init_hw_ok"), Target("init_mem")),
    Transition(OnEvent("init_hw_nok"), Target("init_hw_error"))
  ),
  State("init_hw_error",
    OnEntry([](StateMachine& pLogic) {
      screenClear();
      screenWrite("ERREUR HW", Font_18, 38, 30);
      screenUpdate();
      set_start_lights(255, 0, 0);
    })
  ),
  State("init_mem", 
    OnEntry([&settings](StateMachine& pLogic) {
      loadOrInitSettings(settings);
      // readVariable(Var_sensor1Trhsld, &settings.proxSensor1Set);
      // readVariable(Var_sensor2Trhsld, &settings.proxSensor2Set);
      // readVariable(Var_2players, reinterpret_cast<uint16_t*>(&settings.player2));
      // readVariable(Var_lapCount, reinterpret_cast<uint16_t*>(&settings.lapCount));
      // readVariable(Var_lightIntensity, reinterpret_cast<uint16_t*>(&settings.lightIntensity));
      pLogic.pushEvent("init_mem_ok");
    }),
    Transition(OnEvent("init_mem_ok"), Target("menu"))
  )
),
State(  "menu", 
  
  State(  "menu_lapcount", initialTag,
    OnEntry( [](){
      print_dbg("entry menu_lapcount\n");
      screenClear();
      screenWrite("TOURS", Font_18, 38, 30);
      screenUpdate();
    }),
    Transition(OnEvent("encoder_press"), Target("config_lapcount")),
    Transition(OnEvent("encoder_up"), Target("menu_settings")),
    Transition(OnEvent("encoder_down"), Target("menu_timeattack"))
  )
  ,
  State(  "menu_timeattack",
    OnEntry( [](){
      print_dbg("entry menu_timeattack\n");
      screenClear();
      screenWrite("TEMPS", Font_18, 38, 30);
      screenUpdate();
    }),
    Transition(OnEvent("encoder_press"), Target("config_timeattack")),
    Transition(OnEvent("encoder_up"), Target("menu_lapcount")),
    Transition(OnEvent("encoder_down"), Target("menu_settings"))
  )
  ,
  State(  "menu_settings",
    OnEntry( [](){
      print_dbg("entry menu_settings\n");
      screenClear();
      screenWrite("CONFIG", Font_18, 30, 30);
      screenUpdate();
    }),
    Transition(OnEvent("encoder_press"), Target("settings") ),
    Transition(OnEvent("encoder_up"), Target("menu_timeattack")),
    Transition(OnEvent("encoder_down"), Target("menu_lapcount"))
  )
  
),
#ifndef CONFIG_LAPCOUNT
State( "config_lapcount",
  OnEntry([]() {
    print_dbg("entry config_lapcount\n");
  }),
  State( "config_lapcount_laps", initialTag,
    OnEntry([&settings]() {
      print_dbg("entry config_lapcount_laps\n");
      screenClear();
      screenWrite("TOURS", Font_18, 30, 12);
      screenWrite("NB TOURS : ", Font_10, 20, 34);
      screenWrite(std::to_string(settings.lapCount).c_str(), Font_10, 80, 34);
      screenUpdate();
    }),
    Transition(OnEvent("encoder_press"), Target("config_lapcount_players"), Action([&settings](){
      writeVariable(Var_lapCount, settings.lapCount);
    })),
    Transition(OnEvent("encoder_up"), Target("config_lapcount_laps"),
      Action([&settings]() {
        settings.lapCount++; 
      })
    ),
    Transition(OnEvent("encoder_down"), Target("config_lapcount_laps"),
      Action([&settings]() {
        settings.lapCount--;
      })
    )
  ),
  State("config_lapcount_players",
    OnEntry([&settings]() {
      print_dbg("entry config_lapcount_players\n");
      screenClear();
      screenWrite("TOURS", Font_18, 30, 12);
      screenWrite("NB JOUEURS : ", Font_10, 20, 34);
      screenWrite(settings.player2?"2":"1", Font_10, 92, 34);
      screenUpdate();
    }),
    Transition(OnEvent("encoder_press"), Target("run_lapcount")),
    Transition(OnEvent("encoder_up"), Target("config_lapcount_players"),
      Action([&settings]() {
        settings.player2 = !settings.player2;
      })
    ),
    Transition(OnEvent("encoder_down"), Target("config_lapcount_players"),
      Action([&settings]() {
        settings.player2 = !settings.player2;
      })
    ),
    OnExit([&settings]() {
      writeVariable(Var_2players, settings.player2);
    })
  )
),
#endif
State("config_timeattack",
  OnEntry([]() {
    print_dbg("entry config_timeattack\n");
  })
),

#ifndef SETTINGS
State("settings",
  OnEntry([]() {
    print_dbg("entry settings\n");
    screenClear();
    screenWrite("CONFIG", Font_18, 30, 10);
    screenUpdate();
  }),
  State("settings_reset",
    OnEntry([]() {
      print_dbg("entry settings_reset\n");
      screenClear();
      screenWrite("CONFIG", Font_18, 30, 10);
      screenWrite("RESET", Font_10, 50, 30);
      screenLineDraw(30, 35, 35, 30);
      screenLineDraw(35, 30, 35, 40);
      screenLineDraw(35, 40, 30, 35);
      screenLineDraw(110, 35, 105, 30);
      screenLineDraw(105, 30, 105, 40);
      screenLineDraw(105, 40, 110, 35);
      screenUpdate();
    }),
    Transition(OnEvent("encoder_up"), Target("settings_back")),
    Transition(OnEvent("encoder_down"), Target("settings_calibrate")),
    Transition(OnEvent("encoder_press"), Action([](){eeprom_clear();}))
    
  ),
  State("settings_calibrate", 
    OnEntry([&runData, &settings]() {
      print_dbg("entry settings_calibrate\n");
      
    }),
    State("settings_calibrate_select", initialTag,
      OnEntry([](StateMachine& pLogic) {
        screenClear();
        screenWrite("CONFIG", Font_18, 30, 10);
        screenWrite("CALIB", Font_10, 50, 30);
        screenLineDraw(30, 35, 35, 30);
        screenLineDraw(35, 30, 35, 40);
        screenLineDraw(35, 40, 30, 35);
        screenLineDraw(110, 35, 105, 30);
        screenLineDraw(105, 30, 105, 40);
        screenLineDraw(105, 40, 110, 35);
        screenUpdate();
      }),
      Transition(OnEvent("encoder_press"), Target("settings_calibrate_track1")),
      Transition(OnEvent("encoder_up"), Target("settings_reset")),
      Transition(OnEvent("encoder_down"), Target("setting_light_intensity"))
    ),
    State("settings_calibrate_track1",
      OnEntry([](StateMachine& pLogic) {
        pLogic.pushEvent("settings_calibrate_track1_update");
        pLogic.pushEvent("settings_calibrate_track1_screenupdate");
        
      }),
      Transition(OnEvent("settings_calibrate_track1_screenupdate"), Action([&runData, &settings]() {
        screenClear();
        screenWrite("CONFIG", Font_18, 30, 10);
        screenWrite("CALIB", Font_10, 50, 30);
        screenWrite("PISTE 1", Font_10, 50, 40);
        char lBuffer[20];
        snprintf(lBuffer, 20, "%u", settings.proxSensor1Set);
        screenWrite(lBuffer, Font_10, 50, 52);
        screenUpdate();
      })),
      Transition(OnEvent("settings_calibrate_track1_update"), Action([&runData, &settings]() {
        print_dbg("settings_calibrate_track1_update\n");
        uint8_t lr1 = 0, lg1 = settings.lightIntensity, lb1 = 0;
        if (!runData.track1Sensor) {
          lg1 = 0;
          lr1 = settings.lightIntensity;
        }

        set_start_lights(lr1, lg1, lb1);
      })),
      Transition(OnEvent("track1_online"), Action([](StateMachine& pLogic) {pLogic.pushEvent("settings_calibrate_track1_update"); })),
      Transition(OnEvent("track1_offline"), Action([](StateMachine& pLogic) {pLogic.pushEvent("settings_calibrate_track1_update"); })),
      Transition(OnEvent("encoder_up"), Action([&settings](StateMachine& pLogic) {
        settings.proxSensor1Set = std::max(0, std::min(settings.proxSensor1Set+50, 1000));
        fprint_dbg("proxSensor1Set: %u\n", settings.proxSensor1Set);
        pLogic.pushEvent("settings_calibrate_track1_screenupdate");
      })),
      Transition(OnEvent("encoder_down"), Action([&settings](StateMachine& pLogic) {
        settings.proxSensor1Set = std::max(0, std::min(settings.proxSensor1Set - 50, 1000));
        fprint_dbg("proxSensor1Set: %u\n", settings.proxSensor1Set);
        pLogic.pushEvent("settings_calibrate_track1_screenupdate");
      })),
      Transition(OnEvent("encoder_press"), Target("settings_calibrate_track2")),
      OnExit([&settings]() {
        writeVariable(Var_sensor1Trhsld, settings.proxSensor1Set);
      })
    ),
    State("settings_calibrate_track2",
      OnEntry([](StateMachine& pLogic) {
        pLogic.pushEvent("settings_calibrate_track2_update");
        pLogic.pushEvent("settings_calibrate_track2_screenupdate");
      }),
      Transition(OnEvent("settings_calibrate_track2_screenupdate"), Action([&runData, &settings]() {
        screenClear();
        screenWrite("CONFIG", Font_18, 30, 10);
        screenWrite("CALIB", Font_10, 50, 30);
        screenWrite("PISTE 2", Font_10, 50, 40);
        char lBuffer[20];
        snprintf(lBuffer, 20, "%u", settings.proxSensor2Set);
        screenWrite(lBuffer, Font_10, 50, 52);
        screenUpdate();
      })),
      Transition(OnEvent("settings_calibrate_track2_update"), Action([&runData, &settings]() {
        uint8_t lr1 = 0, lg1 = settings.lightIntensity, lb1 = 0;    

        if (!runData.track2Sensor) {
          lg1 = 0;
          lr1 = settings.lightIntensity;
        }
        set_start_lights(lr1, lg1, lb1);
      })),
      Transition(OnEvent("track2_online"), Action([](StateMachine& pLogic) {pLogic.pushEvent("settings_calibrate_track2_update"); })),
      Transition(OnEvent("track2_offline"), Action([](StateMachine& pLogic) {pLogic.pushEvent("settings_calibrate_track2_update"); })),
      Transition(OnEvent("encoder_up"), Action([&settings](StateMachine& pLogic) {
        settings.proxSensor2Set = std::max(0, std::min(settings.proxSensor2Set + 50, 1000));
        fprint_dbg("proxSensor2Set: %u\n", settings.proxSensor2Set);
        pLogic.pushEvent("settings_calibrate_track2_screenupdate");
      })),
      Transition(OnEvent("encoder_down"), Action([&settings](StateMachine& pLogic) {
        settings.proxSensor2Set = std::max(0, std::min(settings.proxSensor2Set - 50, 1000));
        fprint_dbg("proxSensor2Set: %u\n", settings.proxSensor2Set);

        pLogic.pushEvent("settings_calibrate_track2_screenupdate");
      })),
      Transition(OnEvent("encoder_press"), Target("settings_calibrate")),
      OnExit([&settings]() {
        set_start_lights(0, 0, 0); 
        writeVariable(Var_sensor2Trhsld, settings.proxSensor2Set);
      })
    )
  ),
  State("setting_light_intensity",
    State("setting_light_intensity_select", initialTag,
      OnEntry([](StateMachine& pLogic) {
        print_dbg("entry setting_light_intensity\n");
        screenClear();
        screenWrite("CONFIG", Font_18, 30, 10);
        screenWrite(" FEUX ", Font_10, 50, 30);
        screenLineDraw(30, 35, 35, 30);
        screenLineDraw(35, 30, 35, 40);
        screenLineDraw(35, 40, 30, 35);
        screenLineDraw(110, 35, 105, 30);
        screenLineDraw(105, 30, 105, 40);
        screenLineDraw(105, 40, 110, 35);
        screenUpdate();
      }),
      Transition(OnEvent("encoder_press"), Target("setting_light_intensity_edit")),
      Transition(OnEvent("encoder_up"), Target("settings_calibrate")),
      Transition(OnEvent("encoder_down"), Target("settings_back"))
    ),
    State("setting_light_intensity_edit",
      OnEntry([](StateMachine& pLogic) {
        pLogic.pushEvent("setting_light_intensity_update");
        screenClear();
        screenWrite("CONFIG", Font_18, 30, 10);
        screenWrite(" FEUX ", Font_10, 50, 30);
        screenUpdate();
      }),
      Transition(OnEvent("setting_light_intensity_update"), Action([&runData, &settings]() {
        fprint_dbg("light intensity set to %d\n", settings.lightIntensity);
        set_start_lights(0, 0, settings.lightIntensity);
      })),
      Transition(OnEvent("encoder_up"), Action([&settings](StateMachine& pLogic) {
        settings.lightIntensity = std::max(0, std::min(settings.lightIntensity + 5, 255));
        pLogic.pushEvent("setting_light_intensity_update");
      })),
      Transition(OnEvent("encoder_down"), Action([&settings](StateMachine& pLogic) {
        settings.lightIntensity = std::max(0, std::min(settings.lightIntensity - 5, 255));
        pLogic.pushEvent("setting_light_intensity_update");
      })),
      Transition(OnEvent("encoder_press"), Target("setting_light_intensity_select")),
      OnExit([&settings]() {
        set_start_lights(0, 0, 0);
        writeVariable(Var_lightIntensity, settings.lightIntensity);
      })
    )
  ),
  State("settings_back", initialTag,
     OnEntry([]() {
      print_dbg("entry settings_back\n");
      screenClear();
      screenWrite("CONFIG", Font_18, 30, 10);
      screenWrite("RETOUR", Font_10, 50, 30);
      screenLineDraw(30, 35, 35, 30);
      screenLineDraw(35, 30, 35, 40);
      screenLineDraw(35, 40, 30, 35);
      screenLineDraw(110, 35, 105, 30);
      screenLineDraw(105, 30, 105, 40);
      screenLineDraw(105, 40, 110, 35);
      screenUpdate();
    }),
    Transition(OnEvent("encoder_press"), Target("menu")),
    Transition(OnEvent("encoder_up"), Target("setting_light_intensity")),
    Transition(OnEvent("encoder_down"), Target("settings_reset"))
  )
),

#endif
#ifndef RUN_LAPCOUNT
State( "run_lapcount",
  //do setup state pour s'aligner sur la ligne de départ
  //detect start crossing during countdown for false starts
  State("run_lapcount_set", initialTag,
    OnEntry([&runData](StateMachine& pMachine) {
      print_dbg("entry run_lapcount_set\n");
      screenClear();
      screenWrite("A", Font_18, 60, 4);
      screenWrite("VOS", Font_18, 50, 18);
      screenWrite("MARQUES", Font_18, 30, 32);
      screenUpdate();
      if (!runData.track1Sensor && !runData.track2Sensor) {
        pMachine.pushEvent("track1_offline");
      }
    }),
    State("run_set_online", initialTag, 
      OnEntry([&settings]() {
        print_dbg("entry run_set_online\n");
        set_start_lights(settings.lightIntensity, settings.lightIntensity*0.5, 0);
      }),
      Transition(OnEvent("track1_offline"), Condition([&runData]() {
        return !runData.track1Sensor && !runData.track2Sensor;
      }), Target("run_set_offline")),
      Transition(OnEvent("track2_offline"), Condition([&runData]() {
        return !runData.track1Sensor && !runData.track2Sensor;
      }), Target("run_set_offline"))
    ),
    State("run_set_offline", 
      OnEntry([&settings]() {
        print_dbg("entry run_set_offline\n");
        set_start_lights(0, settings.lightIntensity, 0);
      }),
      Transition(OnEvent("encoder_press"), Target("run_lapcount_countdown")),
      Transition(OnEvent("track1_online"), Target("run_set_online")),
      Transition(OnEvent("track2_online"), Target("run_set_online"))
    )
  ),
  State("run_lapcount_countdown",
    State("run_lapcount_countdown_3", initialTag,
      OnEntry([&settings]() {
        print_dbg("entry run_lapcount_countdown_3\n");
        set_start_lights(settings.lightIntensity, 0, 0);
        screenClear();
        screenWrite("3", Font_26, 56, 30);
        screenUpdate();
        addDeferredEvent("run_lapcount_go", 800);
      }),
      Transition(OnEvent("run_lapcount_go"), Target("run_lapcount_countdown_3b"))
    ),
    State("run_lapcount_countdown_3b",
      OnEntry([]() {
        set_start_lights(0, 0, 0);
        addDeferredEvent("run_lapcount_go", 200);
      }),
      Transition(OnEvent("run_lapcount_go"), Target("run_lapcount_countdown_2"))
    ),
    State("run_lapcount_countdown_2",
      OnEntry([&settings]() {
        print_dbg("entry run_lapcount_countdown_2\n");
        set_start_lights(settings.lightIntensity, 0, 0);
        screenClear();
        screenWrite("2", Font_26, 56, 30);
        screenUpdate();
        addDeferredEvent("run_lapcount_go", 800);
      }),
      Transition(OnEvent("run_lapcount_go"), Target("run_lapcount_countdown_2b"))
    ),
    State("run_lapcount_countdown_2b",
      OnEntry([]() {
        set_start_lights(0, 0, 0);
        addDeferredEvent("run_lapcount_go", 200);
      }),
      Transition(OnEvent("run_lapcount_go"), Target("run_lapcount_countdown_1"))
    ),
    State("run_lapcount_countdown_1",
      OnEntry([&settings]() {
        print_dbg("entry run_lapcount_countdown_1\n");
        set_start_lights(settings.lightIntensity, settings.lightIntensity, 0);
        screenClear();
        screenWrite("1", Font_26, 56, 30);
        screenUpdate();
        addDeferredEvent("run_lapcount_go", 800);
      }),
      Transition(OnEvent("run_lapcount_go"), Target("run_lapcount_countdown_1b"))
    ),
    State("run_lapcount_countdown_1b",
      OnEntry([]() {
        set_start_lights(0, 0, 0);
        addDeferredEvent("run_lapcount_go", 200);
      }),
      Transition(OnEvent("run_lapcount_go"), Target("run_lapcount_race"))
    ),
    Transition(OnEvent("track1_online"), Target("run_false_start")),
    Transition(OnEvent("track2_online"), Target("run_false_start"))
  ),
  State("run_false_start",
    //make lights blink and reset the race after 5 seconds
    OnEntry([]() {
      print_dbg("entry run_false_start\n");
      screenClear();
      screenWrite("FAUX", Font_18, 40, 18);
      screenWrite("DEPART", Font_18, 30, 32);
      screenUpdate();
      addDeferredEvent("run_false_start_reset", 5000);
    }),
    State("run_false_start_lightoff", initialTag,
      OnEntry([]() {
        set_start_lights(0, 0, 0);
        addDeferredEvent("run_false_start_switchlights", 500);
      }),
      Transition(OnEvent("run_false_start_switchlights"), Target("run_false_start_lighton"))
    ),
    State("run_false_start_lighton",
      OnEntry([&settings]() {
        set_start_lights(settings.lightIntensity, settings.lightIntensity*0.5, 0);
        addDeferredEvent("run_false_start_switchlights", 500);
      }),
      Transition(OnEvent("run_false_start_switchlights"), Target("run_false_start_lightoff"))
    ),
    Transition(OnEvent("run_false_start_reset"), Target("run_lapcount_set"))
  ),
  State("run_lapcount_race",
    OnEntry([&settings, &runData](ifsm::StateMachine& pLogic) {
      print_dbg("entry run_lapcount_race\n");
      runData.lapCountJ1 = settings.lapCount;
      runData.lapCountJ2 = settings.lapCount;
      runData.startTimeMs = get_clock_ms();

      set_start_lights(0, settings.lightIntensity, 0);

      pLogic.pushEvent("run_lapcount_race_time");
    }),
    Transition(OnEvent("run_lapcount_race_time"), 
      Action([&runData]() {
        updateScreen(runData);
        addDeferredEvent("run_lapcount_race_time", 100);
      }) 
    ),
    Transition(OnEvent("track1_offline"), Target("run_lapcount_finish"), Condition([&runData]() {
        runData.lapCountJ1--;
        if (runData.lapCountJ1 < 0){
          return true;
        }
        fprint_dbg("track1 laps %d\n", runData.lapCountJ1);
        updateScreen(runData);
        return false;
      })
    ),
    Transition(OnEvent("track2_offline"), Target("run_lapcount_finish"), Condition([&runData]() {
        runData.lapCountJ2--;
        if (runData.lapCountJ2 < 0) {
          return true;
        }
        fprint_dbg("track2 laps %d\n", runData.lapCountJ2);
        updateScreen(runData);
        return false;
      })
    ),
    Transition(OnEvent("encoder_press"), Target("run_lapcount_finish"))
  ),
  State("run_lapcount_finish",
    OnEntry([&runData]() {
        print_dbg("entry run_lapcount_finish\n");
        set_start_lights(0, 0, 0);
        if (runData.lapCountJ1 < 0) {
          screenClear();
          screenWrite("GAGNANT", Font_18, 20, 4);
          screenWrite("JOUEUR 1", Font_26, 0, 18);
          screenUpdate();
        } else if (runData.lapCountJ2 < 0) {
          screenClear();
          screenWrite("GAGNANT", Font_18, 20, 4);
          screenWrite("JOUEUR 2", Font_26, 0, 18);
          screenUpdate();
        }
    }),
    Transition(OnEvent("encoder_press"), Target("menu")),

    State("run_lapcount_finish_lightson", initialTag,
      OnEntry([&settings] (){
        set_start_lights(0, 0, settings.lightIntensity);
        addDeferredEvent("run_lapcount_finish_lights_update", 500);
      }),
      Transition(OnEvent("run_lapcount_finish_lights_update"), Target("run_lapcount_finish_lightsoff"))
    ),
    State("run_lapcount_finish_lightsoff",
      OnEntry([&settings]() {
        set_start_lights(0, 0, 0);
        addDeferredEvent("run_lapcount_finish_lights_update", 500);
      }),
      Transition(OnEvent("run_lapcount_finish_lights_update"), Target("run_lapcount_finish_lightson"))
    ),
    OnExit([]() {set_start_lights(0, 0, 0); })
  )
#endif
)