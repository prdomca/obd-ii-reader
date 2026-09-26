#ifndef OBD_UTILS_H
#define OBD_UTILS_H

#include <string>
#include <termios.h>

class Utils
{
  public:
    // These are common serial speeds for ELM327 adapters; 38400 is the factory
    // default for many devices.
    static constexpr int SlowBaudRate = 9600;
    static constexpr int DefaultBaudRate = 38400;
    static constexpr int FastBaudRate = 115200;

    static std::string cleanResponse(const std::string &text);
    static int hexByte(const std::string &text, std::size_t position);
    static speed_t baudToConstant(int baudRate);
    static std::string supportedBaudRatesText();
};

#endif
