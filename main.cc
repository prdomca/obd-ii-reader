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

    // Kiírja a megadott szöveget, majd beolvas egy teljes sort a felhasználótól

    bool readLine(const std::string &prompt, std::string &line) const
    {
        std::cout << prompt;
        return static_cast<bool>(std::getline(std::cin, line));
    }

    // Addig kér be egy egész számot, amíg a felhasználó érvényes értéket nem ad meg

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

            std::cerr << "Hibas input.\n";
        }
    }

    // Addig kér be szöveget, amíg a felhasználó érvényes, nem üres választ ad meg

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

            std::cerr << "Hibas input.\n";
        }
    }

  public:
    // Bekéri a kapcsolódási adatokat, inicializálja az adaptert, majd elindítja a menüt

    int run()
    {
        while (true)
        {
            std::string port;
            int baudRate = 0;

            if (!readText("Add meg a soros portot: ", port))
            {
                std::cerr << "A soros port beolvasasa nem sikerult.\n";
                return 1;
            }

            while (true)
            {
                int baudRateChoice = 0;

                std::cout << "Valassz baud rate-et:\n";
                std::cout << "1 - " << Utils::SlowBaudRate << '\n';
                std::cout << "2 - " << Utils::DefaultBaudRate << '\n';
                std::cout << "3 - " << Utils::FastBaudRate << '\n';
                if (!readInteger("Valasztas: ", baudRateChoice))
                {
                    std::cerr << "A baud rate beolvasasa nem sikerult.\n";
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
                    std::cerr << "Ervenytelen baud rate valasztas.\n";
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

                    std::cout << "\nMit szeretnel lekerdezni?\n";
                    std::cout << "1 - Homerseklet\n";
                    std::cout << "2 - Fordulatszam gyorsan\n";
                    std::cout << "3 - Kilepes\n";
                    if (!readInteger("Valasztas: ", choice))
                    {
                        std::cerr << "Ervenytelen valasztas.\n";
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
                                << "Hutoviz homerseklet lekerdezesi hiba: " << exception.what()
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
                                std::cerr << "RPM lekerdezesi hiba: " << exception.what() << '\n';
                                break;
                            }
                        }
                        break;
                    case 3:
                        return 0;
                    default:
                        std::cerr << "Ervenytelen valasztas.\n";
                        break;
                    }
                }
            }
            catch (const ObdException &exception)
            {
                std::cerr << "Kapcsolodasi vagy inicializalasi hiba: " << exception.what() << '\n';

                bool retryConnection = false;
                while (true)
                {
                    std::string answer;

                    if (!readText("Megadsz masik soros portot? (i/n): ", answer))
                    {
                        return 1;
                    }

                    if (answer == "i" || answer == "I")
                    {
                        retryConnection = true;
                        break;
                    }

                    if (answer == "n" || answer == "N")
                    {
                        break;
                    }

                    std::cerr << "Hibas input.\n";
                }

                if (!retryConnection)
                {
                    return 1;
                }
            }
        }
    }
};

// Elindítja a programot, és kezeli az esetleges váratlan hibákat

int main()
{
    try
    {
        ObdProgram program;
        return program.run();
    }
    catch (const std::exception &exception)
    {
        std::cerr << "Varatlan hiba: " << exception.what() << '\n';
        return 1;
    }
}
