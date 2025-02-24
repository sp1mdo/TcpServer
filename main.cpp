#include <string>
#include "BaseServer.hpp"

int main(int argc, char **argv)
{
    BaseTcpServer server(std::stoi(argv[1]));
    server.run();
}