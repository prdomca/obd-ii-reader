#include "ObdConnection.h"

#include "ObdException.h"
#include "ObdUtils.h"

#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <termios.h>
#include <unistd.h>

// Constructor.

ObdConnection::ObdConnection(const std::string &port, int baud)
    : portName(port), baudRate(baud), fileDescriptor(-1),
      demoMode(port == "demo.txt" ||
               (port.size() >= 9 && port.compare(port.size() - 9, 9, "/demo.txt") == 0)),
      demoConnectionOpen(false)
{
}

// Destructor.

ObdConnection::~ObdConnection() { closeConnection(); }

// Opens the serial port and configures its baud rate, 8N1 framing, raw I/O,
// and read timeout.

void ObdConnection::openConnection()
{
    if (demoMode)
    {
        closeConnection();

        std::ifstream demoFile(portName);
        if (!demoFile)
        {
            throw ObdConnectionException("Could not open the demo file: " + portName);
        }

        demoConnectionOpen = true;
        return;
    }

    speed_t speed = Utils::baudToConstant(baudRate);
    if (speed == 0)
    {
        throw ObdConnectionException("The baud rate must be one of the following: " +
                                     Utils::supportedBaudRatesText() + ".");
    }

    closeConnection();

    fileDescriptor = open(portName.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
    if (fileDescriptor < 0)
    {
        throw ObdConnectionException("Could not open the serial port: " + portName);
    }

    termios tty{};
    if (tcgetattr(fileDescriptor, &tty) != 0)
    {
        closeConnection();
        throw ObdConnectionException("Could not read the serial-port settings.");
    }

    if (cfsetispeed(&tty, speed) != 0 || cfsetospeed(&tty, speed) != 0)
    {
        closeConnection();
        throw ObdConnectionException("Could not set the serial-port baud rate.");
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
        throw ObdConnectionException("Could not apply the serial-port settings.");
    }
}

// Closes the serial port.

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

// Initializes the OBD adapter by sending the required AT commands.

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
            throw ObdConnectionException("Initialization failed while sending command " + command +
                                         ": " + exception.what());
        }

        if (command != "ATZ" && response.find("OK") == std::string::npos)
        {
            throw ObdProtocolException("The adapter returned an unexpected response: " + response);
        }
    }
}

// Sends a command, waits briefly, and reads the adapter's response.

void ObdConnection::query(const std::string &command, std::string &response)
{
    response.clear();

    if (!isOpen())
    {
        throw ObdConnectionException("The connection is not open.");
    }

    sendCommand(command);

    usleep(10000);

    readResponse(response);

    if (response.find("NODATA") != std::string::npos ||
        response.find("ERROR") != std::string::npos || response == "?")
    {
        throw ObdProtocolException("The adapter returned an invalid response: " + response);
    }
}

// Returns whether the serial or demo connection is open.

bool ObdConnection::isOpen() const
{
    if (demoMode)
    {
        return demoConnectionOpen;
    }

    return fileDescriptor >= 0;
}

// Sends a command to the adapter.

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
            throw ObdConnectionException("Failed to send command: " + command);
        }

        if (written == 0)
        {
            throw ObdConnectionException("Command transmission was interrupted: " + command);
        }

        totalWritten += static_cast<std::size_t>(written);
    }
}

// Reads until the terminating '>' prompt arrives or the adapter stops sending data.

void ObdConnection::readResponse(std::string &response)
{
    response.clear();
    std::string result;

    if (demoMode)
    {

        std::ifstream demoFile(portName);
        if (!demoFile)
        {
            throw ObdConnectionException("Could not open the demo file: " + portName);
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
            throw ObdProtocolException("The demo file has no response for command: " + lastCommand);
        }

        response = Utils::cleanResponse(result);
        if (response.empty())
        {
            throw ObdProtocolException("The adapter returned no response.");
        }

        return;
    }

    char buffer[256];

    while (true)
    {
        ssize_t count = read(fileDescriptor, buffer, sizeof(buffer) - 1);
        if (count < 0)
        {
            throw ObdConnectionException("Failed to read the response.");
        }

        if (count == 0)
        {
            break;
        }

        buffer[count] = '\0';
        result += buffer;

        if (result.size() > 1000)
        {
            throw ObdProtocolException("The adapter response is too long.");
        }

        if (result.find('>') != std::string::npos)
        {
            break;
        }
    }

    response = Utils::cleanResponse(result);
    if (response.empty())
    {
        throw ObdProtocolException("The adapter returned no response.");
    }
}
