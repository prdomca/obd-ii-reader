#ifndef OBD_EXCEPTION_H
#define OBD_EXCEPTION_H

#include <stdexcept>
#include <string>

// Exception types for OBD connection and protocol errors.

class ObdException : public std::runtime_error
{
  public:
    ObdException(const std::string &message) : std::runtime_error(message) {}
};

class ObdConnectionException : public ObdException
{
  public:
    ObdConnectionException(const std::string &message) : ObdException(message) {}
};

class ObdProtocolException : public ObdException
{
  public:
    ObdProtocolException(const std::string &message) : ObdException(message) {}
};

#endif
