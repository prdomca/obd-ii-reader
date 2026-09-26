#include "obd/ObdUtils.h"

const std::string HexDigits = "0123456789ABCDEF";

// Removes formatting characters that are not part of the OBD data.
std::string Utils::cleanResponse(const std::string &text)
{
    std::string result;
    for (char character : text)
    {
        if (character != '\r' && character != '\n' && character != ' ' && character != '>')
        {
            result.push_back(character);
        }
    }
    return result;
}

// Converts two hexadecimal characters to a byte value.

int Utils::hexByte(const std::string &text, std::size_t position)
{
    if (position + 2 > text.size())
    {
        return -1;
    }

    std::size_t high = HexDigits.find(text[position]);
    std::size_t low = HexDigits.find(text[position + 1]);

    if (high == std::string::npos || low == std::string::npos)
    {
        return -1;
    }

    return static_cast<int>(high * 16 + low);
}

// Converts a baud rate to the termios constant used to configure the serial port.

speed_t Utils::baudToConstant(int baudRate)
{
    switch (baudRate)
    {
    case SlowBaudRate:
        return B9600;
    case DefaultBaudRate:
        return B38400;
    case FastBaudRate:
        return B115200;
    default:
        return 0;
    }
}

// Returns the supported baud rates as a human-readable string.

std::string Utils::supportedBaudRatesText()
{
    return std::to_string(SlowBaudRate) + ", " + std::to_string(DefaultBaudRate) + ", or " +
           std::to_string(FastBaudRate);
}
