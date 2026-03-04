#ifndef DEBUGWEBSOCKET_HPP
#define DEBUGWEBSOCKET_HPP
#include "NetworkServer.h"
#include "NetworkClient.h"

class DebugWebsocket: public Stream
{
    public:
    DebugWebsocket(uint32_t port);
    ~DebugWebsocket(){}
    void begin();
    void loop();

    // Stream interfaces
    int read();
    int peek();
    int available();

    // Print interface
    size_t write(uint8_t b);
    size_t write(const uint8_t *buffer, size_t size);
    
    protected:
    
    private:
        NetworkClient client;
        NetworkServer server;
        uint32_t last_update = 0;
};

DebugWebsocket::DebugWebsocket(uint32_t port):
    server(port, 1)
{}

void DebugWebsocket::begin()
{
    server.begin();
}

void DebugWebsocket::loop()
{
    if(millis() - last_update >= 100ul)
    {
        NetworkClient cl = server.accept();

        if(cl.connected())
        {
            // No active connection yet
            if(!client.connected())
            {
                Serial.println("New client accepted.");
                client = cl;
            }
            // There already is a client connected. Close new connection.
            else 
            {
                Serial.println("New client refused.");
                cl.stop();
            }
        }
    }
}

int DebugWebsocket::read()
{
    return client.read();
}

int DebugWebsocket::peek()
{
    return client.peek();
}

int DebugWebsocket::available()
{
    return client.available();
}

size_t DebugWebsocket::write(uint8_t b)
{
    return client.write(b);
}

size_t DebugWebsocket::write(const uint8_t *buffer, size_t size)
{
    return client.write(buffer, size);
}

#endif