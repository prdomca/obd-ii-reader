#ifndef OBD_UTILS_H
#define OBD_UTILS_H

#include <string>
#include <termios.h>

class Utils
{
  public:
    // Az ELM327 adaptereknél ezek a gyakori soros sebességek;
    // a 38400 a legtöbb adapter gyári alapértelmezése.
    static constexpr int SlowBaudRate = 9600;
    static constexpr int DefaultBaudRate = 38400;
    static constexpr int FastBaudRate = 115200;

    static std::string cleanResponse(const std::string &text);
    static int hexByte(const std::string &text, std::size_t position);
    static speed_t baudToConstant(int baudRate);
    static std::string supportedBaudRatesText();
};

#endif
