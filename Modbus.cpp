#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <memory>
#include <arpa/inet.h> // for htons etc.

#include "Modbus.h"

uint16_t holdingRegisters[100] = {0};
uint16_t inputRegisters[100] = {0};
bool coils[8] = {true, false, false, false, false, false, false, true};

void ModbusServer::sendExceptionCode(ExceptionCode code, std::unique_ptr<modbus_query_t> &query, const int sock_fd)
{
    modbus_exception_t exception;
    exception.transaction_id = htons(query->transaction_id);
    exception.protocol_id = htons(query->protocol_id);
    exception.length = htons(query->length);
    exception.unit_id = query->unit_id;
    exception.function_code = (0x80 | query->function_code);
    exception.exception_code = static_cast<uint8_t>(code);

    BaseTcpServer::send(sock_fd, reinterpret_cast<uint8_t *>(&exception), sizeof(exception));
    return;
}

void ModbusServer::processRx(const int sock_fd, const uint8_t *data, size_t len)
{
    std::unique_ptr<modbus_query_t> query = parse_modbus_tcp_raw_data((uint8_t *)data, len);
    if (query == NULL)
    {
        std::cerr << "query is NULL.\n";
        return;
    }

    if (query->unit_id != m_SlaveId)
    {
        std::cerr << "other ID\n";
        // Gateway Target Device Failed to Respond (used for invalid slave IDs)
        sendExceptionCode(ExceptionCode::GatewayTargetFailedToRespond, query, sock_fd);
        return;
    }

    uint16_t *registers = (uint16_t *)data + 10;
    uint8_t *send_buf = new uint8_t[12 + sizeof(uint16_t) * (query->word_count)];
    if (send_buf == NULL)
    {
        std::cerr << "send_buf is NULL.\n";
        return;
    }

    modbus_reply_t *reply = (modbus_reply_t *)send_buf;

    reply->transaction_id = htons(query->transaction_id);
    reply->protocol_id = htons(query->protocol_id);
    reply->length = htons(3 + sizeof(uint16_t) * (query->word_count));
    reply->unit_id = query->unit_id;
    reply->function_code = query->function_code;
    reply->byte_count = (uint8_t)(sizeof(uint16_t) * query->word_count);

    uint8_t tmp_bytes = 0;
    uint16_t *offset = (uint16_t *)(send_buf + sizeof(modbus_reply_t));

    switch (static_cast<FunctionCode>(query->function_code))
    {
    case FunctionCode::ReadCoils:
    {
        reply->byte_count = (uint8_t)(query->word_count / 8);
        uint8_t *offset8 = (uint8_t *)offset;
        memset(offset, 0, reply->byte_count);
        for (size_t i = 0; i < query->word_count; i++)
        {
            if (coils[i] == true)
                offset8[i / 8] |= (1 << i);
            else
                offset8[i / 8] &= ~(1 << (i));
        }
        break;
    }
    case FunctionCode::ReadHoldingRegisters:
        reply->byte_count = (uint8_t)(sizeof(uint16_t) * query->word_count);
        for (size_t i = 0; i < reply->byte_count / sizeof(uint16_t); i++)
        {
            if (query->start_addr + i < 100)
            {
                offset[i] = htons(holdingRegisters[(query->start_addr) + i]);
            }
            else
                sendExceptionCode(ExceptionCode::IllegalDataAddress, query, sock_fd);
        }
        break;

    case FunctionCode::ReadInputRegisters:
        reply->byte_count = (uint8_t)(sizeof(uint16_t) * query->word_count);
        for (size_t i = 0; i < reply->byte_count / sizeof(uint16_t); i++)
        {
            if (query->start_addr + i < 100)
            {
                offset[i] = htons(inputRegisters[(query->start_addr) + i]);
            }
            else
                sendExceptionCode(ExceptionCode::IllegalDataAddress, query, sock_fd);
        }
        break;

    case FunctionCode::WriteSingleCoil:
        memcpy((void *)send_buf, (void *)data, 12);
        tmp_bytes = 3;
        break;

    case FunctionCode::WriteSingleRegister:
        holdingRegisters[query->start_addr] = htons(registers[0]);
        memcpy((void *)send_buf, (void *)data, 12);
        reply->byte_count = 3;
        break;

    case FunctionCode::WriteMultipleCoils:
        sendExceptionCode(ExceptionCode::IllegalFunction, query, sock_fd);
        break;

    case FunctionCode::WriteMultipleRegisters:
    {
        reply->byte_count = (uint8_t)(sizeof(uint16_t) * query->word_count);
        uint16_t *data_ptr = (uint16_t *)(data + 13);
        for (size_t i = 0; i < reply->byte_count / sizeof(uint16_t); i++)
        {
            if (query->start_addr + i < 100) // TODO reply invalid data address in such case
            {
                holdingRegisters[query->start_addr + i] = ntohs(data_ptr[i]); // yet to check
                // holdingRegisters[query->start_addr + i] = ((uint8_t *)data)[13 + sizeof(uint16_t) * i] * 256 + ((uint8_t *)data)[14 + sizeof(uint16_t) * i];
            }
            else
                sendExceptionCode(ExceptionCode::IllegalDataAddress, query, sock_fd);
        }

        memcpy((void *)send_buf, (void *)data, 12);
        reply->byte_count = 3;
        break;
    }

    default:
        sendExceptionCode(ExceptionCode::IllegalFunction, query, sock_fd);
        break;
    }

    size_t to_write = sizeof(modbus_reply_t) + reply->byte_count + tmp_bytes;
    BaseTcpServer::send(sock_fd, send_buf, to_write);

    delete[] send_buf;
}

std::unique_ptr<modbus_query_t> ModbusServer::parse_modbus_tcp_raw_data(const uint8_t *data, size_t len)
{
    auto query = std::unique_ptr<modbus_query_t>(new modbus_query_t); // Safer allocation
    query->transaction_id = ntohs(*(uint16_t *)&data[0]);
    query->protocol_id = ntohs(*(uint16_t *)&data[2]);
    query->length = ntohs(*(uint16_t *)&data[4]);
    query->unit_id = data[6];
    query->function_code = data[7];
    query->start_addr = ntohs(*(uint16_t *)&data[8]);
    query->word_count = ntohs(*(uint16_t *)&data[10]);

    return query;
}
