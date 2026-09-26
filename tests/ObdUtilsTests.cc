#include "TestSuites.h"

#include "obd/ObdUtils.h"

namespace
{
void testResponseCleanup(TestRunner &tests)
{
    tests.check(Utils::cleanResponse("\r\n 41 05 5A >") == "41055A",
                "cleanResponse removes ELM327 formatting");
    tests.check(Utils::cleanResponse("NO DATA\r>") == "NODATA",
                "cleanResponse preserves response text");
}

void testHexadecimalParsing(TestRunner &tests)
{
    tests.check(Utils::hexByte("00", 0) == 0, "hexByte parses zero");
    tests.check(Utils::hexByte("5A", 0) == 90, "hexByte parses a regular byte");
    tests.check(Utils::hexByte("00FF", 2) == 255, "hexByte respects the byte position");
    tests.check(Utils::hexByte("5a", 0) == -1, "hexByte rejects lowercase hexadecimal");
    tests.check(Utils::hexByte("G0", 0) == -1, "hexByte rejects invalid hexadecimal");
    tests.check(Utils::hexByte("F", 0) == -1, "hexByte rejects truncated data");
    tests.check(Utils::hexByte("FF", 2) == -1, "hexByte rejects an out-of-range position");
}

void testBaudRates(TestRunner &tests)
{
    tests.check(Utils::baudToConstant(Utils::SlowBaudRate) == B9600,
                "baudToConstant maps 9600 baud");
    tests.check(Utils::baudToConstant(Utils::DefaultBaudRate) == B38400,
                "baudToConstant maps 38400 baud");
    tests.check(Utils::baudToConstant(Utils::FastBaudRate) == B115200,
                "baudToConstant maps 115200 baud");
    tests.check(Utils::baudToConstant(12345) == 0, "baudToConstant rejects unsupported rates");
    tests.check(Utils::supportedBaudRatesText() == "9600, 38400, or 115200",
                "supportedBaudRatesText lists every supported rate");
}
} // namespace

void runObdUtilsTests(TestRunner &tests)
{
    testResponseCleanup(tests);
    testHexadecimalParsing(tests);
    testBaudRates(tests);
}
