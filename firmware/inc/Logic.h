State(  "menu", initialTag,
  
  State(  "menu_lapcount", initialTag,
    OnEntry( [](){
      print_dbg("entry menu_lapcount\n");
      set_start_lights(0,0,100);
    }),
    Transition(OnEvent("encoder_press"), Target("config_lapcount")),
    Transition(OnEvent("encoder_up"), Target("menu_settings")),
    Transition(OnEvent("encoder_down"), Target("menu_timeattack"))
  )
  ,
  State(  "menu_timeattack",
    OnEntry( [](){
      print_dbg("entry menu_timeattack\n");
      set_start_lights(0,100,100);
    }),
    Transition(OnEvent("encoder_press"), Target("config_timeattack")),
    Transition(OnEvent("encoder_up"), Target("menu_lapcount")),
    Transition(OnEvent("encoder_down"), Target("menu_settings"))
  )
  ,
  State(  "menu_settings",
    OnEntry( [](){
      print_dbg("entry menu_settings\n");
      set_start_lights(100,100,0);
    }),
    Transition(OnEvent("encoder_press"), Target("settings")),
    Transition(OnEvent("encoder_up"), Target("menu_timeattack")),
    Transition(OnEvent("encoder_down"), Target("menu_lapcount"))
  )
  
),
State( "config_lapcount",
  OnEntry([]() {
    print_dbg("entry config_lapcount\n");
    set_start_lights(0, 100, 100);
  }),
  State( "config_lapcount_laps", initialTag,
    OnEntry([]() {
      print_dbg("entry config_lapcount_laps\n");
    }),
    Transition(OnEvent("encoder_press"), Target("config_lapcount_players")),
    OnEvent("encoder_up", [&settings]() {
      settings.lapCount++; 
    }),
    OnEvent("encoder_down", [&settings]() {
      settings.lapCount--;
    })
  ),
  State("config_lapcount_players",
    OnEntry([]() {
      print_dbg("entry config_lapcount_players\n");
    }),
    Transition(OnEvent("encoder_press"), Target("run_lapcount")),
    OnEvent("encoder_up", [&settings]() {settings.player2 = !settings.player2; }),
    OnEvent("encoder_down", [&settings]() {settings.player2 = !settings.player2; })
  )
),
State("config_timeattack",
  OnEntry([]() {
    print_dbg("entry config_timeattack\n");
  })
),
State("settings",
  OnEntry([]() {
    print_dbg("entry settings\n");
  }),
  State("settings_reset", initialTag,
    OnEntry([]() {
      print_dbg("entry settings_reset\n");
    }),
    Transition(OnEvent("encoder_up"), Target("settings_back")),
    Transition(OnEvent("encoder_down"), Target("settings_calibrate"))
  ),
  State("settings_calibrate", 
    OnEntry([]() {
      print_dbg("entry settings_calibrate\n");
    }),
    Transition(OnEvent("encoder_up"), Target("settings_reset")),
    Transition(OnEvent("encoder_down"), Target("settings_back"))
  ),
  State("settings_back",
     OnEntry([]() {
      print_dbg("entry settings_back\n");
    }),
    Transition(OnEvent("encoder_press"), Target("menu")),
    Transition(OnEvent("encoder_up"), Target("settings_reset")),
    Transition(OnEvent("encoder_down"), Target("settings_reset"))
  )
),
State( "run_lapcount"

)