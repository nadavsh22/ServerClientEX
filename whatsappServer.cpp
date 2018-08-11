

#include "whatsappServer.h"

whatsappServer::whatsappServer(unsigned short servPortnum) : _servPortnum(servPortnum)
{
    _servSocketFD = -1; // default
}

// ============================
// =                          =
// =        CONNECTORS        =
// =                          =
// ============================


// create socket in server - bind and listen
/**
 * initialize socket, bind and listen
 * @return  SUCCESS/FAIL
 */
int whatsappServer::initSocket()
{
    char myname[MAXHOSTNAME + 1];

    struct hostent *hp;

    // hostnet initialization
    gethostname(myname, MAXHOSTNAME);
    hp = gethostbyname(myname);
    if (hp == NULL)
    {
        return FAIL;
    }
    //sockaddrr_in initlization
    memset(&_serverSocket, 0, sizeof(struct sockaddr_in));
    _serverSocket.sin_family = (sa_family_t) hp->h_addrtype;
    /* this is our host address */
    // memcpy(&_serverSocket.sin_addr, hp->h_addr, (size_t) hp->h_length);
    _serverSocket.sin_addr.s_addr=htonl(INADDR_ANY);
    /* this is our port number */
    _serverSocket.sin_port = htons(_servPortnum);

    
    /* create socket */
    if ((_servSocketFD = socket(_serverSocket.sin_family, SOCK_STREAM, 0)) < 0)
    {
        print_error("socket", errno);
        return (FAIL);
    }

    // Should allow us to use the socket again even if we crashed the program without closing the
    // socket.
    int reuse = 1;
    if (setsockopt(_servSocketFD, SOL_SOCKET, SO_REUSEADDR, (const char *) &reuse, sizeof(reuse)) <
        0)
    { perror("setsockopt(SO_REUSEADDR) failed"); }

#ifdef SO_REUSEPORT
    if (setsockopt(_servSocketFD, SOL_SOCKET, SO_REUSEPORT, (const char *) &reuse, sizeof(reuse)) <
        0)
    { perror("setsockopt(SO_REUSEPORT) failed"); }
#endif
    // end of reuse code

    if (bind(_servSocketFD, (struct sockaddr *) &_serverSocket, sizeof(struct sockaddr_in)) < 0)
    {
        print_error("bind", errno);
        close(_servSocketFD);
        return (FAIL);
    }

    if (listen(_servSocketFD, MAX_NUM_PENDING) < 0)
    { /* max # of queued connects */
        print_error("listen", errno);
        return FAIL;
    }

    return (_servSocketFD);
}


/**
 * ACCEPT CONNECTIONS, wait for a connection to occur on a socket created with establish()
 * @return new socket fd, or FAIL
 */
int whatsappServer::accept_new_call()
{
    int newSocketFD; /* socket of new connection */

    if ((newSocketFD = accept(_servSocketFD, NULL, NULL)) < 0)
    {
        print_error("accept", errno);
        return FAIL;
    }

    char clientNamebuf[WA_MAX_NAME] = {0};
    read_data(newSocketFD, clientNamebuf, WA_MAX_NAME);
    std::string clientName(clientNamebuf);


    char fail[SUCCESS_FAIL_MSG_LEN] = {"dupsies :("}, success[SUCCESS_FAIL_MSG_LEN] = {
            "success :)"};
    if (nameAlreadyTaken(clientName))
    {
        cout << "duplicate" << endl;
        send(newSocketFD, fail, SUCCESS_FAIL_MSG_LEN, 0);
        // server don't print a thing if no connection!
    }
    else
    {
        send(newSocketFD, success, SUCCESS_FAIL_MSG_LEN, 0);
        print_connection_server(clientName);
        _namesOfClients.emplace(clientName);
    }
    client newClient = {clientName, newSocketFD};
    _serverClients.emplace_back(newClient);

    return newSocketFD;
}


// ============================
// =                          =
// =     HELPER  FUNCTIONS    =
// =                          =
// ============================


/**
 * Get name of client with input FD
 * @param clientFD
 * @param cName
 * @return FAIL/SUCCESS
 */
int whatsappServer::get_clientName(int clientFD, std::string &cName)
{
    for (client existingClient : _serverClients)
    {
        if (existingClient.fd == clientFD)
        {
            cName = existingClient.name;
            return SUCCESS;
        }
    }
    return FAIL; // no matching client found
}

int whatsappServer::get_clientFD(const std::string &name)
{
    for (client existingClient : _serverClients)
    {
        if (existingClient.name == name)
        {
            return existingClient.fd;
        }
    }
    return FAIL; // no matching client found}
}

/**
 * gets the FD of the server
 * @return server socketFD
 */
int whatsappServer::get_servSocketFD() const
{
    return _servSocketFD;
}

/**
 * checks whether input name already exists as a client name or group name
 * @param name
 * @return true if taken, false otherwise
 */
bool whatsappServer::nameAlreadyTaken(std::string name)
{
    if (_namesOfClients.empty())
    { return false; }
    else
    {
        bool noClientWithThatName = (_namesOfClients.find(name) == _namesOfClients.end());
        bool noGroupWithThatNAme = (_namesOfGroups.find(name) == _namesOfGroups.end());
        return !(noClientWithThatName && noGroupWithThatNAme);
    }
}


// ============================
// =                          =
// =      HANDLE  FUNCTIONS   =
// =                          =
// ============================

/**
 * reads and handles a request from client with clientFD socket
 * @param clientFD
 * @return FAIL/SUCCESS
 */
int whatsappServer::handleRequest(int clientFD)
{
    char incomingCommandBuff[WA_MAX_INPUT] = {0};
    if (read_data(clientFD, incomingCommandBuff, WA_MAX_INPUT) < 0)
    {
        // client disconnected without exiting
        return CLIENT_EXIT_RETVAL;
    }
    std::string command(incomingCommandBuff);
    std::string requestingClientName;
    get_clientName(clientFD, requestingClientName); // todo check if success;

    // output of parse_command
    std::vector<std::string> newGroup;
    std::string name, message;

    command_type commandType;
    group groupToSend;
    client clientToSend;

    parse_command(command, commandType, name, message, newGroup);

    bool success;
    switch (commandType)
    {
        case CREATE_GROUP:
            success = (createGroup(newGroup, name) == 0);
            replyToClient(clientFD, success);
            print_create_group(true, success, requestingClientName, name);
            return SUCCESS;
        case SEND:

            if (!nameAlreadyTaken(name))
            {
                print_send(true,false,requestingClientName,name,message);
                replyToClient(clientFD,false);
                return FAIL; }
            else
            {
                handleSendCommand(requestingClientName, name, message, groupToSend, success);
            }
            print_send(true, success, requestingClientName, name, message);
            replyToClient(clientFD, success);
            return SUCCESS;
        case WHO:
            replyWho(clientFD);
            get_clientName(clientFD, name);
            print_who_server(name);
            return SUCCESS;
        case EXIT:
            replyToClient(clientFD, true);
            return CLIENT_EXIT_RETVAL;
        case INVALID:
            return FAIL;
        default:
            return FAIL;
    }
}

/**
 * handles the special cases of a 'send' command (to group or client)
 * @param senderName
 * @param recipientName
 * @param message
 * @param groupToSend
 * @param success
 */
void
whatsappServer::handleSendCommand(const std::string &senderName, const std::string &recipientName,
                                  const std::string &message, group &groupToSend, bool &
success)
{
//    if (_namesOfGroups.find(recipientName) == _namesOfGroups.end() && _namesOfClients.find
//            (recipientName) == _namesOfClients.end())
//    {
//
//        success=FAIL;
//        print_send(true,success,senderName,recipientName,message);
//    }
//    else
    if (_namesOfGroups.find(recipientName) == _namesOfGroups.end())
    {
        for (client &curClient: _serverClients)
        {
            if (curClient.name == recipientName)
            {
                success = sendToClient(senderName, curClient, message) >= 0;
                break;
            }
        }
    }
    else
    {
        for (group &curGroup: _serverGroups)
        {
            if (curGroup.name == recipientName)
            {
                groupToSend = curGroup;
                success = sendToGroup(senderName, groupToSend, message) >= 0;
                break;
            }
        }
    }
}

/**
 * replies an 'who' inquery sending the requesting client a buffer with all members of server
 * @param clientFd
 * @return FAIL/SUCCESS
 */
int whatsappServer::replyWho(int clientFd)
{
    std::string allNames;
    for (unsigned int i = 0; i < _serverClients.size(); ++i)
    {
        if (i > 0)
        {
            allNames.append(",");
        }
        allNames.append(_serverClients.at(i).name);
    }

    const char *allNamesBuf = allNames.c_str();
    send(clientFd, allNamesBuf, WA_MAX_INPUT, 0);
    return SUCCESS;
}

/**
 * Create new group with groupMembers and groupName
 * @param groupMembers
 * @param groupName
 * @return FAIL/SUCCESS
 */
int whatsappServer::createGroup(std::vector<std::string> &groupMembers, std::string &groupName)
{
    std::set<std::string> membersToAdd;

    if (nameAlreadyTaken(groupName))
    { return (FAIL); }
    for (const std::string &member : groupMembers)
    {
        if (_namesOfClients.find(member) == _namesOfClients.end())
        {
            return FAIL; // member not found in existing clients list

        }
        else
        {
            membersToAdd.emplace(member); // add member to group to be created
        }

    }
    group newGroup = {.gmembers = membersToAdd, .name = groupName};
    _namesOfGroups.emplace(groupName);
    _serverGroups.emplace_back(newGroup);
    return SUCCESS;
}

/**
 * send client whether his request was successful
 * @param clientFD
 * @param successOrFailure
 * @return FAIL/SUCCESS
 */
int whatsappServer::replyToClient(int clientFD, bool successOrFailure)
{
    char bufSuc[SUCCESS_FAIL_MSG_LEN] = {"SUCCESS"};
    char bufFail[SUCCESS_FAIL_MSG_LEN] = {"FAILED"};
    if (successOrFailure)
    {
        send(clientFD, bufSuc, SUCCESS_FAIL_MSG_LEN, 0);
        return SUCCESS;
    }
    send(clientFD, bufFail, SUCCESS_FAIL_MSG_LEN, 0);
    return SUCCESS;
}



// ============================
// =                          =
// =      SEND  FUNCTIONS     =
// =                          =
// ============================
/**
 * send client a msg
 * @param senderName
 * @param receiver
 * @param msg
 * @return FAIL/SUCCESS
 */
int whatsappServer::sendToClient(const std::string &senderName, const client &receiver, const
std::string &msg)
{
    std::string fullMsg = senderName;
    fullMsg.append(": ").append(msg);
    int receptorFD = receiver.fd;
    std::string successMsg = SUCCESS_MSG_HDR + std::to_string(fullMsg.length());
    char bufRep[successMsg.length() + 1] = {0};
    send(receptorFD, fullMsg.c_str(), WA_MAX_MESSAGE + WA_MAX_NAME + 2, 0);
    read_data(receptorFD, bufRep, successMsg.length());
    std::string response(bufRep, successMsg.length());

    if (response != successMsg)
    { return FAIL; } // sending failed to one client!

    return 0;
}

/**
 * send a message to members of a group
 * @param senderName
 * @param groupToSend
 * @param msg
 * @return FAIL/SUCCESS
 */
int whatsappServer::sendToGroup(const std::string &senderName, const group &groupToSend, const
std::string &msg)
{
    if (groupToSend.gmembers.find(senderName) == groupToSend.gmembers.end())
    {
        return FAIL;
    }
    std::string fullMsg = senderName + ": " + msg;
    int receptorFD;
    std::string successMsg = SUCCESS_MSG_HDR + std::to_string(fullMsg.length());
    char bufRep[successMsg.length() + 1] = {0};

    for (const std::string &member: groupToSend.gmembers)
    {
        if (member != senderName)
        {
            receptorFD = get_clientFD(member);
            send(receptorFD, fullMsg.c_str(), WA_MAX_MESSAGE + WA_MAX_NAME + 2, 0);

            read_data(receptorFD, bufRep, successMsg.length());
            std::string response(bufRep, successMsg.length());

            if (response != successMsg)
            { return FAIL; } // sending failed to one client!
        }

    }
    return SUCCESS;
}


// ============================
// =                          =
// =      EXIT  FUNCTIONS     =
// =                          =
// ============================


/**
 * remove client from server
 * @param clientFd
 * @return FAIL/SUCCESS
 */
int whatsappServer::unregisterClient(int clientFd)
{
    if (_serverClients.empty() || _namesOfClients.empty())
    {
        return FAIL;
    }
    std::string toRemoveName;
    auto it = _serverClients.begin();
    for (; it != _serverClients.end(); ++it)//remove client from server
    {
        client curClient = *it;
        if (curClient.fd == clientFd)
        {

            toRemoveName = curClient.name;
            _serverClients.erase(it);
            break;
        }
    }

    std::vector<unsigned int> groupsToErase;
    for (unsigned int i = 0; i < _serverGroups.size(); i++)//remove client from groups
    {
        if (_serverGroups.at(i).gmembers.find(toRemoveName) != _serverGroups.at(i).gmembers.end())
        {
            _serverGroups.at(i).gmembers.erase(toRemoveName);
            if (_serverGroups.at(i).gmembers.empty())
            {
                groupsToErase.emplace_back(i);
            }
        }
    }
    for (unsigned int i : groupsToErase)
    {
        _serverGroups.erase(_serverGroups.begin() + i);
    }
    _namesOfClients.erase(toRemoveName);
    print_exit(true, toRemoveName);
    return SUCCESS;

}

/**
 * wait for user input to be 'exit'
 * @param exitFlag
 * @return
 */
void *whatsappServer::waitForExit(void *exitFlag)
{
    std::string userinput;
    bool run = true;
    while (run)
    {
        std::getline(std::cin, userinput);
        run = (userinput != "EXIT" && userinput != "exit");
    }

    print_exit();
    close(_servSocketFD);
    *(bool *) exitFlag = false;
    return exitFlag;
}

// ============================
// =                          =
// =      MAIN  FUNCTIONS     =
// =                          =
// ============================


/**
 * Parses commandline arguments, checks they're valid and sets inner members to their
 * corresponding values.
 * @param argc
 * @param argv
 * @param portNum
 * @return
 */
int checkArgs(int argc, char *argv[], unsigned short &portNum)
{
    if (argc != 2)
    {
        return FAIL;
    }
    std::string serverPort = argv[1];
//    std::regex port_regex(R"(^\d{1,10}$)");
    auto portMatch = isLegalPort(serverPort);

    if (!portMatch)
    {
        return FAIL;

    }
    else
    {
        portNum = (unsigned short) atoi(argv[1]);
        return SUCCESS;
    }
}


int main(int argc, char *argv[])
{
    unsigned short portNum = 0;
    bool keepAlive = true;
    // int clientFDs[MAX_CLIENTS];

    if (checkArgs(argc, argv, portNum) != 0)
    {
        print_server_usage();
        exit(1);
    }

    whatsappServer was(portNum);
    if (was.initSocket() < 0)
    {
        std::cout << "error initiating socket" << std::endl;
        exit(1);
    }

    int servSockFD = was.get_servSocketFD();

    fd_set clientsfds, readfds;
    FD_ZERO(&clientsfds);
    FD_ZERO(&readfds);
    FD_SET(servSockFD, &clientsfds);
    FD_SET(STDIN_FILENO, &clientsfds);

    int requestVal = 0;

    while (keepAlive)
    {
        readfds = clientsfds;
        if (select(MAX_CLIENTS + 1, &readfds, NULL, NULL, NULL) < 0)
        {
            print_error("select", errno);
            // terminate server
            return FAIL;
        }
        if (FD_ISSET(servSockFD, &readfds))
        {
            int newSockFd = was.accept_new_call();
            if (newSockFd < 0)
            { perror("oh no"); }
            else
            {
                FD_SET(newSockFd, &clientsfds);
            }
        }
        if (FD_ISSET(STDIN_FILENO, &readfds))
        {
            was.waitForExit(&keepAlive);
        }
        else
        {
            for (auto client: was._serverClients)
            {
                if (FD_ISSET(client.fd, &readfds))
                {
                    requestVal = was.handleRequest(client.fd);
                    if (requestVal == CLIENT_EXIT_RETVAL)
                    {
                        was.unregisterClient(client.fd);
                        FD_CLR(client.fd, &clientsfds);
                        FD_CLR(client.fd, &readfds);
                    }
                    break; // we may change _serverClients so we want to break from the loop
                }
            }
        }
    }
}


