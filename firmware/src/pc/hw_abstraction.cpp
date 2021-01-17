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
  
  HttpServer();
  void operator()();

  void updateStartLights(std::array<std::tuple<uint8_t, uint8_t, uint8_t>, 3>&& pLightsColor);

  void addDebug(const std::string& pDebugMessage);

  int8_t getEncoderOffset();

  bool getEncoderPressed();
  
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
      const slEvents = new EventSource("start_lights");
      slEvents.onerror = function (e) {console.log('error:'+JSON.stringify(e));}
      slEvents.onopen = function (e) {console.log('open:'+JSON.stringify(e));}
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
    </script>
  </head>
  <body>
    <div style="position:absolute;height:50px;width:130px;border:1px solid black;">
      <div id="start_light_left"   style="position:absolute;top:10px;left:10px;width:30px;height:30px;background-color:grey;border:1px solid black" ></div>
      <div id="start_light_center" style="position:absolute;top:10px;left:50px;width:30px;height:30px;background-color:grey;border:1px solid black" ></div>
      <div id="start_light_right"  style="position:absolute;top:10px;left:90px;width:30px;height:30px;background-color:grey;border:1px solid black" ></div>
      </div>
    </div>
    <div id="encoder" style="position:absolute;top:60px;width:130px;height:100px;border:1px solid black" onwheel="encoderMove(event.deltaY*-1);" onmouseup="encoderPress();" ><h1>Encoder scroll</h1></div>
    <div id="debug" style="position:absolute;top:160px;width:auto;height:auto;border:1px solid black">
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

std::unique_ptr<HttpServer> gServer;

void hw_init(){
  
  gServer = std::make_unique<HttpServer>();

}


void toggleDbgLed(){
  
}

void set_start_lights(uint16_t red, uint16_t green, uint16_t blue){
  std::array<std::tuple<uint8_t, uint8_t, uint8_t>, 3> lLights{ std::make_tuple(red , green, blue), std::make_tuple(red , green, blue), std::make_tuple(red , green, blue) };
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


