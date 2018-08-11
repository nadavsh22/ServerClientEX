//
// Created by nadavsh22 on 6/13/18.
//

#ifndef EX4_WHATSAPPCLIENT_H
#define EX4_WHATSAPPCLIENT_H
static const int lenOfSuccessfulConnectionMSG = 5;

static const int SERVER_AND_STDIN = 2;

static const int SERVER_EXIT = -2;



#include "Protocol.h"

using std::cin;
using std::cout;
using std::endl;
using std::getline;
using std::string;

// A Class representing a whatsapp client object
class whatsappClient {
private:

    // members

    struct sockaddr_in _csa;

    int _socketFD;

    std::string _clientName;

    bool _connected;

public:

    // methods

    // ctor
    whatsappClient(char *serverIPaddr, short serverPort, const char *clientName);


    int connectToServer();

    //
    int clientLoop();

    int getUserInput(bool &exitFlag);

    int sendCommand(const std::string &command, bool &serverReply);

    int getMessageFromServer();

    int sendWhoCommand(const std::string &command, std::vector<std::string> &names, bool
    &serverReply);

};


#endif //EX4_WHATSAPPCLIENT_H
