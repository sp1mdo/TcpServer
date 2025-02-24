#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <poll.h>
#include <vector>
#include <iostream>

#include "BaseServer.hpp"

int main(int argc, char **argv)
{
    BaseTcpServer server(atoi(argv[1]));
    server.run();
}