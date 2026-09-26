#include "TestSuites.h"

#include "obd/ObdException.h"
#include "obd/ObdReader.h"

#include <vector>

namespace
{
void testPidParsing(TestRunner &tests)
{
    const std::vector<int> rpmBytes =
        ObdPidReader::parsePidBytes("010C410C1AF8", "410C", 2, "010C");
    tests.check(rpmBytes == std::vector<int>({0x1A, 0xF8}),
                "parsePidBytes finds and parses RPM data");

    tests.expectThrows<ObdProtocolException>("parsePidBytes rejects an unexpected header", [] {
        ObdPidReader::parsePidBytes("41055A", "410C", 2, "010C");
    });
    tests.expectThrows<ObdProtocolException>("parsePidBytes rejects a short response", [] {
        ObdPidReader::parsePidBytes("410C1A", "410C", 2, "010C");
    });
    tests.expectThrows<ObdProtocolException>("parsePidBytes rejects invalid hexadecimal data", [] {
        ObdPidReader::parsePidBytes("410C1AG8", "410C", 2, "010C");
    });
}

void testConversions(TestRunner &tests)
{
    tests.check(EngineDataReader::calculateCoolantTemperature(0) == -40,
                "coolant conversion handles the minimum byte");
    tests.check(EngineDataReader::calculateCoolantTemperature(90) == 50,
                "coolant conversion matches the demo response");
    tests.check(EngineDataReader::calculateCoolantTemperature(255) == 215,
                "coolant conversion handles the maximum byte");

    tests.check(EngineDataReader::calculateRpm(0, 0) == 0, "RPM conversion handles zero");
    tests.check(EngineDataReader::calculateRpm(0x1A, 0xF8) == 1726,
                "RPM conversion matches the demo response");
    tests.check(EngineDataReader::calculateRpm(0xFF, 0xFF) == 16383,
                "RPM conversion truncates the maximum fractional result");
}
} // namespace

void runObdReaderTests(TestRunner &tests)
{
    testPidParsing(tests);
    testConversions(tests);
}
