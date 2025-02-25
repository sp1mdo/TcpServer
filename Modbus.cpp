#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <memory>

#include "Modbus.h"
#include <arpa/inet.h> // for htons etc.

uint16_t holdingRegisters[100] = {0};
uint16_t inputRegisters[100] = {0};
bool coils[8] = {true, false, false, false, false, false, false, true};

void ModbusServer::processRx(const int sock_fd, uint8_t *data, size_t len)
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
        uint8_t reply[9];
        reply[0] = data[0];
        reply[1] = data[1];
        reply[2] = 0x00;
        reply[3] = 0x00;
        reply[4] = 0x00;
        reply[5] = 0x03;
        reply[6] = query->unit_id;
        reply[7] = data[7] + 0x80;
        reply[8] = 0x0B;

        BaseTcpServer::send(sock_fd, reply, sizeof(reply));
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

    switch (query->function_code)
    {
    case READ_COILS:
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
    case READ_HOLDING_REGISTERS:
        reply->byte_count = (uint8_t)(sizeof(uint16_t) * query->word_count);
        for (size_t i = 0; i < reply->byte_count / sizeof(uint16_t); i++)
        {
            if (query->start_addr + i < 100) // TODO reply invalid data address in such case
            {
                offset[i] = htons(holdingRegisters[(query->start_addr) + i]);
            }
        }
        break;

    case READ_INPUT_REGISTERS:
        reply->byte_count = (uint8_t)(sizeof(uint16_t) * query->word_count);
        for (size_t i = 0; i < reply->byte_count / sizeof(uint16_t); i++)
        {
            if (query->start_addr + i < 100) // TODO reply invalid data address in such case
            {
                offset[i] = htons(inputRegisters[(query->start_addr) + i]);
            }
        }
        break;

    case WRITE_SINGLE_COIL:
        memcpy((void *)send_buf, (void *)data, 12);
        tmp_bytes = 3;
        break;

    case WRITE_SINGLE_REGISTER:
        holdingRegisters[query->start_addr] = htons(registers[0]);
        memcpy((void *)send_buf, (void *)data, 12);
        reply->byte_count = 3;
        break;

    case WRITE_MULTIPLE_COILS:
        // TODO
        break;

    case WRITE_MULTIPLE_REGISTERS:
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
        }

        memcpy((void *)send_buf, (void *)data, 12);
        reply->byte_count = 3;
        break;
    }

    default:
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