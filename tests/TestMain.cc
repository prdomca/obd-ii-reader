#include "TestSuites.h"

int main()
{
    TestRunner tests;
    runObdUtilsTests(tests);
    runObdReaderTests(tests);
    runObdConnectionTests(tests);
    return tests.result();
}
