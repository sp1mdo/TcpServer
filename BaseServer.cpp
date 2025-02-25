#include <iostream>
#include <vector>
#include <algorithm>

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <cerrno>
#include <poll.h>
#include "BaseServer.hpp"

constexpr size_t SIZE = 128;

bool operator<(const pollfd &lhs, const pollfd &rhs)
{
    return lhs.fd < rhs.fd;
}

BaseTcpServer::BaseTcpServer(uint16_t port) : m_ServerPort(port)
{
    m_ServerFD = socket(AF_INET, SOCK_STREAM, 0);
    m_pollFds.push_back(pollfd{m_ServerFD, POLLIN | POLLPRI, 0});

    int opt = 1;
    if (setsockopt(m_ServerFD, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(m_ServerPort);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int bindResult = bind(m_ServerFD, (struct sockaddr *)&addr, sizeof(addr));
    if (bindResult == -1)
    {
        std::cerr << "bindResult\n";
    }

    int listenResult = listen(m_ServerFD, 5);
    if (listenResult == -1)
    {
        std::cerr << "listenResult\n";
    }
    std::cout << "server start\n";
};

int BaseTcpServer::send(const int client_sock, const uint8_t *data, size_t len) const
{
    size_t to_write = len;
    int written = -1;
    while (to_write > 0)
    {
        written = ::send(client_sock, data + (len - to_write), to_write, 0);
        if (written < 0)
        {
            std::cerr << "Error occurred during sending:" << strerror(errno) << "\n";
        }
        to_write -= written;
    }
    return written;
}

void BaseTcpServer::updateFds(void)
{
    // Remove polldfs from the main vector
    std::erase_if(m_pollFds, [&](const pollfd &pfd1)
                  { return std::find_if(m_pollFds_ToRemove.begin(), m_pollFds_ToRemove.end(), [&](const pollfd &pfd2)
                                        { return pfd1.fd == pfd2.fd; }) != m_pollFds_ToRemove.end(); });

    // Close sockets with disconnected clients
    for (auto &fd : m_pollFds_ToRemove)
    {
        close(fd.fd);
    }

    // Process adding newly connected client pollfds
    for (auto &fd : m_pollFds_ToAdd)
    {
        m_pollFds.push_back(fd);
    }

    m_pollFds_ToRemove.clear();
    m_pollFds_ToAdd.clear();
}

void BaseTcpServer::run(void)
{
    m_pollFds[0].fd = m_ServerFD;
    m_pollFds[0].events = POLLIN | POLLPRI;

    while (1)
    {
        updateFds();
        // std::cout << "Clients connected " << m_pollFds.size() - 1 << "\n";
        int pollResult = poll(&m_pollFds[0], m_pollFds.size(), -1);
        if (pollResult < 0)
        {
            std::cerr << "poll error: " << strerror(errno) << " " << m_pollFds.size() << "\n";
        }
        if (pollResult > 0)
        {
            if (m_pollFds[0].fd > 0 && (m_pollFds[0].revents & POLLIN))
            {
                struct sockaddr_in cliaddr;
                int addrlen = sizeof(cliaddr);
                int client_socket = accept(m_ServerFD, (struct sockaddr *)&cliaddr, (socklen_t *)&addrlen);

                // Add client fd to vector od pollfds that are going to be added into main vector of polldfs in next loop cycle
                if (client_socket != -1)
                    m_pollFds_ToAdd.emplace(pollfd{client_socket, POLLIN | POLLPRI, 0});
                else
                    std::cout << "accept fail: " << inet_ntoa(cliaddr.sin_addr) << ":" << (cliaddr.sin_port) << "\n";
            }
            for (size_t i = 1; i < m_pollFds.size(); i++)
            {
                if (m_pollFds[i].fd > 0 && m_pollFds[i].revents & POLLIN)
                {
                    uint8_t buf[SIZE];
                    int bufSize = read(m_pollFds[i].fd, buf, SIZE - 1);
                    if (bufSize == -1)
                    {
                        // std::cout << "Client [fd=" << m_pollFds[i].fd << "] disconnected\n";
                        m_pollFds_ToRemove.emplace(m_pollFds[i]);
                    }
                    else if (bufSize == 0)
                    {
                        // std::cout << "Client [fd=" << m_pollFds[i].fd << "] disconnected\n";
                        m_pollFds_ToRemove.emplace(m_pollFds[i]);
                    }
                    else
                    {
                        // Handle received data according to your business logic
                        processRx(m_pollFds[i].fd, buf, bufSize);
                    }
                }
            }
        }
    }
}

void BaseTcpServer::processRx(const int sock_fd, const uint8_t *data, size_t len)
{
    write(sock_fd, data, len);
}
