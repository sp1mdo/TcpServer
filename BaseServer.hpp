#include <vector>
#include <iostream>
#include <poll.h>
#include <unistd.h>
#include <set>

constexpr size_t SIZE=128;

class BaseTcpServer
{
public:
    BaseTcpServer(uint16_t port);
    void run(void);

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
    virtual void processRx(const int sock_fd, uint8_t *data, size_t len) ;
    uint16_t m_ServerPort;
    int m_ServerFD;
    std::vector<pollfd> m_pollFds;
    std::set<pollfd> m_pollFds_ToAdd;
    std::set<pollfd> m_pollFds_ToRemove;
};
