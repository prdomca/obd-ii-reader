#include "ObdConnection.h"

#include "ObdException.h"
#include "ObdUtils.h"

#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <termios.h>
#include <unistd.h>

// Konstruktor

ObdConnection::ObdConnection(const std::string &port, int baud)
    : portName(port), baudRate(baud), fileDescriptor(-1),
      demoMode(port == "demo.txt" ||
               (port.size() >= 9 && port.compare(port.size() - 9, 9, "/demo.txt") == 0)),
      demoConnectionOpen(false)
{
}

// Destruktor

ObdConnection::~ObdConnection() { closeConnection(); }

// Megnyitja a soros portot, majd beállítja a kommunikációhoz szükséges paramétereket:
// a sebességet, a 8N1-es adatkeretet, a nyers adatkezelést és az olvasási időkorlátot.

void ObdConnection::openConnection()
{
    if (demoMode)
    {
        closeConnection();

        std::ifstream demoFile(portName);
        if (!demoFile)
        {
            throw ObdConnectionException("A demo fajlt nem lehetett megnyitni: " + portName);
        }

        demoConnectionOpen = true;
        return;
    }

    speed_t speed = Utils::baudToConstant(baudRate);
    if (speed == 0)
    {
        throw ObdConnectionException("A baud rate csak " + Utils::supportedBaudRatesText() +
                                     " lehet.");
    }

    closeConnection();

    fileDescriptor = open(portName.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
    if (fileDescriptor < 0)
    {
        throw ObdConnectionException("A soros portot nem lehetett megnyitni: " + portName);
    }

    termios tty{};
    if (tcgetattr(fileDescriptor, &tty) != 0)
    {
        closeConnection();
        throw ObdConnectionException("A soros port beallitasai nem olvashatok ki.");
    }

    if (cfsetispeed(&tty, speed) != 0 || cfsetospeed(&tty, speed) != 0)
    {
        closeConnection();
        throw ObdConnectionException("A soros port sebesseget nem lehetett beallitani.");
    }

    tty.c_cflag = static_cast<tcflag_t>((tty.c_cflag & static_cast<tcflag_t>(~CSIZE)) | CS8);
    tty.c_cflag |= static_cast<tcflag_t>(CLOCAL | CREAD);
    tty.c_cflag &= static_cast<tcflag_t>(~static_cast<tcflag_t>(PARENB | CSTOPB | CRTSCTS));
    tty.c_iflag = 0;
    tty.c_oflag = 0;
    tty.c_lflag = 0;
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 10;

    tcflush(fileDescriptor, TCIFLUSH);
    if (tcsetattr(fileDescriptor, TCSANOW, &tty) != 0)
    {
        closeConnection();
        throw ObdConnectionException("A soros port beallitasait nem lehetett alkalmazni.");
    }
}

// Lezárja a soros portot

void ObdConnection::closeConnection()
{
    demoConnectionOpen = false;
    lastCommand.clear();

    if (fileDescriptor >= 0)
    {
        close(fileDescriptor);
        fileDescriptor = -1;
    }
}

// Inicializálja az OBD-adaptert a szükséges AT parancsok elküldésével

void ObdConnection::initialize()
{
    if (demoMode)
    {
        return;
    }

    const std::string commands[] = {"ATZ", "ATE0", "ATL0", "ATS0", "ATH0", "ATSP0"};

    for (const std::string &command : commands)
    {
        std::string response;
        try
        {
            query(command, response);
        }
        catch (const ObdException &exception)
        {
            throw ObdConnectionException("Inicializalasi hiba a kovetkezo parancsnal: " + command +
                                         ". " + exception.what());
        }

        if (command != "ATZ" && response.find("OK") == std::string::npos)
        {
            throw ObdProtocolException("Az adapter nem vart valaszt adott: " + response);
        }
    }
}

// A tényleges kommunikációért felelős függvény, küld egy parancsot, majd kis várakozás után
// kiolvassa a választ

void ObdConnection::query(const std::string &command, std::string &response)
{
    response.clear();

    if (!isOpen())
    {
        throw ObdConnectionException("Nincs megnyitott kapcsolat.");
    }

    sendCommand(command);

    usleep(10000);

    readResponse(response);

    if (response.find("NODATA") != std::string::npos ||
        response.find("ERROR") != std::string::npos || response == "?")
    {
        throw ObdProtocolException("Az adapter ervenytelen valaszt adott: " + response);
    }
}

// Ellenőrzi, hogy meg van-e nyitva a soros port

bool ObdConnection::isOpen() const
{
    if (demoMode)
    {
        return demoConnectionOpen;
    }

    return fileDescriptor >= 0;
}

// Kiküldi a kívánt parancsot

void ObdConnection::sendCommand(const std::string &command)
{
    lastCommand = command;

    if (demoMode)
    {
        return;
    }

    std::string line = command + '\r';
    std::size_t totalWritten = 0;

    while (totalWritten < line.size())
    {
        ssize_t written =
            write(fileDescriptor, line.c_str() + totalWritten, line.size() - totalWritten);

        if (written < 0)
        {
            throw ObdConnectionException("A parancs kuldese nem sikerult: " + command);
        }

        if (written == 0)
        {
            throw ObdConnectionException("A parancs kuldese megszakadt: " + command);
        }

        totalWritten += static_cast<std::size_t>(written);
    }
}

// Addig olvassa a választ, amíg meg nem érkezik a lezáró '>' karakter,
// vagy az adapter már nem küld több adatot.

void ObdConnection::readResponse(std::string &response)
{
    response.clear();
    std::string result;

    if (demoMode)
    {

        std::ifstream demoFile(portName);
        if (!demoFile)
        {
            throw ObdConnectionException("A demo fajlt nem lehetett megnyitni: " + portName);
        }

        std::string line;
        while (std::getline(demoFile, line))
        {
            std::size_t separator = line.find('=');
            if (separator == std::string::npos)
            {
                continue;
            }

            std::string command = Utils::cleanResponse(line.substr(0, separator));
            if (command == lastCommand)
            {
                result = line.substr(separator + 1);
                break;
            }
        }

        if (result.empty())
        {
            throw ObdProtocolException("A demo fajlban nincs valasz erre a parancsra: " +
                                       lastCommand);
        }

        response = Utils::cleanResponse(result);
        if (response.empty())
        {
            throw ObdProtocolException("Nem jott valasz az adaptertol.");
        }

        return;
    }

    char buffer[256];

    while (true)
    {
        ssize_t count = read(fileDescriptor, buffer, sizeof(buffer) - 1);
        if (count < 0)
        {
            throw ObdConnectionException("A valasz olvasasa nem sikerult.");
        }

        if (count == 0)
        {
            break;
        }

        buffer[count] = '\0';
        result += buffer;

        if (result.size() > 1000)
        {
            throw ObdProtocolException("Tul hosszu valasz erkezett az adaptertol.");
        }

        if (result.find('>') != std::string::npos)
        {
            break;
        }
    }

    response = Utils::cleanResponse(result);
    if (response.empty())
    {
        throw ObdProtocolException("Nem jott valasz az adaptertol.");
    }
}
