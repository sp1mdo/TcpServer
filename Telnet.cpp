#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>
#include <algorithm>
#include "Telnet.hpp"
#include "RingBuffer.hpp"

int g_telnet_sock{-1};
extern RingBuffer<char, 100> telnetToCli;
// static const char *TAG = "telnet";

std::string TelnetServer::commandToString(uint8_t cmd)
{
    using namespace TelnetCommand;

    switch (cmd)
    {
    case IAC:
        return "IAC";
    case DO:
        return "DO";
    case DONT:
        return "DONT";
    case WILL:
        return "WILL";
    case WONT:
        return "WONT";
    case SUBOPTION:
        return "SUBOPTION";
    default:
        return std::to_string(cmd);;
    };
}

std::string TelnetServer::optionToString(uint8_t opt)
{
    using namespace TelnetOption;
    switch (opt)
    {
        case IAC:
        return "IAC";
    case DO:
        return "DO";
    case DONT:
        return "DONT";
    case WILL:
        return "WILL";
    case WONT:
        return "WONT";
    case SUBOPTION:
        return "SUBOPTION";


    case BINARY_TRANSMISSION:
        return "Binary Transmission";
    case ECHO:
        return "Echo";
    case RECONNECTION:
        return "Reconnection";
    case SUPPRESS_GO_AHEAD:
        return "Supress go ahead";
    case APPROX_MSG_SIZE_NEGOTIATION:
        return "Approx msg size negotiation";
    case STATUS:
        return "Status";
    case TIMING_MARK:
        return "Timing mark";
    case REMOTE_CONTROLLED_TRANSMIT_ECHO:
        return "Remote controlled transmit echo";
    case OUTPUT_LINE_WIDTH:
        return "Output line width";
    case OUTPUT_PAGE_SIZE:
        return "Output page size";
    case OUTPUT_CARRIAGE_RETURN_DISPOSITION:
        return "Output carriage return disposition";
    case OUTPUT_HORIZONTAL_TAB_STOPS:
        return "Output horizontal tab stops";
    case OUTPUT_HORIZONTAL_TAB_DISPOSITION:
        return "Output horizontal tab disposition";
    case OUTPUT_FORMFEED_DISPOSITION:
        return "OUTPUT_FORMFEED_DISPOSITION";
    case OUTPUT_VERTICAL_TABSTOPS:
        return "OUTPUT_VERTICAL_TABSTOPS";
    case OUTPUT_VERTICAL_TAB_DISPOSITION:
        return "OUTPUT_VERTICAL_TAB_DISPOSITION";
    case OUTPUT_LINEFEED_DISPOSITION:
        return "OUTPUT_LINEFEED_DISPOSITION";
    case EXTENDED_ASCII:
        return "EXTENDED_ASCII";
    case LOGOUT:
        return "LOGOUT";
    case BYTE_MACRO:
        return "BYTE_MACRO";
    case DATA_ENTRY_TERMINAL:
        return "DATA_ENTRY_TERMINAL";
    case SUPDUP:
        return "SUPDUP";
    case SUPDUP_OUTPUT:
        return "SUPDUP_OUTPUT";
    case SEND_LOCATION:
        return "SEND_LOCATION";
    case TERMINAL_TYPE:
        return "TERMINAL_TYPE";
    case END_OF_RECORD:
        return "END_OF_RECORD";
    case TACACS_USER_IDENTIFICATION:
        return "TACACS_USER_IDENTIFICATION";
    case OUTPUT_MARKING:
        return "OUTPUT_MARKING";
    case TERMINAL_LOCATION_NUMBER:
        return "TERMINAL_LOCATION_NUMBER";
    case REGIME_3270:
        return "REGIME_3270";
    case X3_PAD:
        return "X3_PAD";
    case NEGOTIATE_ABOUT_WINDOW_SIZE:
        return "NEGOTIATE_ABOUT_WINDOW_SIZE";
    case TERMINAL_SPEED:
        return "TERMINAL_SPEED";
    case REMOTE_FLOW_CONTROL:
        return "REMOTE_FLOW_CONTROL";
    case LINE_MODE:
        return "LINE_MODE";
    case X_DISPLAY_LOCATION:
        return "X_DISPLAY_LOCATION";
    case ENVIRONMENT_OPTION:
        return "ENVIRONMENT_OPTION";
    case AUTHENTICATION_OPTION:
        return "AUTHENTICATION_OPTION";
    case ENCRYPTION_OPTION:
        return "ENCRYPTION_OPTION";
    case NEW_ENVIRONMENT_OPTION:
        return "NEW_ENVIRONMENT_OPTION";

    default:
        return std::to_string(opt);;;
    };
}

void TelnetServer::processRx(const int sock_fd, const uint8_t *data, size_t len)
{
    g_telnet_sock = sock_fd;
    std::vector<uint8_t> response;
    response.reserve(len);


    if (data[0] != TelnetCommand::IAC)
    {
        // uint8_t new_data[10];
        // if (data[0] == 0x0d)
        //{
        //     new_data[0] = '\r';
        //     new_data[1] = '\n';
        //     BaseTcpServer::send(sock_fd, new_data, 2); // Handle newline
        //     return;
        // }
        // if (data[0] == 0x7f)
        //{
        //    new_data[0] = '\b';
        //    new_data[1] = ' ';
        //    new_data[2] = '\b';
        //    BaseTcpServer::send(sock_fd, new_data, 3); // Handle backspace
        //    return;
        //}

        for (size_t i = 0; i < len; i++)
        {
            if (data[i] != 0x00)
                telnetToCli.push_back(data[i]);
        }

        // BaseTcpServer::send(sock_fd, data, len); // Echo from server
        return;
    }
    else
    {
        //printf("\nreceived: ");
        for (size_t i = 0; i < len; i++)
        {
            if(data[i] == TelnetCommand::IAC)
                printf("\nCLIENT: ");

            printf(" %s ",optionToString(data[i]).c_str());
        }
        printf("\n");
    }

    for (size_t i = 0; i < len; i++)
    {
        if (data[i] == TelnetCommand::IAC)
        {
            if (i + 1 < len && data[i + 1] == SB) // Start of subnegotiation
            {
                size_t j = i + 2;
                while (j < len && !(data[j] == TelnetCommand::IAC && j + 1 < len && data[j + 1] == SUBCOMMAND_END))
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
                uint8_t option = data[i + 2];

                {
                    printf("\n Server :"); //TODO
                    response.push_back(TelnetCommand::IAC);
                    printf(" %s ",commandToString(response.back()).c_str());

                    if (command == TelnetCommand::WILL && option == TelnetOption::ECHO)
                    {
                        response.push_back((TelnetCommand::DONT));
                        printf(" %s ",commandToString(response.back()).c_str());
                    }
                    else if (command == TelnetCommand::DO && option == TelnetOption::ECHO)
                    {
                        response.push_back((TelnetCommand::WILL));
                        printf(" %s ",commandToString(response.back()).c_str());
                    }
                    else if (command == TelnetCommand::WONT && option == TelnetOption::ECHO)
                    {
                        response.push_back((TelnetCommand::DONT));
                        printf(" %s ",commandToString(response.back()).c_str());
                    }
                    else if (command == TelnetCommand::DONT && option == TelnetOption::ECHO)
                    {
                        response.push_back((TelnetCommand::WILL));
                        printf(" %s ",commandToString(response.back()).c_str());
                    }
                    else if (command == TelnetCommand::DO && option == TelnetOption::LINE_MODE)
                    {
                        response.push_back((TelnetCommand::WONT));
                        printf(" %s ",commandToString(response.back()).c_str());
                    }
                    else if (command == TelnetCommand::DONT && option == TelnetOption::LINE_MODE)
                    {
                        response.push_back((TelnetCommand::WONT));
                        printf(" %s ",commandToString(response.back()).c_str());
                    }
                    else if (command == TelnetCommand::WILL && option == TelnetOption::LINE_MODE)
                    {
                        response.push_back((TelnetCommand::DONT));
                        printf(" %s ",commandToString(response.back()).c_str());
                    }
                    else if (command == TelnetCommand::WONT && option == TelnetOption::LINE_MODE)
                    {
                        response.push_back((TelnetCommand::DONT));
                        printf(" %s ",commandToString(response.back()).c_str());
                    }
                    else if (command == (TelnetCommand::DO))
                    {
                        response.push_back((TelnetCommand::WILL));
                        printf(" %s ",commandToString(response.back()).c_str());
                    }
                    else if (command == (TelnetCommand::WONT))
                    {
                        response.push_back((TelnetCommand::DONT));
                        printf(" %s ",commandToString(response.back()).c_str());
                    }
                    else if (command == (TelnetCommand::DONT))
                    {
                        response.push_back((TelnetCommand::WONT));
                        printf(" %s ",commandToString(response.back()).c_str());
                    }
                    else
                    {
                        response.pop_back();
                        printf("\r");
                        continue;
                    }
                    response.push_back(data[i + 2]);
                    printf(" %s ",optionToString(response.back()).c_str());
                }
                i += 2; // Move past option
            }
        }
    }
    response.push_back(0xff);
    response.push_back(0xfe);
    response.push_back(0x22);
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
        data.push_back(TelnetCommand::IAC),
        printf("\n %s ",commandToString(data.back()).c_str());
        data.push_back((demand.m_Command));
        printf(" %s ",commandToString(data.back()).c_str());
        data.push_back((demand.m_Option));
        printf(" %s",optionToString(data.back()).c_str());
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
