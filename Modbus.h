#ifndef _MODBUS_H
#define _MODBUS_H

#include <inttypes.h>
#include <cstdlib>
#include <memory>
#include <map>

#include "BaseServer.hpp"

#define READ_COILS 1
#define READ_DISCREETE_INPUTS 2
#define READ_HOLDING_REGISTERS 3
#define READ_INPUT_REGISTERS 4
#define WRITE_SINGLE_COIL 5
#define WRITE_SINGLE_REGISTER 6 
#define WRITE_MULTIPLE_COILS 15
#define WRITE_MULTIPLE_REGISTERS 16


enum class FunctionCode : uint8_t
{
    ReadCoils = 1,
    ReadDiscreteInputs = 2,
    ReadHoldingRegisters = 3,
    ReadInputRegisters = 4,
    WriteSingleCoil = 5,
    WriteSingleRegister = 6,
    WriteMultipleCoils = 15,
    WriteMultipleRegisters = 16
} ;

typedef struct modbus_query_t
{
    uint16_t transaction_id;
    uint16_t protocol_id;
    uint16_t length;
    uint8_t unit_id;
    uint8_t function_code;
    uint16_t start_addr;
    uint16_t word_count;
    // uint16_t *data;
} __attribute__((packed)) modbus_query_t;

typedef struct modbus_reply_t
{
    uint16_t transaction_id;
    uint16_t protocol_id;
    uint16_t length;
    uint8_t unit_id;
    uint8_t function_code;
    uint8_t byte_count;
} __attribute__((packed)) modbus_reply_t;

std::unique_ptr<modbus_query_t> parse_modbus_tcp_raw_data(const uint8_t *data, size_t len);
int tcp_server_recv(uint8_t *payload, size_t length, const int sock);

class ModbusServer : public BaseTcpServer
{
public:
    ModbusServer(uint8_t slave_id, uint16_t port) : BaseTcpServer(port), m_SlaveId(slave_id) {  };
    void processRx(const int sock_fd, uint8_t *data, size_t len) override;

    //std::map<uint16_t, uint16_t *> holdingRegisters;
    //std::map<uint16_t, uint16_t *> inputRegisters;
    

private:
    std::unique_ptr<modbus_query_t> parse_modbus_tcp_raw_data(const uint8_t *data, size_t len);
    uint8_t m_SlaveId;
};

#endif