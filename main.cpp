#include <string>
#include <thread>
#include <memory>

#include "BaseServer.hpp"
#include "Modbus.h"

void server_thread(std::unique_ptr<BaseTcpServer> my_server)
{
    my_server->run();
}

int main(int argc, char **argv)
{
    std::thread modbus_server_thread(server_thread, std::make_unique<ModbusServer>(1, 502));
    std::thread echo_server_thread(server_thread, std::make_unique<BaseTcpServer>(23));
    modbus_server_thread.join();
    echo_server_thread.join();
}