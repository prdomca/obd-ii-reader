#include "ObdReader.h"

#include "ObdException.h"
#include "ObdUtils.h"

#include <iostream>

// A lekérdezésekhez tartozó parancsok és a hozzájuk várt válaszfejlécek

const std::string RpmCommand = "010C";
const std::string RpmResponseHeader = "410C";
const std::string CoolantTemperatureCommand = "0105";
const std::string CoolantTemperatureResponseHeader = "4105";

// Konstruktor

ObdPidReader::ObdPidReader(ObdConnection &obdConnection) : connection(obdConnection) {}

// Elküldi a megadott OBD-parancsot, ellenőrzi a válasz fejlécét és hosszát,
// majd kiolvassa és visszaadja a válaszban található adatbájtokat.

std::vector<int> ObdPidReader::readPidBytes(const std::string &command,
                                            const std::string &expectedHeader,
                                            std::size_t dataByteCount)
{
    std::string response;
    connection.query(command, response);

    std::size_t headerPosition = response.find(expectedHeader);
    if (headerPosition == std::string::npos)
    {
        throw ObdProtocolException("Nem a vart OBD valasz erkezett. Parancs: " + command +
                                   ", valasz: " + response);
    }

    std::size_t dataPosition = headerPosition + expectedHeader.size();
    if (response.size() < dataPosition + dataByteCount * 2)
    {
        throw ObdProtocolException("Tul rovid OBD valasz erkezett. Parancs: " + command +
                                   ", valasz: " + response);
    }

    std::vector<int> bytes;
    bytes.reserve(dataByteCount);
    for (std::size_t index = 0; index < dataByteCount; ++index)
    {
        int byte = Utils::hexByte(response, dataPosition + index * 2);
        if (byte == -1)
        {
            throw ObdProtocolException("Hibas hexadecimalis adat erkezett. Parancs: " + command +
                                       ", valasz: " + response);
        }

        bytes.push_back(byte);
    }

    return bytes;
}

// Konstruktor

EngineDataReader::EngineDataReader(ObdConnection &obdConnection) : ObdPidReader(obdConnection) {}

// Kiolvassa, kiszámítja, majd kiírja a motor fordulatszámát.

void EngineDataReader::readRpm()
{
    std::vector<int> bytes = readPidBytes(RpmCommand, RpmResponseHeader, 2);
    int rpm = (bytes[0] * 256 + bytes[1]) / 4;

    std::cout << "Motor fordulatszam: " << rpm << " rpm\n";
}

// Kiolvassa, kiszámítja, majd kiírja a hűtőfolyadék hőmérsékletét.

void EngineDataReader::readCoolantTemperature()
{
    std::vector<int> bytes =
        readPidBytes(CoolantTemperatureCommand, CoolantTemperatureResponseHeader, 1);
    int coolantTemperature = bytes[0] - 40;

    std::cout << "Hutoviz homerseklet: " << coolantTemperature << " C\n";
}
