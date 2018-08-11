
#include <sys/socket.h>
#include <netinet/in.h>
#include "whatsappio.h"
#include <unistd.h>
#include <netdb.h>
#include <cstring>
#include <iostream>
#include <algorithm>

#ifndef EX4_PROTOCOL_H

#define EX4_PROTOCOL_H

#define MSG_SIZE 256
#define SUCCESS 0
#define FAIL -1
#define SUCCESS_FAIL_MSG_LEN 11
#define SUCCESS_MSG_HDR "got message of length: "
#define SUCCESS_MSG_LEN 35


int read_data(int socketFD, char *buf, unsigned long n);
bool isValidIp(char * ip_str);
bool isLegalPort(const std::string& s);

#endif //EX4_PROTOCOL_H
