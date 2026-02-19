#ifndef WIFIMANAGER_HPP
#define WIFIMANAGER_HPP
#include "WiFi.h"
#include "debug.hpp"

class WiFiManager
{
    public:
    WiFiManager(const char* ssid, const char* passphrase, bool initial_state = true);
    ~WiFiManager(){}
    
    void begin(Debug& debugger = emptydebug);
    void loop();
    bool connected() { return state == CONNECTED; }
    void enable()    { desired_state = true; }
    void disable()   { desired_state = false; }
    
    protected:
    
    private:
    const char* ssid;
    const char* passphrase;
    bool desired_state = true;
    Debug* debug;

    enum WIFI_STATE {
        ERROR,
        DISCONNECTED,
        CONNECTING,
        CONNECTED,
        DISCONNECTING
    } state = DISCONNECTED;
};

WiFiManager::WiFiManager(const char* ssid, const char* passphrase, bool initial_state):
    ssid(ssid),
    passphrase(passphrase),
    desired_state(initial_state)
{}

void WiFiManager::begin(Debug &debugger)
{
    debug = &debugger;
}

void WiFiManager::loop()
{
    wl_status_t status = WiFi.status();
    
    if(state == DISCONNECTED)
    {
        if(desired_state)
        {
            debug->print("[WiFiManager] Connecting to WiFi.");
            WiFi.mode(WIFI_STA);
            WiFi.begin(ssid, passphrase);
            state = CONNECTING;
        }
    }
    else if(state == CONNECTING)
    {
        if(status == WL_CONNECTED)
        {
            debug->print("[WiFiManager] IP: ", WiFi.localIP());
            debug->print("[WiFiManager] Getting time from NTP.");
            configTime(3600, 0, "europe.pool.ntp.org");
            state = CONNECTED;
        }
    }
    else if(state == CONNECTED)
    {
        if(desired_state && status != WL_CONNECTED)
        {
            debug->print("[WiFiManager] Connecting to WiFi.");
            WiFi.mode(WIFI_STA);
            WiFi.begin(ssid, passphrase);
            state = CONNECTING;
        }
        else if(!desired_state && status == WL_CONNECTED)
        {
            // Disconnect WiFi.
            debug->print("[WiFiManager] Disconnecting.");
            WiFi.disconnect(true);
            state = DISCONNECTING;
        }
    }
    else if(state == DISCONNECTING)
    {
        if(status == WL_DISCONNECTED || status == WL_STOPPED)
        {
            debug->print("[WiFiManager] Disconnected.");
            state = DISCONNECTED;
        }
    }

}

#endif