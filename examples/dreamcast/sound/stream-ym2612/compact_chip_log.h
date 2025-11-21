#pragma once

#include <stdint.h>


typedef enum __attribute__((packed)) {
    CCL_CMD_end_of_file = 0,
    CCL_CMD_stream_update = 1,
    CCL_CMD_stream_update_nibble = 2,
    CCL_CMD_stream_update_wide = 3,
    CCL_CMD_port_write = 4,
    CCL_CMD_port_write_pack_0_1 = 5,
    CCL_CMD_port_write_pack_2_3 = 6,
    CCL_CMD_port_write_pack_0_1_2_3 = 7,
} CCL_Command;


typedef struct __attribute__((packed)) {
    CCL_Command type : 4;
} CCL_PacketPeek;

typedef struct __attribute__((packed)) {
    CCL_Command type : 4;
    union {
        uint8_t stream_updates;
        uint8_t port_writes;
        uint8_t address_value;
    };
} CCL_Packet;


typedef struct __attribute__((packed)) {
    CCL_Command type : 4;
    uint16_t stream_updates;
} CCL_PacketStreamUpdateWide;

typedef struct __attribute__((packed)) {
    CCL_Command type : 4;
    uint8_t stream_updates: 4;
} CCL_PacketStreamUpdateNibble;


typedef struct __attribute__((packed)) {
    uint8_t offset;
    uint8_t data;
} CCL_PacketPortWrite;

typedef struct __attribute__((packed)) {
    CCL_Command type : 4;
    uint8_t data[2];
} CCL_PacketPortPack_0_1;

typedef struct __attribute__((packed)) {
    CCL_Command type : 4;
    uint8_t data[2];
} CCL_PacketPortPack_2_3;

typedef struct __attribute__((packed)) {
    CCL_Command type : 4;
    uint8_t data[4];
} CCL_PacketPortPack_0_1_2_3;


typedef struct __attribute__((packed)) {
    char signature[4];
    uint8_t version;
    uint8_t reserved[2];
    char chip_identifier[8];
    uint32_t chip_clock;
    uint8_t chip_flags;
    uint32_t sample_rate;
    uint32_t pcm_samples;
    uint32_t loop_offset;
    uint32_t loop_start_samples;
} CCL_FileHeader;



int ccl_file_initialize(const char* filename, const char* target_chip);
void ccl_set_clock(int clock);
void ccl_set_rate(int rate);
void ccl_set_flags(uint8_t flags);
void ccl_stream_update(int count);
void ccl_port_write(uint32_t offset, uint8_t data);
void ccl_loop_mark();
void ccl_file_finalize();
