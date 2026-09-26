#ifndef OBD_READER_H
#define OBD_READER_H

#include "ObdConnection.h"

#include <string>
#include <vector>

class ObdPidReader
{
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

    void readRpm();
    void readCoolantTemperature();
};

#endif
