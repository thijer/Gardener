#ifndef DEBUG_HPP
#define DEBUG_HPP
#include <initializer_list>
#include <vector>
#include "Stream.h"

class DebugStreamer
{
    public:
        DebugStreamer(Stream* p): streamer(p) { }

        template<typename T, typename... Args>
        void print(T t, Args... args)
        {
            streamer->print(t);
            print(args...);
        }

        void print()
        {
            streamer->println();
        }
        int available()
        {
            return streamer->available();
        }
        
        size_t write(const uint8_t* buffer, size_t size)
        {
            return streamer->write(buffer, size);   
        }


    private:
        Stream* streamer;
};

class Debug : public Print
{
    public:
        Debug(std::initializer_list<Stream*> pr)
        {
            streamers.reserve(pr.size());
            for(Stream* p : pr)
            {
                streamers.push_back(DebugStreamer(p));
            }
        }

        template<typename... Args>
        void print(Args... args)
        {
            for(DebugStreamer p : streamers) p.print(args...);
        }

        
        size_t write(const uint8_t* buffer, size_t size)
        {
            size_t written = SIZE_MAX;
            for(DebugStreamer p : streamers) written = min(written, p.write(buffer, size));
            return written;
        }

        size_t write(uint8_t b)
        {
            return write(&b, 1);
        }

    private:
        std::vector<DebugStreamer> streamers;
};

Debug emptydebug({});

/* 
class MultiDebug : public Debug
{
    public:
        MultiDebug(std::initializer_list<Stream*> pr)
        {
            streamers.reserve(pr.size());
            for(Stream* p : pr)
            {
                streamers.push_back(DebugStreamer(p));
            }
        }

        template<typename... Args>
        void print(Args... args)
        {
            for(DebugStreamer p : streamers) p.print(args...);
        }

        
        size_t write(const uint8_t* buffer, size_t size)
        {
            size_t written = SIZE_MAX;
            for(DebugStreamer p : streamers) written = min(written, p.write(buffer, size));
            return written;
        }
        size_t write(uint8_t b) { return write(&b, 1); }
    private:
        std::vector<DebugStreamer> streamers;
};

class SingleDebug : public Debug
{
    public:
        SingleDebug(Stream& s):
            streamer(&s)
        {}

        template<typename... Args>
        void print(Args... args)
        {
            streamer.print(args...);
        }
        size_t write(const uint8_t* buffer, size_t size)
        {
            return streamer.write(buffer, size);
        }
        size_t write(uint8_t b) { return write(&b, 1); }

    private:
        DebugStreamer streamer;
};
*/

#endif