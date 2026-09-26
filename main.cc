#include "ObdConnection.h"
#include "ObdException.h"
#include "ObdReader.h"
#include "ObdUtils.h"

#include <exception>
#include <iostream>
#include <sstream>
#include <string>

class ObdProgram
{
  private:
    static const int RpmReadCount = 50;

    // Prints the prompt and reads a complete line from standard input.

    bool readLine(const std::string &prompt, std::string &line) const
    {
        std::cout << prompt;
        return static_cast<bool>(std::getline(std::cin, line));
    }

    // Keeps prompting until the user enters a valid integer.

    bool readInteger(const std::string &prompt, int &value) const
    {
        while (true)
        {
            std::string line;
            if (!readLine(prompt, line))
            {
                return false;
            }

            std::istringstream input(line);
            char extraCharacter;
            if (input >> value && !(input >> extraCharacter))
            {
                return true;
            }

            std::cerr << "Invalid input.\n";
        }
    }

    // Keeps prompting until the user enters a non-empty value.

    bool readText(const std::string &prompt, std::string &value) const
    {
        while (true)
        {
            if (!readLine(prompt, value))
            {
                return false;
            }

            if (!value.empty())
            {
                return true;
            }

            std::cerr << "Invalid input.\n";
        }
    }

  public:
    // Reads the connection settings, initializes the adapter, and starts the menu.

    int run()
    {
        while (true)
        {
            std::string port;
            int baudRate = 0;

            if (!readText("Enter the serial port: ", port))
            {
                std::cerr << "Failed to read the serial port.\n";
                return 1;
            }

            while (true)
            {
                int baudRateChoice = 0;

                std::cout << "Select a baud rate:\n";
                std::cout << "1 - " << Utils::SlowBaudRate << '\n';
                std::cout << "2 - " << Utils::DefaultBaudRate << '\n';
                std::cout << "3 - " << Utils::FastBaudRate << '\n';
                if (!readInteger("Choice: ", baudRateChoice))
                {
                    std::cerr << "Failed to read the baud rate.\n";
                    return 1;
                }

                switch (baudRateChoice)
                {
                case 1:
                    baudRate = Utils::SlowBaudRate;
                    break;
                case 2:
                    baudRate = Utils::DefaultBaudRate;
                    break;
                case 3:
                    baudRate = Utils::FastBaudRate;
                    break;
                default:
                    std::cerr << "Invalid baud-rate choice.\n";
                    break;
                }

                if (baudRate != 0)
                {
                    break;
                }
            }

            try
            {
                ObdConnection connection(port, baudRate);
                connection.openConnection();
                connection.initialize();

                EngineDataReader engineDataReader(connection);

                while (true)
                {
                    int choice = 0;

                    std::cout << "\nWhat would you like to read?\n";
                    std::cout << "1 - Coolant temperature\n";
                    std::cout << "2 - Engine speed (rapid sampling)\n";
                    std::cout << "3 - Exit\n";
                    if (!readInteger("Choice: ", choice))
                    {
                        std::cerr << "Invalid choice.\n";
                        return 1;
                    }

                    switch (choice)
                    {
                    case 1:
                        try
                        {
                            engineDataReader.readCoolantTemperature();
                        }
                        catch (const ObdException &exception)
                        {
                            std::cerr
                                << "Failed to read coolant temperature: " << exception.what()
                                << '\n';
                        }
                        break;
                    case 2:
                        for (int readIndex = 0; readIndex < RpmReadCount; ++readIndex)
                        {
                            try
                            {
                                engineDataReader.readRpm();
                            }
                            catch (const ObdException &exception)
                            {
                                std::cerr << "Failed to read engine speed: " << exception.what()
                                          << '\n';
                                break;
                            }
                        }
                        break;
                    case 3:
                        return 0;
                    default:
                        std::cerr << "Invalid choice.\n";
                        break;
                    }
                }
            }
            catch (const ObdException &exception)
            {
                std::cerr << "Connection or initialization error: " << exception.what() << '\n';

                bool retryConnection = false;
                while (true)
                {
                    std::string answer;

                    if (!readText("Try another serial port? (y/n): ", answer))
                    {
                        return 1;
                    }

                    if (answer == "y" || answer == "Y")
                    {
                        retryConnection = true;
                        break;
                    }

                    if (answer == "n" || answer == "N")
                    {
                        break;
                    }

                    std::cerr << "Invalid input.\n";
                }

                if (!retryConnection)
                {
                    return 1;
                }
            }
        }
    }
};

// Starts the application and handles unexpected errors.

int main()
{
    try
    {
        ObdProgram program;
        return program.run();
    }
    catch (const std::exception &exception)
    {
        std::cerr << "Unexpected error: " << exception.what() << '\n';
        return 1;
    }
}
