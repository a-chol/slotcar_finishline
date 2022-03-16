#include <hw_abstraction.h>

#include "httplib.h"

#include <chrono>
#include <thread>
#include <memory>
#include <thread>

struct HttpServer {
  std::thread mHttpThread;

  std::atomic_bool mHasClient;

  std::condition_variable mLightsUpdate;
  std::mutex mLightsColorMtx;
  std::array<std::tuple<uint8_t, uint8_t, uint8_t>, 3> mLightsColor;

  std::condition_variable mDebugUpdate;
  std::mutex mDebugMtx;
  std::vector<std::string> mDebug;

  std::atomic_char mEncoderOffset;

  std::atomic_bool mEncoderPressed;

  std::atomic<uint16_t> mTrackSensor1;
  std::atomic<uint16_t> mTrackSensor2;
  
  std::condition_variable mScreenUpdate;
  std::mutex mScreenMtx;
  std::vector<std::string> mScreenCommands;
  
  
  HttpServer();
  void operator()();

  void updateStartLights(std::array<std::tuple<uint8_t, uint8_t, uint8_t>, 3>&& pLightsColor);

  void addDebug(const std::string& pDebugMessage);

  int8_t getEncoderOffset();

  bool getEncoderPressed();

  uint16_t getTrack1Sensor();
  uint16_t getTrack2Sensor();
  
  void addScreenCommand(const std::string& pCmd);
};

HttpServer::HttpServer()
  : mHttpThread(&HttpServer::operator(), this) {

}

void HttpServer::operator()() {
  httplib::Server mHttpServer;

  mHasClient.store(false);

  mHttpServer.Get("/", [](const httplib::Request&, httplib::Response& res) {
    res.set_content(R"--(
<html>
  <head>
    <script>
      var sensorMaxScroll = 0;
      var sensorTimeout;
      window.onload = function(){
        calibrateSensorScroll();
      };
      
      const slEvents = new EventSource("start_lights");
      slEvents.addEventListener('message',function(e) {
        var req = JSON.parse(e.data);
        if (req.type == "set_color"){
          req.lights.forEach(function(elm, idx){
            if (idx==0){
              document.getElementById('start_light_left').style.backgroundColor='rgb('+elm.r+', '+elm.g+', '+elm.b+')';
            } else if (idx==1){
              document.getElementById('start_light_center').style.backgroundColor='rgb('+elm.r+', '+elm.g+', '+elm.b+')';
            } else if (idx==2){
              document.getElementById('start_light_right').style.backgroundColor='rgb('+elm.r+', '+elm.g+', '+elm.b+')';
            } 
          });
          
        }
      });
      function encoderMove(delta){
        var xhr = new XMLHttpRequest();
        xhr.open('GET', 'encoder?dir='+(delta>0?1:-1));
        xhr.send();
      }

      function encoderPress(){
        var xhr = new XMLHttpRequest();
        xhr.open('GET', 'encoder?press=1');
        xhr.send();
      }

      const dbgEvents = new EventSource("debug");
      dbgEvents.addEventListener('message',function(e) {
        var dbgElm = document.getElementById('debug_list');
        dbgElm.innerHTML += "<li>"+e.data+"</li>";
        dbgElm.scrollTop = dbgElm.scrollHeight;
      });
      
      const screenEvents = new EventSource("screen");
      screenEvents.addEventListener('message',function(e) {
        var canvas = document.getElementById('screen');
        var req = JSON.parse(e.data);
        if (req.type == "line"){
          var ctx = canvas.getContext("2d");
          ctx.beginPath();
          ctx.strokeStyle = "#1CBEC9";
          ctx.moveTo(req.x1*4, req.y1*4);
          ctx.lineTo(req.x2*4, req.y2*4);
          ctx.stroke(); 
        } else if (req.type == "text"){
          var ctx = canvas.getContext("2d");
          ctx.fillStyle = "#1CBEC9";
          ctx.font = (req.size*4)+"px monospace";
          ctx.fillText(req.text, req.x*4, req.y*4+req.size*2); 
        } else if (req.type == "clear"){
          var ctx = canvas.getContext("2d");
          ctx.fillStyle = "#11064C";
          ctx.fillRect(0, 0, 512, 256);
        }
        else if (req.type == "clean"){
          var ctx = canvas.getContext("2d");
          ctx.fillStyle = "#11064C";
          ctx.fillRect(req.coord.x1*4, req.coord.y1*4, req.coord.x2*4, req.coord.y2*4);
        }
      });

      function tracksensor(num, val){
        console.log('send tracksensor?id='+num+'&val='+val);
        clearTimeout(sensorTimeout);
        if (val>0 && val < 1024){
          sensorTimeout = setTimeout(function(){
            console.log('send tracksensor?id='+num+'&val='+val);
            var xhr = new XMLHttpRequest();
            xhr.open('GET', 'tracksensor?id='+num+'&val='+val);
            xhr.send();
          }, 500);
        } else {
          var xhr = new XMLHttpRequest();
          xhr.open('GET', 'tracksensor?id='+num+'&val='+val);
          xhr.send();
        }
        
      }
      
      function calibrateSensorScroll(){
        var elm = document.getElementById('track_sensor1');
        var set = 1000;
        var step = 1000;
        elm.scrollTop = set;
        while (Math.abs(elm.scrollTop-set) < 2 && step > 1){
          step = step/2;
          if (elm.scrollTop < set){
            set = set - step;
          } else {
            set = set + step;
          }
          elm.scrollTop = set;
        }
        sensorMaxScroll = elm.scrollTop;
        elm.scrollTop = 0;
      }

      function getScrolledSensorValue(sensorid){
        var elm = document.getElementById('track_sensor'+sensorid);
        var val =  Math.round((1-(elm.scrollTop/sensorMaxScroll))*1024);
        console.log("sensor1:"+val);
        document.getElementById('track_sensor'+sensorid+'_value').innerHTML = val;
        return val;
      }
    </script>
  </head>
  <body>
    <div style="position:absolute;height:50px;width:130px;border:1px solid black;">
      <div id="start_light_left"   style="position:absolute;top:10px;left:10px;width:30px;height:30px;background-color:grey;border:1px solid black" ></div>
      <div id="start_light_center" style="position:absolute;top:10px;left:50px;width:30px;height:30px;background-color:grey;border:1px solid black" ></div>
      <div id="start_light_right"  style="position:absolute;top:10px;left:90px;width:30px;height:30px;background-color:grey;border:1px solid black" ></div>
      </div>
    </div>
    <div id="encoder" style="position:absolute;top:60px;width:130px;height:100px;border:1px solid black" onwheel="event.preventDefault();event.stopPropagation();encoderMove(event.deltaY*-1);" onmouseup="encoderPress();" ><h1>Encoder scroll</h1></div>
    <div id="track_sensor1" style="position:absolute;top:162px;width:130px;height:200px;border:1px solid black;overflow-y:scroll;overflow-x:hidden;" onmousedown="tracksensor(1,1024);" onmouseup="tracksensor(1,0);" onscroll="tracksensor(1,getScrolledSensorValue(1))">
      <h4 style="position:sticky;top:0px;left:4px;">track 1 sensor</h4>
      <h4 id="track_sensor1_value" style="position:sticky;top:20px;left:6px;"></h4>
      <br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/>
      <br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/>
      <br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/>
    </div>
    <div id="track_sensor2" style="position:absolute;top:162px;left:142px;width:130px;height:200px;border:1px solid black;overflow-y:scroll;overflow-x:hidden;" onmousedown="tracksensor(2,1024);" onmouseup="tracksensor(2,0);" onscroll="tracksensor(2,getScrolledSensorValue(2))">
      <h4 style="position:sticky;top:0px;left:4px;">track 2 sensor</h4>
      <h4 id="track_sensor2_value" style="position:sticky;top:20px;left:6px;"></h4>
      <br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/>
      <br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/>
      <br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/>
    </div>
    <canvas id="screen" width="512" height="256" style="border:3px solid blue;background-color:#11064C;position:absolute;left:300px;">
      
    </canvas>
    <div id="debug" style="position:absolute;top:390px;width:auto;height:auto;border:1px solid black">
      <input type="button" onclick="document.getElementById('debug_list').innerHTML = '';" value="clear"/>
      debug:<ul id="debug_list" style="width:200px;height:600px;overflow-y:scroll;"></ul>
    </div>
  </body>
</html>
)--", "text/html");
  });

  mHttpServer.Get("/start_lights", [&, this](const httplib::Request& req, httplib::Response& res) {

    bool lExp = false;
    if (!mHasClient.compare_exchange_strong(lExp, true)) {
      res.status = 500;
      return;
    }
    
    res.set_header("Cache-Control", "no-cache");
    res.status = 200;
    res.set_chunked_content_provider("text/event-stream",
      [&](size_t /*offset*/, httplib::DataSink& sink) {
      std::unique_lock <std::mutex> lLock(mLightsColorMtx);
      mLightsUpdate.wait(lLock);
      if (sink.is_writable()) {
        std::string lStartLightsUpdate = std::string("data: {\"type\":\"set_color\",\"lights\":[")+
          "{\"r\":"+ std::to_string(std::get<0>(mLightsColor[0]))+",\"g\":"+ std::to_string(std::get<1>(mLightsColor[0])) +", \"b\":"+ std::to_string(std::get<2>(mLightsColor[0])) +"},"+
          "{\"r\":"+ std::to_string(std::get<0>(mLightsColor[1]))+",\"g\":"+ std::to_string(std::get<1>(mLightsColor[1])) +", \"b\":"+ std::to_string(std::get<2>(mLightsColor[1])) +"},"+
          "{\"r\":"+ std::to_string(std::get<0>(mLightsColor[2]))+",\"g\":"+ std::to_string(std::get<1>(mLightsColor[2])) +", \"b\":"+ std::to_string(std::get<2>(mLightsColor[2])) +"}"+
          "]}\n\n";
        sink.write(lStartLightsUpdate.data(), lStartLightsUpdate.size());
      }
      return true;
    });
  });

  mHttpServer.Get("/debug", [&, this](const httplib::Request& req, httplib::Response& res) {
    res.set_header("Cache-Control", "no-cache");
    res.status = 200;
    res.set_chunked_content_provider("text/event-stream",
      [&](size_t /*offset*/, httplib::DataSink& sink) {

      std::unique_lock<std::mutex> lLock(mDebugMtx);

      if (mDebug.size() == 0) {
        mDebugUpdate.wait(lLock);
      }
      
      if (sink.is_writable()) {
        for (auto&& lDebugMsg : mDebug) {
          std::string lDebugUpdate = "data: " + lDebugMsg + "\n\n";
          sink.write(lDebugUpdate.data(), lDebugUpdate.size());
        }
        mDebug.clear();
      }
      return true;
    });
  });

  mHttpServer.Get("/encoder", [&, this](const httplib::Request& req, httplib::Response& res) {
    auto lParamDir = req.params.find("dir");
    if (lParamDir != req.params.end()) {
      std::istringstream lStm(lParamDir->second);
      uint16_t lDir;
      lStm >> lDir;
      mEncoderOffset.fetch_add(static_cast<int8_t>(lDir));
    }

    lParamDir = req.params.find("press");
    if (lParamDir != req.params.end()) {
      if (lParamDir->second == "1") {
        mEncoderPressed.store(true);
      }
    }
  });

  mHttpServer.Get("/tracksensor", [&, this](const httplib::Request& req, httplib::Response& res) {
    uint16_t lId = -1;
    uint16_t lState = 0;
    auto lParamId = req.params.find("id");
    if (lParamId != req.params.end()) {
      std::istringstream lStm(lParamId->second);
      lStm >> lId;
      if (lStm.bad() || (lId != 1 && lId != 2))
        return;
    }

    auto lParamState = req.params.find("val");
    if (lParamState != req.params.end()) {
      std::istringstream lStm(lParamState->second);
      lStm >> lState;
      if (lStm.bad() || (lState < 0 || lState > 1024))
        return;
    }

    if (lId == 1) {
      mTrackSensor1.store(lState);
    }
    else {
      mTrackSensor2.store(lState);
    }
  });
  
  mHttpServer.Get("/screen", [&, this](const httplib::Request& req, httplib::Response& res) {
    res.set_header("Cache-Control", "no-cache");
    res.status = 200;
    res.set_chunked_content_provider("text/event-stream",
      [&](size_t /*offset*/, httplib::DataSink& sink) {

      std::unique_lock<std::mutex> lLock(mScreenMtx);

      if (mScreenCommands.size() == 0) {
        mScreenUpdate.wait(lLock);
      }
      
      if (sink.is_writable()) {
        for (auto&& lScreenCommand : mScreenCommands) {
          std::string lScreenUpdate = "data: " + lScreenCommand + "\n\n";
          sink.write(lScreenUpdate.data(), lScreenUpdate.size());
        }
        mScreenCommands.clear();
      }
      return true;
    });
  });

  mHttpServer.listen("0.0.0.0", 80);
}

void HttpServer::updateStartLights(std::array<std::tuple<uint8_t, uint8_t, uint8_t>, 3>&& pLightsColor) {
  if (!mHasClient.load()) {
    return;
  }

  std::lock_guard<std::mutex> lLock(mLightsColorMtx);
  mLightsColor = std::move(pLightsColor);
  mLightsUpdate.notify_all();
}

void HttpServer::addDebug(const std::string& pDebugMessage) {
  if (!mHasClient.load()) {
    return;
  }

  std::lock_guard<std::mutex> lLock(mDebugMtx);
  mDebug.push_back(pDebugMessage);
  mDebugUpdate.notify_all();
}

int8_t HttpServer::getEncoderOffset() {
  return mEncoderOffset.exchange(0);
}

bool HttpServer::getEncoderPressed() {
  return mEncoderPressed.exchange(false);
}

uint16_t HttpServer::getTrack1Sensor() {
  return mTrackSensor1.load();
}

uint16_t HttpServer::getTrack2Sensor() {
  return mTrackSensor2.load();
}

void HttpServer::addScreenCommand(const std::string& pCmd){
  if (!mHasClient.load()) {
    return;
  }

  std::lock_guard<std::mutex> lLock(mScreenMtx);
  mScreenCommands.push_back(pCmd);
  mScreenUpdate.notify_all();
}

std::unique_ptr<HttpServer> gServer;

bool hw_init(){
  
  gServer = std::make_unique<HttpServer>();
  return true;
}


void toggleDbgLed(){
  
}

void set_start_lights(uint8_t red, uint8_t green, uint8_t blue){
  std::array<std::tuple<uint8_t, uint8_t, uint8_t>, 3> lLights{ std::make_tuple(red , green, blue), std::make_tuple(red , green, blue), std::make_tuple(red , green, blue) };
  gServer->updateStartLights(std::move(lLights));
}

void set_start_lights3(uint8_t red1, uint8_t green1, uint8_t blue1, uint8_t red2, uint8_t green2, uint8_t blue2, uint8_t red3, uint8_t green3, uint8_t blue3) {
  std::array<std::tuple<uint8_t, uint8_t, uint8_t>, 3> lLights{ std::make_tuple(red1 , green1, blue1), std::make_tuple(red2 , green2, blue2), std::make_tuple(red3 , green3, blue3) };
  gServer->updateStartLights(std::move(lLights));
}

EncoderStatus get_encoder_status(){
  auto lOffset = gServer->getEncoderOffset();
  if (lOffset > 0) {
    return Increment;
  }
  else if (lOffset < 0) {
    return Decrement;
  }
  return Stall;
}

bool get_encoder_pressed() {
  return gServer->getEncoderPressed();
}

uint16_t get_sensor_track_1() {
  return gServer->getTrack1Sensor();
}

uint16_t get_sensor_track_2() {
  return gServer->getTrack2Sensor();
}

void sleep_ms(int ms){
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void print_dbg(char* msg) {
  gServer->addDebug(msg);
}

void fprint_dbg(char* format, ...) {
  std::string lRes(strlen(format) * 2 + 30, '\0');
  va_list arglist;
  va_start(arglist, format);
  auto len = vsprintf(&lRes[0], format, arglist);
  va_end(arglist);
  lRes.resize(len);
  gServer->addDebug(lRes);
}

uint32_t get_clock_ms() {
  return clock();
}

void screenClear(){
  gServer->addScreenCommand("{\"type\":\"clear\"}");
}

void screenWrite(const char* text, FontSize size, uint8_t x, uint8_t y){
  gServer->addScreenCommand("{\"type\":\"text\",\"text\":\""+std::string(text)+"\",\"size\":\""+std::to_string(size)+"\",\"x\":\""+std::to_string(x)+"\",\"y\":\""+std::to_string(y)+"\"}");
}

void screenLineDraw(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2){
  gServer->addScreenCommand("{\"type\":\"line\",\"x1\":\""+std::to_string(x1)+"\",\"y1\":\""+std::to_string(y1)+"\",\"x2\":\""+std::to_string(x2)+"\",\"y2\":\""+std::to_string(y2)+"\"}");
}

void screenUpdate() {

}

void screenClean(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2) {
  gServer->addScreenCommand("{\"type\":\"clean\", \"coord\":{\"x1\":"+std::to_string(x1)+",\"y1\":"+ std::to_string(y1)+",\"x2\":" + std::to_string(x2) + ",\"y2\":" + std::to_string(y2) + "}}");
}

void writeVariable(Variable var, uint16_t value) {

}

uint16_t readVariable(Variable var, uint16_t* value) {
  return 1;
}


