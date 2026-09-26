#include "TestSuites.h"

#include "obd/ObdConnection.h"
#include "obd/ObdException.h"
#include "obd/ObdUtils.h"

#include <string>

void runObdConnectionTests(TestRunner &tests)
{
    ObdConnection closedConnection("data/demo.txt", Utils::DefaultBaudRate);
    tests.expectThrows<ObdConnectionException>("query rejects a closed connection", [&] {
        std::string response;
        closedConnection.query("0105", response);
    });

    ObdConnection connection("data/demo.txt", Utils::DefaultBaudRate);
    connection.openConnection();
    connection.initialize();

    std::string response;
    connection.query("0105", response);
    tests.check(response == "41055A", "demo connection returns coolant data");
    connection.query("010C", response);
    tests.check(response == "410C1AF8", "demo connection returns RPM data");
    tests.expectThrows<ObdProtocolException>("demo connection rejects a missing command", [&] {
        connection.query("0100", response);
    });

    ObdConnection missingFile("tests/missing/demo.txt", Utils::DefaultBaudRate);
    tests.expectThrows<ObdConnectionException>("demo connection rejects a missing file", [&] {
        missingFile.openConnection();
    });
}
