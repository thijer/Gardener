#ifndef WEBINTERFACE_HPP
#define WEBINTERFACE_HPP


#include <functional>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>

#ifdef ENABLE_OTA
// #define OTA_TEST
#include <Update.h>
#ifdef OTA_TEST
#include "MD5Builder.h"
#endif
#endif

// #define CONFIG_ASYNC_TCP_STACK_SIZE 4096
#include "ESPAsyncWebServer.h"

#include "../Debug/Debug.hpp"
#include "credentials.h"

#ifdef ENABLE_FEEDER
extern bool start_feed(uint32_t, uint32_t);
#endif

class WebInterface : public Stream
{
    public:
        WebInterface(uint16_t port = 80, uint8_t ledpin = 0);
        // Management
        bool begin(Debug& debugger = emptydebug);
        bool start();
        bool stop();
        void loop();
        bool running();
        // bool no_reason_to_live();

        // void property_ap_timeout(IntegerProperty* p);

        // Stream interfaces
        int read();
        int peek();
        int available();

        // Print interface
        size_t write(uint8_t b);
        size_t write(const uint8_t *buffer, size_t size);

    private:
        AsyncWebServer server;
        AsyncWebSocket ws;
        Debug* debug;
        uint16_t port;
        uint8_t ledpin;

        #ifdef OTA_TEST
        MD5Builder md5;
        #endif
        String md5_str;

        // IntegerProperty* ap_timeout;
        bool is_running = false;
        uint32_t last_activity = 0;
        
        static const uint32_t SIZE = 128;
        uint8_t in_buffer[SIZE] = {0};
        uint32_t in_readptr = 0;
        uint32_t in_writeptr = 0;
        uint32_t in_n_bytes = 0;

        uint8_t out_buffer[SIZE] = {0};
        uint8_t* out_writeptr = &out_buffer[0];
        uint32_t out_n_bytes = 0;

        bool write_buffer(const uint8_t* data, uint32_t length);
        void reset_out_buffer();

        #ifdef ENABLE_FEEDER
        void cb_start_feed(AsyncWebServerRequest* request);
        void cb_abort_feed(AsyncWebServerRequest* request);
        #endif
        void cb_index(AsyncWebServerRequest* request);
        void cb_css(AsyncWebServerRequest* request);
        void cb_script(AsyncWebServerRequest* request);
        void cb_websocket_event(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len);
        #ifdef ENABLE_OTA
        void cb_firmware_upload_request(AsyncWebServerRequest *request);
        void cb_firmware_upload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
        void cb_firmware_meta(AsyncWebServerRequest *request);
        size_t bytecounter = 0;
        #endif
};

WebInterface::WebInterface(uint16_t port, uint8_t ledpin):
    server(port),
    port(port),
    ws("/ws"),
    ledpin(ledpin)
{}

// bool WebInterface::begin()
bool WebInterface::begin(Debug& debugger)
{
    using namespace std::placeholders;

    debug = &debugger;
    // debug->print("[WebGUI] test.");
    
    // Timeout property needs to be set before calling begin().
    // if(ap_timeout == nullptr) return false;

    // Webserver is already running.
    if(is_running) return true;
    
    if(ledpin)
    {
        pinMode(ledpin, OUTPUT);
        digitalWrite(ledpin, 0);
    }
    
    if(!SPIFFS.begin(true))
    {
        debug->print("[WebGUI] ERROR: mounting SPIFFS failed.");
        return false;
    }

    // Set up DNS
    // if(!MDNS.begin("gardener"))
    // {
    //     debug->print("[WebGUI] ERROR: mDNS failed.");
    //     return false;
    // }
    // MDNS.addService("http", "tcp", port);
    
    // Set HTTP endpoints.
    #ifdef ENABLE_OTA
    server.on(
        "/update/binary", 
        HTTP_POST, 
        std::bind(&WebInterface::cb_firmware_upload_request, this, _1), 
        std::bind(&WebInterface::cb_firmware_upload, this, _1, _2, _3, _4, _5, _6)
    );
    server.on(
        "/update/meta",
        HTTP_POST,
        std::bind(&WebInterface::cb_firmware_meta, this, _1)
    );
    #endif
    #ifdef ENABLE_FEEDER
    server.on("/start_feed",    HTTP_POST, std::bind(&WebInterface::cb_start_feed, this, _1));
    server.on("/abort_feed",    HTTP_GET,  std::bind(&WebInterface::cb_abort_feed, this, _1));
    #endif
    server.on("/",              HTTP_GET,  std::bind(&WebInterface::cb_index, this, _1));
    server.on("/style.css",     HTTP_GET,  std::bind(&WebInterface::cb_css, this, _1));
    server.on("/script.js",     HTTP_GET,  std::bind(&WebInterface::cb_script, this, _1));
    
    // Set up websocket event handler.
    ws.onEvent(std::bind(&WebInterface::cb_websocket_event, this, _1, _2, _3, _4, _5, _6));
    server.addHandler(&ws);
    
    #ifdef OTA_TEST
    md5 = MD5Builder();
    #endif
    return true;
}

bool WebInterface::start()
{
    debug->print("[WebGUI] starting.");
    server.begin();
    last_activity = millis();
    if(ledpin) digitalWrite(ledpin, 1);
    is_running = true;
    return is_running;
}

bool WebInterface::stop()
{
    log_w("[WebGUI] running: %d, connections: %d, idle time: %d", is_running, ws.count(), millis() - last_activity);
    debug->print("[WebGUI] shutting down server.");
    ws.closeAll();
    server.end();
    is_running = false;
    if(ledpin) digitalWrite(ledpin, 0);
    return is_running;
}

void WebInterface::loop()
{

}

bool WebInterface::running()
{
    return is_running;
}

/* bool WebInterface::no_reason_to_live()
{
    return is_running && ws.count() == 0 && ((millis() - last_activity) >= uint32_t(ap_timeout->get()));
} */

// PROPERTY SETTERS
/* void WebInterface::property_ap_timeout(IntegerProperty* p)
{
    ap_timeout = p;
} */

// STREAM INTERFACES
int WebInterface::read()
{
    if(in_n_bytes == 0) return -1;
    int ret = int(in_buffer[in_readptr]);
    in_readptr = (in_readptr + 1) % SIZE;
    in_n_bytes--;
    return ret;
}

int WebInterface::peek() 
{
    if(in_n_bytes == 0) return -1;
    return int(in_buffer[in_readptr]);
}

int WebInterface::available()
{
    return int(in_n_bytes);
}

// PRINT INTERFACES
size_t WebInterface::write(uint8_t b)
{
    return write(&b, 1);
}

size_t WebInterface::write(const uint8_t *buffer, size_t size)
{
    
    if(is_running && ws.count() > 0)
    {
        if(size <= 0) return 0;
        if((size + out_n_bytes) >= SIZE)
        {
            log_e("outbuffer full");
            reset_out_buffer();
            return 0;
        }
        memcpy(out_writeptr, buffer, size);
        uint8_t* newline = (uint8_t*)strchr((const char*)out_writeptr, '\n');
        out_writeptr += size;
        out_n_bytes += size;
        if(newline != nullptr)
        {
            size_t writesize = (newline + 1) - out_buffer;
            ws.textAll(out_buffer, writesize);
            reset_out_buffer();
        }
        return size;
    }
    return 0;
}

// INTERNAL BUFFER
bool WebInterface::write_buffer(const uint8_t* data, uint32_t length)
{
    if(length >= (SIZE - in_n_bytes)) return false;

    bool overflow = length >= (SIZE - in_writeptr);

    uint32_t written = min(length, SIZE - in_writeptr);
    memcpy(&in_buffer[in_writeptr], data, written);
    if(overflow) memcpy(&in_buffer[0], data + written, length - written);

    in_writeptr = (in_writeptr + length) % SIZE;
    in_n_bytes += length;
    return true;
}

void WebInterface::reset_out_buffer()
{
    memset(&out_buffer[0], 0, SIZE);
    out_writeptr = &out_buffer[0];
    out_n_bytes = 0;

}

// CALLBACKS
#ifdef ENABLE_FEEDER
void WebInterface::cb_start_feed(AsyncWebServerRequest* request)
{
    last_activity = millis();
    debug->print("[WebGUI] start_feed");
    int n_params = request->params();
    String pos = "", dur = "";
    for(int i = 0; i < n_params; i++)
    {
        const AsyncWebParameter* param = request->getParam(i);
        // PRINT.print("[WebGUI] POST /start_feed: ", param->name(), ", ", param->value());
        if(param->name() == "position") pos = param->value();
        else if(param->name() == "duration") dur = param->value();
    }
    String command = "[Feeder] feed:" + pos + ',' + dur + '\n';
    debug->print("[WebGUI] command: ", command);
    write_buffer((const uint8_t*)command.c_str(), command.length());
    // PRINT.print("[WebGUI] ", res ? "starting feed" : "ERROR: feed already active.");
    request->send(200, "text/plain", "OK");
}

void WebInterface::cb_abort_feed(AsyncWebServerRequest* request)
{
    last_activity = millis();

    const char* command = "[Feeder] abort\n";
    write_buffer((const uint8_t*)command, strlen(command));
    request->send(200, "text/plain", "OK");
}
#endif

void WebInterface::cb_index(AsyncWebServerRequest* request)
{
    last_activity = millis();
    
    request->send(SPIFFS, "/index.html");
}

void WebInterface::cb_css(AsyncWebServerRequest* request)
{
    last_activity = millis();
    
    request->send(SPIFFS, "/style.css", "text/css");
}

void WebInterface::cb_script(AsyncWebServerRequest* request)
{
    last_activity = millis();
    
    request->send(SPIFFS, "/script.js", "text/javascript");
}

void WebInterface::cb_websocket_event(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len)
{
    last_activity = millis();
    
    if(type == WS_EVT_DATA)
    {
        log_i("[WebGUI] websocket incoming data.");
        // Serial.write(data, len);
        bool success = write_buffer(data, len);
        if(!success) log_e("[WebGUI] ERROR: write to buffer failed.");
    }
    else if(type == WS_EVT_CONNECT)
    {
        reset_out_buffer();
        debug->print("[WebGUI] websocket connected.");
    }
    else if(type == WS_EVT_DISCONNECT)
    {
        debug->print("[WebGUI] websocket disconnected.");
    }
}

#ifdef ENABLE_OTA
void WebInterface::cb_firmware_upload_request(AsyncWebServerRequest* request)
{
    last_activity = millis();
    if (request->getResponse())
    {
        return;
    }
    #ifdef OTA_TEST
    /* size_t n_h = request->headers();
    for(int i = 0; i < n_h; i++)
    {
        const AsyncWebHeader* header = request->getHeader(i);
        Serial.print("[OTA] header: ");
        Serial.print(header->name());
        Serial.print(": ");
        Serial.println(header->value());   
    }
    int n_params = request->params();
    for(int i = 0; i < n_params; i++)
    {
        const AsyncWebParameter* param = request->getParam(i);
        Serial.print("[OTA] param: ");
        Serial.print(param->name());
        Serial.print(": ");
        Serial.println(param->value());
    }
    int n_args = request->args();
    for(int i = 0; i < n_args; i++)
    {
        Serial.print("[OTA] arg: ");
        Serial.print(request->arg(i));
    } */
    
    Serial.println("[OTA] test complete.");
    request->send(200, "text/plain", "OK");
    #else
    if(Update.hasError())   request->send(502, "text/plain", Update.errorString());
    else
    {
        Serial.println("[OTA] firmware received.");
        request->send(200, "text/plain", "OK");
    }
    #endif
}

void WebInterface::cb_firmware_upload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t* data, size_t len, bool final)
{
    last_activity = millis();
    if(!index)
    {
        Serial.println("[OTA] /upload/binary");
    }

    if(len)
    {
        #ifdef OTA_TEST
        md5.add(data, len);
        Serial.print("[OTA] chunk "); Serial.print(index);
        Serial.print(", bytes: "); Serial.println(len);
        Serial.write(data, len);
        Serial.println();
        #else
        bytecounter += len;
        Serial.print("[OTA] chunk "); Serial.print(index);
        Serial.print(", bytes: "); Serial.println(len);
        if(Update.write(data, len) != len)
        {
            Serial.print("[OTA] ERROR: ");
            Serial.println(Update.errorString());
            Update.abort();
            request->abort();
            return;
        }
        #endif
    }
    // Last time this callback is called.
    if(final)
    {
        #ifdef OTA_TEST
        md5.calculate();
        bool check = md5_str == md5.toString();
        Serial.print("[OTA] MD5: "); Serial.println(md5.toString());
        Serial.print("[OTA] MD5 check "); Serial.println(check ? "succeeded." : "failed.");
        
        #else
        Serial.println("[OTA] final chunk"); 
        if(Update.end())
        {
            Serial.println("[OTA] upload succeeded.");
            delay(500);
            ESP.restart();
        }
        else
        {
            Serial.print("[OTA] ERROR: ");
            Serial.println(Update.errorString());
            
        }
        #endif
    }
}

void WebInterface::cb_firmware_meta(AsyncWebServerRequest *request)
{
    size_t size = 0;
    Serial.println("[OTA] /upload/meta");
    #ifdef OTA_TEST_2
    size_t n_h = request->headers();
    for(int i = 0; i < n_h; i++)
    {
        const AsyncWebHeader* header = request->getHeader(i);
        Serial.print("[OTA] header: ");
        Serial.print(header->name());
        Serial.print(": ");
        Serial.println(header->value());   
    }
    int n_params = request->params();
    for(int i = 0; i < n_params; i++)
    {
        const AsyncWebParameter* param = request->getParam(i);
        Serial.print("[OTA] param: ");
        Serial.print(param->name());
        Serial.print(": ");
        Serial.println(param->value());
    }
    int n_args = request->args();
    for(int i = 0; i < n_args; i++)
    {
        Serial.print("[OTA] arg: ");
        Serial.println(request->arg(i));
    }
    #endif

    if(request->hasParam("size", true))
    {
        const AsyncWebParameter* p = request->getParam("size", true);
        int s = p->value().toInt();
        if(s <= 0)
        {
            request->send(400, "text/plain", "Invalid size.");
            return;
        }
        size = size_t(s);
    }
    else
    {
        request->send(400, "text/plain", "Size parameter missing");
        return;
    }
    if(request->hasParam("md5", true))
    {
        const AsyncWebParameter* p = request->getParam("md5", true);
        md5_str = p->value();
    }
    else
    {
        request->send(400, "text/plain", "MD5 parameter missing");
        return;
    }
    Serial.print("[OTA] Binary size: "); Serial.println(size);
    Serial.print("[OTA] MD5:         "); Serial.println(md5_str);
    
    #ifdef OTA_TEST
    md5.begin();
    #else
    if(!Update.begin(size))
    {
        Serial.println("[OTA] ERROR: Not enough memory available");
        request->send(500, "text/plain", "Unable to allocate enough memory to begin update.");
        return;
    }
    else
    {
        if(!Update.setMD5(md5_str.c_str()))
        {
            request->send(400, "text/plain", "MD5 incorrect length");
            return;
        }
    }
    #endif
    bytecounter = 0;
    request->send(200, "text/plain", "OK");
}
#endif

#endif