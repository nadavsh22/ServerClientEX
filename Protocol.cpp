
#include "Protocol.h"
#define DELIM "."

/**
 * basic method to read data from socket
 * @param socketFD
 * @param buf
 * @param n
 * @return
 */
int read_data(int socketFD, char *buf, unsigned long n)
{
    unsigned int bcount;       /* counts bytes read */
    int br;           /* bytes read this pass */
    bcount = 0;
    br = 0;

    while (bcount < n)
    { /* loop until full buffer */
        br = (int)recv(socketFD, buf, (size_t)n - bcount, 0);
        if (br > 0)
        {
            bcount += br;
            buf += br;
        }

        if (br < 1)
        {
            return (-1);
        }
    }
    return bcount;
}
/* return 1 if string contain only digits, else return 0 */
int valid_digit(char *ip_str)
{
    while (*ip_str) {
        if (*ip_str >= '0' && *ip_str <= '9')
            ++ip_str;
        else
            return 0;
    }
    return 1;
}

/* return 1 if IP string is valid, else return 0 */
bool isValidIp(char *ip_str)
{
    int  num, dots = 0;
    char *ptr;

    if (ip_str == NULL)
        return false;

    ptr = strtok(ip_str, DELIM);

    if (ptr == NULL)
        return false;

    while (ptr) {

        /* after parsing string, it must contain only digits */
        if (!valid_digit(ptr))
            return false;

        num = atoi(ptr);

        /* check for valid IP */
        if (num >= 0 && num <= 255) {
            /* parse remaining string */
            ptr = strtok(NULL, DELIM);
            if (ptr != NULL)
                ++dots;
        } else
            return false;
    }

    /* valid IP string must contain 3 dots */
    return dots == 3;
}
bool isLegalPort(const std::string& s)
{
    return !s.empty() && std::find_if(s.begin(),
                                      s.end(), [](char c) { return !std::isdigit(c); }) == s.end();
}