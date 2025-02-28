#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>
#include <algorithm>
#include "telnet.hpp"

// static const char *TAG = "telnet";

#define IAC 255 // Interpret as Command
#define SB 250
#define NAWS 31
#define SGA 3 // Suppress Go Ahead
#define SE 248 // Suppress Go Ahead

void TelnetServer::processRx(const int sock_fd, const uint8_t *data, size_t len)
{
    std::vector<uint8_t> response;

    printf("received: ");
    for (size_t i = 0; i < len; i++)
        printf("%02x ", data[i]);
    printf("\n");

    if (data[0] != IAC)
    {
        uint8_t new_data[10];
        if (data[0] == 0x0d)
        {
            new_data[0] = '\r';
            new_data[1] = '\n';
            BaseTcpServer::send(sock_fd, new_data, 2); // Handle newline
            return;
        }
        if (data[0] == 0x7f)
        {
            new_data[0] = '\b';
            new_data[1] = ' ';
            new_data[2] = '\b';
            BaseTcpServer::send(sock_fd, new_data, 3); // Handle backspace
            return;
        }

        BaseTcpServer::send(sock_fd, data, len); // Echo from server
        return;
    }

    for (size_t i = 0; i < len; i++)
    {
        if (data[i] == IAC)
        {
            if (i + 1 < len && data[i + 1] == SB) // Start of subnegotiation
            {
                size_t j = i + 2;
                while (j < len && !(data[j] == IAC && j + 1 < len && data[j + 1] == SE))
                {
                    j++;
                }

                if (j + 1 < len)
                {
                    // Process subnegotiation data between i+2 and j
                    processSubnegotiation(data + i + 2, j - (i + 2));
                    i = j + 1; // Move past SE
                }
            }
            else if (i + 2 < len) // Normal command processing
            {
                uint8_t command = data[i + 1];
                TelnetOption option = static_cast<TelnetOption>(data[i + 2]);

                
                
                auto it = std::find_if(m_Demans.begin(), m_Demans.end(), [&](const NegotiationOption &opt)
                                       { return opt.m_Option == option; });

                if (it != m_Demans.end())
                {
                    if ((it->m_Command == TelnetCommand::DO && command == static_cast<uint8_t>(TelnetCommand::WILL)) ||
                        (it->m_Command == TelnetCommand::WILL && command == static_cast<uint8_t>(TelnetCommand::DO)))
                        m_Demans.erase(it);
                }
                else
                {
                    response.push_back(IAC);
                    if (command == static_cast<uint8_t>(TelnetCommand::WILL))
                    {
                        response.push_back(static_cast<uint8_t>(TelnetCommand::DONT));
                    }
                    else if (command == static_cast<uint8_t>(TelnetCommand::DO))
                    {
                        response.push_back(static_cast<uint8_t>(TelnetCommand::WONT));
                    }
                    else if (command == static_cast<uint8_t>(TelnetCommand::WONT))
                    {
                        response.push_back(static_cast<uint8_t>(TelnetCommand::DONT));
                    }
                    else if (command == static_cast<uint8_t>(TelnetCommand::DONT))
                    {
                        response.push_back(static_cast<uint8_t>(TelnetCommand::WILL));
                    }
                    else
                    {   
                        printf("%u %02x\n", __LINE__, response.back());
                        response.pop_back();
                        continue;
                    }
                    response.push_back(data[i + 2]);
                    printf("%u %02x\n", __LINE__, response.back());
                }
                i += 2; // Move past option
            }
        }
    }

    printf("sending: ");
    for (size_t i = 0; i < response.size(); i++)
        printf("%02x ", response[i]);
    printf("\n");

    if (!response.empty())
    {
        BaseTcpServer::send(sock_fd, response.data(), response.size());
    }
}

void TelnetServer::sendDemands(const int sock_fd)
{
    std::vector<uint8_t> data;

    for (auto &demand : m_Demans)
    {
        data.push_back(IAC),
            data.push_back(static_cast<uint8_t>(demand.m_Command));
        data.push_back(static_cast<uint8_t>(demand.m_Option));
    }

    BaseTcpServer::send(sock_fd, &data[0], data.size());
}

void TelnetServer::sendWelcomeMessage(const int sock_fd)
{
    sendDemands(sock_fd);
}

void TelnetServer::processSubnegotiation(const uint8_t *data, size_t len)
{
    printf("Processing subnegotiation: ");
    for (size_t i = 0; i < len; i++)
        printf("%02x ", data[i]);
    printf("\n");

    // Handle subnegotiation logic here
}
