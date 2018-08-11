
#ifndef EX4_WHATSAPPSERVER_H
#define EX4_WHATSAPPSERVER_H


static const int MAX_NUM_PENDING = 10;
static const int MAXHOSTNAME = 30; // todo what should this be???????
static const int MAX_CLIENTS = 10;
static const int CLIENT_EXIT_RETVAL = -2;
#define NUM_ARGS 2

#include <cstddef>
#include <set>
#include "Protocol.h"

using std::cin;
using std::cout;
using std::endl;
using std::getline;


struct client {
    std::string name;
    int fd;
};
struct group {
    std::set<std::string> gmembers;
    std::string name;
};

// A Class representing a whatsapp server object
class whatsappServer {
private:

    std::set<std::string> _namesOfGroups;
    std::set<std::string> _namesOfClients;


    struct sockaddr_in _serverSocket;

    int _servSocketFD;

    unsigned short _servPortnum;

    int createGroup(std::vector<std::string> &groupMembers, std::string &groupName);

public:

    std::vector<client> _serverClients;
    std::vector<group> _serverGroups;

    int initSocket();

    explicit whatsappServer(unsigned short servPortnum);

    int get_servSocketFD() const;

    int accept_new_call();

    int get_clientName(int clientFD, std::string &cName);

    int get_clientFD(const std::string &name);

    int replyToClient(int clientFD, bool successOrFailure);

    int handleRequest(int clientFD);

    void *waitForExit(void *exitFlag);

    bool nameAlreadyTaken(std::string name);

    int unregisterClient(int clientFd);

    int replyWho(int clientFd);

    int sendToGroup(const std::string &senderName, const group &groupToSend, const
    std::string &msg);

    int sendToClient(const std::string &senderName, const client &receiver, const
    std::string &msg);

    void handleSendCommand(const std::string &senderName, const std::string &recipientName,
                           const std::string &message, group &groupToSend, bool &success);
};


#endif //EX4_WHATSAPPSERVER_H
