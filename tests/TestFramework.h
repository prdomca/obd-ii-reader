#ifndef OBD_READER_TEST_FRAMEWORK_H
#define OBD_READER_TEST_FRAMEWORK_H

#include <functional>
#include <iostream>
#include <string>

class TestRunner
{
  private:
    int passed = 0;
    int failed = 0;

  public:
    void check(bool condition, const std::string &name)
    {
        if (condition)
        {
            ++passed;
            std::cout << "[PASS] " << name << '\n';
            return;
        }

        ++failed;
        std::cerr << "[FAIL] " << name << '\n';
    }

    template <typename ExceptionType>
    void expectThrows(const std::string &name, const std::function<void()> &operation)
    {
        try
        {
            operation();
            check(false, name);
        }
        catch (const ExceptionType &)
        {
            check(true, name);
        }
        catch (...)
        {
            check(false, name + " (wrong exception type)");
        }
    }

    int result() const
    {
        std::cout << "\n" << passed << " passed, " << failed << " failed\n";
        return failed == 0 ? 0 : 1;
    }
};

#endif // OBD_READER_TEST_FRAMEWORK_H
