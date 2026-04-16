#ifndef WIFIMANAGER_HPP
#define WIFIMANAGER_HPP
#include "WiFi.h"
#include "../Debug/Debug.hpp"
#include "../config.h"

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
    enum WIFI_STATE {
        ERROR,
        DISCONNECTED,
        CONNECTING,
        CONNECTED,
        RECONNECTING,
        PRE_DISCONNECT,
        DISCONNECTING
    };
    
    void set_state(WIFI_STATE newstate);
    const char* ssid;
    const char* passphrase;
    bool desired_state = true;
    uint32_t last_state_change = 0;
    Debug* debug;


    WIFI_STATE state = DISCONNECTED;
};

WiFiManager::WiFiManager(const char* ssid, const char* passphrase, bool initial_state):
    ssid(ssid),
    passphrase(passphrase),
    desired_state(initial_state)
{}

void WiFiManager::begin(Debug &debugger)
{
    #ifdef ENABLE_WIFI_STA
    WiFi.mode(WIFI_STA);
    #else
    WiFi.mode(WIFI_AP);
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
    WiFi.softAPdisconnect(true);
    #endif
    debug = &debugger;
}

#ifdef ENABLE_WIFI_STA
void WiFiManager::loop()
{
    wl_status_t status = WiFi.status();
    
    if(state == DISCONNECTED)
    {
        if(desired_state)
        {
            set_state(CONNECTING);
        }
    }
    else if(state == CONNECTING || state == RECONNECTING)
    {
        if(status == WL_CONNECTED)
        {
            set_state(CONNECTED);
        }
        else if(!desired_state)
        {
            set_state(DISCONNECTING);
        }
        // else if(status == WL_CONNECT_FAILED && millis() - last_state_change > 5000ul)
        // {
        //     set_state(RECONNECTING);
        // }
        // else if(status == WL_NO_SSID_AVAIL && millis() - last_state_change > 30000ul)
        // {
        //     set_state(RECONNECTING);
        // }
    }
    else if(state == CONNECTED)
    {
        if(desired_state && status != WL_CONNECTED)
        {
            set_state(RECONNECTING);
        }
        else if(!desired_state && status == WL_CONNECTED)
        {
            set_state(PRE_DISCONNECT);
        }
    }
    else if(state == PRE_DISCONNECT)
    {
        if(millis() - last_state_change >= WIFI_DISCONNECT_DELAY)
        {
            set_state(DISCONNECTING);
        }
    }
    else if(state == DISCONNECTING)
    {
        if(status == WL_DISCONNECTED || status == WL_STOPPED)
        {
            set_state(DISCONNECTED);
        }
    }

}

void WiFiManager::set_state(WIFI_STATE newstate)
{
    state = newstate;
    last_state_change = millis();

    if(newstate == CONNECTING)
    {
        debug->print("[WiFiManager] Connecting to WiFi.");
        WiFi.begin(ssid, passphrase);
    }
    else if(newstate == CONNECTED)
    {
        debug->print("[WiFiManager] IP: ", WiFi.localIP());
        debug->print("[WiFiManager] Getting time from NTP.");
        configTime(3600, 0, "europe.pool.ntp.org");
        debug->print("[WiFiManager] Connected.");
    }
    else if(newstate == RECONNECTING)
    {
        debug->print("[WiFiManager] ERROR: Connection failed. Retrying.");
        WiFi.reconnect();
    }
    else if(newstate == PRE_DISCONNECT)
    {
        debug->print("[WiFiManager] Disconnection notice");
    }
    else if(newstate == DISCONNECTING)
    {
        debug->print("[WiFiManager] Disconnecting.");
        WiFi.disconnect(true);
    }
    else if(newstate == DISCONNECTED)
    {
        debug->print("[WiFiManager] Disconnected.");
    }
}
#else

void WiFiManager::loop()
{
    if(state == DISCONNECTED)
    {
        if(desired_state)
        {
            set_state(CONNECTING);
        }
    }
    else if(state == CONNECTING || state == RECONNECTING)
    {
        if(WiFi.AP.started()) set_state(CONNECTED);
    }
    else if(state == CONNECTED)
    {
        if(desired_state && !WiFi.AP.started())
        {
            set_state(RECONNECTING);
        }
        else if(!desired_state && WiFi.AP.started())
        {
            set_state(PRE_DISCONNECT);
        }
    }
    else if(state == PRE_DISCONNECT)
    {
        // Give dependent systems 3 seconds to wrap up before actually disconnecting.
        if(millis() - last_state_change >= WIFI_DISCONNECT_DELAY)
        {
            set_state(DISCONNECTING);
        }
    }
    else if(state == DISCONNECTING)
    {
        // Check AP state.
        if(!WiFi.AP.started())
        {
            set_state(DISCONNECTED);
        }
    }
}

void WiFiManager::set_state(WIFI_STATE newstate)
{
    state = newstate;
    last_state_change = millis();

    if(newstate == CONNECTING || newstate == RECONNECTING)
    {
        debug->print("[WiFiManager] Starting AP with SSID: ", ssid);
        WiFi.softAP(ssid, passphrase);
    }
    else if(newstate == CONNECTED)
    {
        debug->print("[WiFiManager] IP: ", WiFi.softAPIP());
    }
    else if(newstate == PRE_DISCONNECT)
    {
        debug->print("[WiFiManager] Disconnection notice");
    }
    else if(newstate == DISCONNECTING)
    {
        debug->print("[WiFiManager] Disconnecting");
        WiFi.softAPdisconnect(true);
    }
    else if(newstate == DISCONNECTED)
    {
        debug->print("[WiFiManager] Disconnected");
    }
}
#endif

#endif