#include <string>
#include <thread>
#include <memory>

#include "BaseServer.hpp"
#include "Modbus.h"
#include "Telnet.hpp"
#include "Prompt.hpp"
#include "RingBuffer.hpp"

void print_text(const char *format, ...);

using namespace cli;

RingBuffer<char, 100> cliToTelnet;
RingBuffer<char, 100> telnetToCli;

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

void callback(int id, const std::string &str)
{
    std::cout << "Received : id=" << id << " arg=[" << str << "]" << std::endl;
    print_text("Received :id = %d arg=[%s]\n",id, str.c_str());
}

void cli_routine(const std::string name)
{
    Prompt my_cli(name);
    int fun_id{0};
    my_cli.insertMapElement("polska szczecin", std::bind(callback, fun_id++, std::placeholders::_1));
    my_cli.insertMapElement("polska warszawa", std::bind(callback, fun_id++, std::placeholders::_1));
    my_cli.insertMapElement("niemcy berlin", std::bind(callback, fun_id++, std::placeholders::_1));
    my_cli.insertMapElement("niemcy frankfurt", std::bind(callback, fun_id++, std::placeholders::_1));

    my_cli.Run();
}


int main(int argc, char **argv)
{
    std::thread modbus_server_thread(server_thread, std::make_unique<ModbusServer>(1, 502));
    std::thread echo_server_thread(server_thread, std::make_unique<TelnetServer>(23));
    std::thread cli_th(cli_routine, "CLI");
    modbus_server_thread.join();
    echo_server_thread.join();
    cli_th.join();
}