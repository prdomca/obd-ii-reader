#ifndef OBD_CONNECTION_H
#define OBD_CONNECTION_H

#include <string>

class ObdConnection
{
  private:
    std::string portName;
    int baudRate;
    int fileDescriptor;
    bool demoMode;
    bool demoConnectionOpen;
    std::string lastCommand;

    bool isOpen() const;
    void sendCommand(const std::string &command);
    void readResponse(std::string &response);

  public:
    ObdConnection(const std::string &port, int baud);
    ~ObdConnection();

    void openConnection();
    void closeConnection();
    void initialize();
    void query(const std::string &command, std::string &response);
};

#endif
