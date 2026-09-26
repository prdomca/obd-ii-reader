#include "obd/ObdReader.h"

#include "obd/ObdException.h"
#include "obd/ObdUtils.h"

#include <iostream>

// Commands and expected response headers for the supported queries.

const std::string RpmCommand = "010C";
const std::string RpmResponseHeader = "410C";
const std::string CoolantTemperatureCommand = "0105";
const std::string CoolantTemperatureResponseHeader = "4105";

// Constructor.

ObdPidReader::ObdPidReader(ObdConnection &obdConnection) : connection(obdConnection) {}

// Validates a PID response and returns the requested data bytes.

std::vector<int> ObdPidReader::parsePidBytes(const std::string &response,
                                             const std::string &expectedHeader,
                                             std::size_t dataByteCount,
                                             const std::string &command)
{
    std::size_t headerPosition = response.find(expectedHeader);
    if (headerPosition == std::string::npos)
    {
        throw ObdProtocolException("Unexpected OBD response. Command: " + command +
                                   ", response: " + response);
    }

    std::size_t dataPosition = headerPosition + expectedHeader.size();
    if (response.size() < dataPosition + dataByteCount * 2)
    {
        throw ObdProtocolException("OBD response is too short. Command: " + command +
                                   ", response: " + response);
    }

    std::vector<int> bytes;
    bytes.reserve(dataByteCount);
    for (std::size_t index = 0; index < dataByteCount; ++index)
    {
        int byte = Utils::hexByte(response, dataPosition + index * 2);
        if (byte == -1)
        {
            throw ObdProtocolException("Invalid hexadecimal data. Command: " + command +
                                       ", response: " + response);
        }

        bytes.push_back(byte);
    }

    return bytes;
}

// Sends an OBD command and parses the returned data bytes.

std::vector<int> ObdPidReader::readPidBytes(const std::string &command,
                                            const std::string &expectedHeader,
                                            std::size_t dataByteCount)
{
    std::string response;
    connection.query(command, response);
    return parsePidBytes(response, expectedHeader, dataByteCount, command);
}

// Constructor.

EngineDataReader::EngineDataReader(ObdConnection &obdConnection) : ObdPidReader(obdConnection) {}

int EngineDataReader::calculateRpm(int firstByte, int secondByte)
{
    return (firstByte * 256 + secondByte) / 4;
}

int EngineDataReader::calculateCoolantTemperature(int dataByte) { return dataByte - 40; }

// Reads, calculates, and prints the engine speed.

void EngineDataReader::readRpm()
{
    std::vector<int> bytes = readPidBytes(RpmCommand, RpmResponseHeader, 2);
    int rpm = calculateRpm(bytes[0], bytes[1]);

    std::cout << "Engine speed: " << rpm << " rpm\n";
}

// Reads, calculates, and prints the engine coolant temperature.

void EngineDataReader::readCoolantTemperature()
{
    std::vector<int> bytes =
        readPidBytes(CoolantTemperatureCommand, CoolantTemperatureResponseHeader, 1);
    int coolantTemperature = calculateCoolantTemperature(bytes[0]);

    std::cout << "Coolant temperature: " << coolantTemperature << " C\n";
}
