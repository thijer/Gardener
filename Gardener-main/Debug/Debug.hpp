#ifndef DEBUG_HPP
#define DEBUG_HPP
#include <initializer_list>
#include <vector>
#include "Stream.h"

class Debug : public Print
{
    public:
        Debug(std::initializer_list<Stream*> s):
            streamers(s)
        {}

        /// @brief Add a `Stream` interface to print messages to.
        /// @param s The interface
        void add_streamer(Stream* s)
        {
            streamers.push_back(s);
        }

        /// @brief Print a variable number of values/strings to all connected `Stream`ers.
        /// @tparam ...Args Types of the values.
        /// @param ...args The values.
        template<typename... Args>
        void print(Args... args)
        {
            for(Stream* s : streamers) print_to_stream(s, args...);
        }

        /// @brief function used by `print(Args... args)` to print `args` to a `Stream`.
        /// @tparam T Type to be printed
        /// @tparam ...Args remaining types
        /// @param s The `Stream` to print to
        /// @param t The value to print
        /// @param ...args remaining values to print
        template<typename T, typename... Args>
        void print_to_stream(Stream* s, T t, Args... args)
        {
            s->print(t);
            print_to_stream(s, args...);
        }

        /// @brief Terminating function of the recursive variadic function.
        /// @param s The `Stream` to print the terminating character (LF) to.
        void print_to_stream(Stream* s)
        {
            s->print('\n');
        }

        /// @brief Write bytes to all connected `Stream`ers.
        /// @param buffer Pointer to a byte array.
        /// @param size Number of bytes to be written.
        /// @return Number of bytes written. With multiple `Streams`, this returns the lowest value.
        size_t write(const uint8_t* buffer, size_t size)
        {
            size_t written = SIZE_MAX;
            for(Stream* s : streamers) written = min(written, s->write(buffer, size));
            return written;
        }

        /// @brief Write a single byte.
        /// @param b byte
        /// @return Number of bytes written.
        size_t write(uint8_t b)
        {
            return write(&b, 1);
        }

    private:
        std::vector<Stream*> streamers;
};

Debug emptydebug({});

/* 
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