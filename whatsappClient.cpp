
#include "whatsappClient.h"


#define NUM_ARGS 4


// ============================
// =                          =
// =      CLASS METHODS       =
// =                          =
// ============================


/**
 * ctor for a new client with clientName
 * @param serverIPaddr server IP address
 * @param serverPort
 * @param clientName
 */
whatsappClient::whatsappClient(char *serverIPaddr, short serverPort, const char *clientName)
{
    _csa.sin_family = AF_INET;
    _csa.sin_port = htons(serverPort);
    memset(&(_csa.sin_zero), '\0', 8);
    // get server by IP:

    struct hostent *hp = gethostbyname(serverIPaddr);
    if (hp == nullptr)
    {
        print_error("gethostbyname", h_errno);
        exit(1);
    }

    memset(&_csa, 0, sizeof(_csa));
    memcpy((char *) &_csa.sin_addr, hp->h_addr, (sa_family_t) hp->h_length);
    _csa.sin_family = (sa_family_t) hp->h_addrtype;
    _csa.sin_port = htons((u_short) serverPort);

    _csa.sin_addr = *((struct in_addr *) hp->h_addr_list[0]);

    _clientName = std::string(clientName);
    _connected = false;

}


//CONNECTOR

// connects to server
// (address, port and such were defined in the constructor)
/**
 * connects to the server
 * @return FAIL/SUCCESS
 */
int whatsappClient::connectToServer()
{
    int sockFD;
    if ((sockFD = socket(_csa.sin_family, SOCK_STREAM, 0)) < 0)
    {
        print_error("socket", errno);
        print_fail_connection();
        return FAIL;
    }

    if (connect(sockFD, (struct sockaddr *) &_csa, sizeof(_csa)) < 0)
    {
        print_error("connect", errno);
        close(sockFD);
        print_fail_connection();
        return FAIL;
    }

    // send client name to server and check that the name isn't already in use
    const char *clientNameBuf = _clientName.c_str();
    send(sockFD, clientNameBuf, WA_MAX_NAME, 0);// write name to server

    char responseBuff[SUCCESS_FAIL_MSG_LEN] = {0};

    read_data(sockFD, responseBuff, SUCCESS_FAIL_MSG_LEN);
    std::string response(responseBuff);

    if (response == "dupsies :(")
    {
        print_dup_connection();
        return FAIL;
    }
    else if (response != "success :)")
    {
        print_fail_connection();
        return FAIL;
    }

    _connected = true;
    print_connection(); // prints connection success message
    _socketFD = sockFD;
    return (sockFD);
}

// ============================
// =                          =
// =        CLIENT LOOP       =
// =                          =
// ============================

/**
 * rund loop of the client as long as he is connected listens to socket and user input
 * @return
 */
int whatsappClient::clientLoop()
{
    if (!_connected)
    { return FAIL; } // not connected to server! shouldn't happen, but it's better
    // safe than sorry

    fd_set readfds, currFDs;

    FD_ZERO(&readfds);
    FD_ZERO(&currFDs);
    FD_SET(_socketFD, &readfds);
    FD_SET(STDIN_FILENO, &readfds);


    while (_connected)
    {
        currFDs = readfds;
        if (select(10, &currFDs, NULL, NULL, NULL) < 0)
        {
            print_error("select", errno); // exit from server
            return FAIL;
        }
        if (FD_ISSET(_socketFD, &currFDs))
        {
            getMessageFromServer();
        }
        if (FD_ISSET(STDIN_FILENO, &currFDs))
        {
            getUserInput(_connected);
        }
    }
    close(_socketFD);
    exit(0);
}

// ============================
// =                          =
// =   GET FROM SERVER/USER   =
// =                          =
// ============================

/**
 * read the buffer
 * @return SUCCESS if SUCCESS,SERVER_EXIT if the server did 'exit'
 */
int whatsappClient::getMessageFromServer()
{
    char incomingMessageBuf[WA_MAX_NAME + WA_MAX_MESSAGE + 2] = {0}; // NAME + ": " + MSG
    auto recievedBytes = read_data(_socketFD, incomingMessageBuf, WA_MAX_NAME + WA_MAX_MESSAGE + 2);
    // receive message from server
    if (recievedBytes <= 0)
    { // supposed to happen if server exits
//        cout << "server closed" << endl;
        print_exit(false, _clientName);
        _connected = false;
        return SERVER_EXIT;
    }

    std::string msg(incomingMessageBuf);
    std::string clientName, message;
    clientName = msg.substr(0, msg.find(':'));
    message = msg.substr(msg.find(':') + 2);
    print_message(clientName, message);

    std::string success = SUCCESS_MSG_HDR + std::to_string(msg.length());
    char successBuf[success.length()] = {0};

    strcpy(successBuf, success.c_str());
    send(_socketFD, successBuf, success.length(), 0); // write success message back to server
    return SUCCESS;
}

/**
 * receives user input and sends appropriate command to server
 * @param exitFlag
 * @return SUCCESS/FAIL
 */
int whatsappClient::getUserInput(bool &exitFlag)
{
    std::string userInput, targetName, message;
    std::getline(std::cin, userInput);
    std::vector<std::string> clients; // this needs to be an actual vector!

    command_type commandType;

    parse_command(userInput, commandType, targetName, message, clients);
    // return input;
    std::vector<std::string> names;

    bool commandSuccess = false;
    bool isClientInGroup = false;
    bool flagCatchError = false;
    switch (commandType)
    {
        case CREATE_GROUP:

            for (std::string &curClient: clients)//check if client included in
            {
                if (_clientName == curClient)
                {
                    if (clients.size() == 1)
                    {
                        print_create_group(false, false, _clientName, targetName);
                        flagCatchError = true;
                    }
                    isClientInGroup = true;
                }
            }
            if (!flagCatchError)
            {
                if (!isClientInGroup)
                { userInput.append(",").append(_clientName); }
                if (!clients.empty())
                {
                    sendCommand(userInput, commandSuccess);
                }
                print_create_group(false, commandSuccess, _clientName, targetName);
            }
            break;
        case SEND:
            if (targetName == _clientName)
            {
                print_send(false, false, _clientName, targetName, message);
                break;
            }
            sendCommand(userInput, commandSuccess);
            print_send(false, commandSuccess, _clientName, targetName, message);
            break;
        case WHO:
            sendWhoCommand(userInput, names, commandSuccess); // "who" needs a specific send command
            print_who_client(commandSuccess, names);
            break;
        case EXIT:
            sendCommand(userInput, commandSuccess);
            print_exit(false, _clientName);
            exitFlag = false;
            break;
        case INVALID:
            print_invalid_input();
            break;
    }
    return SUCCESS;
}

// ============================
// =                          =
// =      SEND COMMANDS       =
// =                          =
// ============================



/**
 * send command to server
 * @param command
 * @param serverReply
 * @return SUCCESS/FAIL
 */
int whatsappClient::sendCommand(const std::string &command, bool &serverReply)
{
    const char *commandBuf = command.c_str();
    send(_socketFD, commandBuf, WA_MAX_INPUT, 0);// send the command to server and
    // wait for it to handle

    char responseBuff[SUCCESS_FAIL_MSG_LEN] = {0};

    read_data(_socketFD, responseBuff, SUCCESS_FAIL_MSG_LEN);
    std::string response(responseBuff);

    // std::cout << "got from server: " << response << endl; //report unsuccessful connection
    if (response == "FAILED")
    {
        serverReply = false;
        return FAIL;
    }
    else if (response == "SUCCESS")
    {
        serverReply = true;
        return (SUCCESS);
    }
    else
    {
        serverReply = false; // unknown reply
        return FAIL;
    }
}


/**
 * sends 'who' command to server
 * @param command
 * @param names
 * @param serverReply
 * @return SUCCESS/FAIL
 */
int whatsappClient::sendWhoCommand(const std::string &command, std::vector<std::string> &names, bool
&serverReply)
{
    char commandBuff[WA_MAX_INPUT] = {0};
    strcpy(commandBuff, command.c_str());
    // const char *commandBuf = command.c_str();
    send(_socketFD, commandBuff, WA_MAX_INPUT, 0);// send the command to
    // server and wait for it to handle

    char responseBuff[WA_MAX_INPUT] = {0};

    read_data(_socketFD, responseBuff, WA_MAX_INPUT);
    std::string response(responseBuff);


    char c[WA_MAX_INPUT];
    char *saveRest = c;
    const char *token;

    strcpy(c, response.c_str());

    while ((token = strtok_r(saveRest, ",", &saveRest)) != NULL)
    {
        names.emplace_back(token);
    }
//    if (token == NULL) {
//        names.emplace_back(saveRest); // only one client connected
//    }

    serverReply = true;
    return SUCCESS;

}




// ============================
// =                          =
// =      MAIN  FUNCTIONS     =
// =                          =
// ============================




/**
 * Checks that arguments passed from commandline are legit, and sets sIP and portNum to
 * matching input values
 * @param argc
 * @param argv
 * @param sIP
 * @param portNum
 */
int checkArgs(int argc, char *argv[], char *&sIP, unsigned short &portNum, char *&clientName)
{
//    std::regex ip_regex(R"(\d+\.\d+\.\d+\.\d+)");
//    std::regex port_regex(R"(^\d{1,10}$)");
    if (argc != NUM_ARGS)
    {
        print_client_usage();
        return FAIL;
    }
    clientName = argv[1]; // todo should check legality???
    string serverAddress = argv[2];
    string serverPort = argv[3];

    char c[20];
    strcpy(c, serverAddress.c_str());

    bool ipMatch = isValidIp(c);
    bool portMatch = isLegalPort(serverPort);
    // port_regex);
    if (!ipMatch)
    {
        print_client_usage();
        return FAIL;
    }
    if (!portMatch)
    {
        print_client_usage();
        return FAIL;
    }
    else
    {
        portNum = (unsigned short) atoi(argv[3]);
        sIP = argv[2];
        return SUCCESS;
    }
}


// creates a new client based on arguments,
// does actions based on user input
int main(int argc, char *argv[])
{
    unsigned short portNum = 0;
    char *serverIP, *clientName;

    if (checkArgs(argc, argv, serverIP, portNum, clientName) == FAIL)
    {
        print_client_usage(); // wrong usage message
        return FAIL;
    }

    whatsappClient wac(serverIP, portNum, clientName);
    if (wac.connectToServer() == FAIL)
    { exit(1); }
    wac.clientLoop();

}




