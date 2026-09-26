#ifndef OBD_READER_OBD_READER_H
#define OBD_READER_OBD_READER_H

#include "obd/ObdConnection.h"

#include <string>
#include <vector>

class ObdPidReader
{
  public:
    virtual ~ObdPidReader() = default;

    static std::vector<int> parsePidBytes(const std::string &response,
                                          const std::string &expectedHeader,
                                          std::size_t dataByteCount, const std::string &command);

  protected:
    ObdConnection &connection;

    explicit ObdPidReader(ObdConnection &obdConnection);
    std::vector<int> readPidBytes(const std::string &command, const std::string &expectedHeader,
                                  std::size_t dataByteCount);
};

class EngineDataReader : public ObdPidReader
{
  public:
    explicit EngineDataReader(ObdConnection &obdConnection);

    static int calculateRpm(int firstByte, int secondByte);
    static int calculateCoolantTemperature(int dataByte);

    void readRpm();
    void readCoolantTemperature();
};

#endif // OBD_READER_OBD_READER_H
