#include <string>
#include <thread>
#include <memory>

#include "BaseServer.hpp"
#include "Modbus.h"
#include "telnet.hpp"

void server_thread(std::unique_ptr<BaseTcpServer> my_server)
{
    try
    {
        my_server->run();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
}

int main(int argc, char **argv)
{
    std::thread modbus_server_thread(server_thread, std::make_unique<ModbusServer>(1, 502));
    std::thread echo_server_thread(server_thread, std::make_unique<TelnetServer>(23));
    modbus_server_thread.join();
    echo_server_thread.join();
}