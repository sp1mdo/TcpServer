#ifndef _BASE_SERVER_H
#define _BASE_SERVER_H

#include <vector>
#include <iostream>
#include <poll.h>
#include <unistd.h>
#include <set>

class BaseTcpServer
{
public:
    BaseTcpServer(uint16_t port);
    void run(void);
    int send(const int client_sock, const uint8_t* data, size_t len) const ;

    virtual ~BaseTcpServer()
    {
        for(const auto &pollfd : m_pollFds)
        {
            std::cout << "Terminating connection with client [fd=" << pollfd.fd << "]\n";
            close(pollfd.fd);
        }

        close(m_ServerFD);
    }

private:
    void updateFds(void);
    virtual void processRx(const int sock_fd, const uint8_t *data, size_t len) = 0;
    virtual void sendWelcomeMessage(const int sock_fd) = 0;    
    uint16_t m_ServerPort;
    int m_ServerFD;
    std::vector<pollfd> m_pollFds;
    std::set<pollfd> m_pollFds_ToAdd;
    std::set<pollfd> m_pollFds_ToRemove;
};
#endif // _BASE_SERVER_H