/* KallistiOS ##version##

   mie_fw.c
   Copyright (C) 2026 Ruslan Rostovtsev

   Z80 firmware lifecycle: self-test, code upload to bridge RAM, JVS bus
   reset/setaddr, and verification poll. EEPROM access uses a separate
   IOR command sequence (REQUEST/FETCH/WRITE).
*/

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <dc/maple.h>
#include <dc/maple/mie.h>

#include "mie_internal.h"

#include <kos/dbglog.h>
#include <kos/fs.h>
#include <kos/thread.h>

#define MIE_CMD_IO       0x86
#define MIE_RESP_IO      0x87

#define MIE_CMD_UPLOAD_FW  0x80
#define MIE_RESP_UPLOAD_FW 0x81
#define MIE_CMD_SELF_TEST  0x84
#define MIE_RESP_SELF_TEST 0x85

#define MIE_IOR_EEPROM_REQUEST 0x01
#define MIE_IOR_EEPROM_ACK     0x02
#define MIE_IOR_EEPROM_FETCH   0x03
#define MIE_IOR_EEPROM_WRITE   0x0B

#define MIE_IOR_JVS_EMPTY       0x32

#define MIE_JVS_FETCH_MIN_WORDS 8

#define MIE_EEPROM_CHUNK 16

#define MIE_Z80_RAM_BASE            0x8010
#define MIE_CODE_CHUNK              24
#define MIE_CODE_MAX                65536
#define MIE_RESP_UPLOAD_BOOTROM     0x80
#define MIE_UPLOAD_CHUNK_RETRIES    8
#define MIE_UPLOAD_RETRY_DELAY_MS   5
#define MIE_IO_RETRY_ATTEMPTS       8
#define MIE_IO_RETRY_DELAY_MS       50

static maple_response_t *mie_io_retry(const void *req, int len);

/* Bridge must respond before code upload or EEPROM access. */
static bool mie_self_test(void) {
    maple_response_t *resp;
    int i;

    for(i = 0; i < 5; i++) {
        resp = mie_cmd_resp(MIE_CMD_SELF_TEST, NULL, 0);
        if(resp && (uint8_t)resp->response == MIE_RESP_SELF_TEST) {
            break;
        }
        thd_sleep(5);
    }

    if(!resp || (uint8_t)resp->response != MIE_RESP_SELF_TEST) {
        dbglog(DBG_ERROR, "mie: self test no resp\n");
        return false;
    }

    if(resp->data_len < 1) {
        dbglog(DBG_ERROR, "mie: self test no payload\n");
        return false;
    }

    return true;
}

/* Maple-level reset before uploading new Z80 image into bridge RAM. */
static bool mie_maple_dev_reset(void) {
    maple_response_t *resp;
    int i;

    for(i = 0; i < 40; i++) {
        mie_poll_idle();
        resp = mie_cmd_resp(MAPLE_COMMAND_RESET, NULL, 0);
        if(resp && resp->response == MAPLE_RESPONSE_OK) {
            thd_sleep(30);
            return mie_poll_idle();
        }
        thd_sleep(20);
    }

    dbglog(DBG_ERROR, "mie: maple dev reset failed\n");
    return false;
}

static maple_response_t *mie_io_retry(const void *req, int len) {
    maple_response_t *resp = NULL;
    int attempt;

    for(attempt = 0; attempt < MIE_IO_RETRY_ATTEMPTS; attempt++) {
        if(attempt > 0) {
            mie_poll_idle();
            thd_sleep(MIE_IO_RETRY_DELAY_MS);
        }

        resp = mie_cmd_resp(MIE_CMD_IO, (void *)req, len);
        if(!resp || resp->response == MAPLE_RESPONSE_AGAIN) {
            continue;
        }

        if((uint8_t)resp->response == MIE_RESP_IO) {
            return resp;
        }
    }

    return NULL;
}

uint16_t mie_eeprom_crc16(const uint8_t *buf, size_t size) {
    uint32_t n = 0xdebdeb00;
    size_t i;
    int bit;

    assert(buf);

    for(i = 0; i < size; i++) {
        n &= 0xffffff00;
        n += buf[i];

        for(bit = 0; bit < 8; bit++) {
            if(n & 0x80000000) {
                n = (n << 1) + 0x10210000;
            }
            else {
                n <<= 1;
            }
        }
    }

    for(bit = 0; bit < 8; bit++) {
        if(n & 0x80000000) {
            n = (n << 1) + 0x10210000;
        }
        else {
            n <<= 1;
        }
    }

    return (uint16_t)(n >> 16);
}

/* EEPROM stores two CRC copies per section (header at 0/18 and 36/40). */
void mie_eeprom_fix_crc(uint8_t *eeprom) {
    uint16_t crc;
    uint8_t size;

    assert(eeprom);

    crc = mie_eeprom_crc16(eeprom + 2, 16);
    eeprom[0] = (uint8_t)(crc & 0xff);
    eeprom[1] = (uint8_t)(crc >> 8);
    eeprom[18] = eeprom[0];
    eeprom[19] = eeprom[1];

    size = eeprom[39];
    if(size == 0 || (uint32_t)44 + size > MIE_EEPROM_SIZE) {
        return;
    }

    crc = mie_eeprom_crc16(eeprom + 44, size);
    eeprom[36] = (uint8_t)(crc & 0xff);
    eeprom[37] = (uint8_t)(crc >> 8);
    eeprom[40] = eeprom[36];
    eeprom[41] = eeprom[37];
}

/* EEPROM read: REQUEST -> idle -> FETCH (128 bytes). */
bool mie_get_eeprom(void *dst) {
    uint8_t ioreq[4];
    maple_response_t *resp;

    assert(dst);

    mie_bus_acquire_scoped(bus);
    if(!bus) {
        return false;
    }

    if(!mie_poll_idle()) {
        dbglog(DBG_ERROR, "mie: eeprom pre poll timeout\n");
        return false;
    }

    memset(ioreq, 0, sizeof(ioreq));
    ioreq[0] = MIE_IOR_EEPROM_REQUEST;

    resp = mie_io_retry(ioreq, 1);
    if(!resp) {
        dbglog(DBG_ERROR, "mie: eeprom req failed\n");
        return false;
    }

    if(resp->data_len >= 1 && resp->data[0] != MIE_IOR_EEPROM_ACK) {
        dbglog(DBG_ERROR, "mie: eeprom req ack %02x\n", resp->data[0]);
        return false;
    }

    if(!mie_poll_idle()) {
        dbglog(DBG_ERROR, "mie: eeprom read poll timeout\n");
        return false;
    }

    ioreq[0] = MIE_IOR_EEPROM_FETCH;

    resp = mie_io_retry(ioreq, 1);
    if(!resp) {
        dbglog(DBG_ERROR, "mie: eeprom fetch failed\n");
        return false;
    }

    if(resp->data_len != 0x20) {
        dbglog(DBG_ERROR, "mie: eeprom fetch len %02x\n", resp->data_len);
        return false;
    }

    memcpy(dst, resp->data, MIE_EEPROM_SIZE);
    return true;
}

/* Write 128-byte EEPROM in 16-byte chunks with settle delay between. */
bool mie_set_eeprom(uint8_t *eeprom) {
    uint8_t ioreq[20];
    maple_response_t *resp;
    unsigned int offset;

    assert(eeprom);

    mie_bus_acquire_scoped(bus);
    if(!bus) {
        return false;
    }

    if(!mie_poll_idle()) {
        return false;
    }

    for(offset = 0; offset < MIE_EEPROM_SIZE; offset += MIE_EEPROM_CHUNK) {
        memset(ioreq, 0, sizeof(ioreq));
        ioreq[0] = MIE_IOR_EEPROM_WRITE;
        ioreq[1] = (uint8_t)offset;
        ioreq[2] = MIE_EEPROM_CHUNK;
        memcpy(ioreq + 4, eeprom + offset, MIE_EEPROM_CHUNK);

        resp = mie_io_retry(ioreq, 5);
        if(!resp) {
            dbglog(DBG_ERROR, "mie: eeprom write failed at %02x\n", offset);
            return false;
        }

        if(!mie_poll_idle()) {
            dbglog(DBG_ERROR, "mie: eeprom write poll timeout at %02x\n", offset);
            return false;
        }

        thd_sleep(50);
    }

    return true;
}

static bool mie_upload_code_chunk_ack(maple_response_t *resp, uint16_t addr,
                                      uint8_t checksum) {
    uint32_t ack;
    uint16_t resp_addr;

    if(!resp || ((uint8_t)resp->response != MIE_RESP_UPLOAD_FW &&
            (uint8_t)resp->response != MIE_RESP_UPLOAD_BOOTROM)) {
        return false;
    }

    if(resp->data_len != 1) {
        return false;
    }

    ack = *(uint32_t *)resp->data;
    resp_addr = (uint16_t)(((ack & 0xff0000) >> 8) | ((ack & 0xff000000) >> 24));
    if(resp_addr != addr) {
        return false;
    }

    return (ack & 0xff) == checksum;
}

static bool mie_upload_code_chunk(uint16_t addr, const uint8_t *data,
                                       int chunk_len) {
    uint8_t upload[28];
    maple_response_t *resp;
    uint8_t checksum;
    int attempt;
    int i;

    memset(upload, 0, sizeof(upload));
    upload[2] = (uint8_t)(addr >> 8);
    upload[3] = (uint8_t)(addr & 0xff);
    memcpy(upload + 4, data, chunk_len);

    checksum = 0;
    for(i = 0; i < 28; i++)
        checksum = (checksum + upload[i]) & 0xff;

    resp = NULL;
    for(attempt = 0; attempt < MIE_UPLOAD_CHUNK_RETRIES; attempt++) {
        if(attempt > 0) {
            thd_sleep(MIE_UPLOAD_RETRY_DELAY_MS);
        }

        resp = mie_cmd_resp(MIE_CMD_UPLOAD_FW, upload, 28 / 4);
        if(!resp) {
            continue;
        }

        if(resp->response == MAPLE_RESPONSE_AGAIN ||
                resp->response == MAPLE_RESPONSE_NONE) {
            continue;
        }

        if(mie_upload_code_chunk_ack(resp, addr, checksum)) {
            return true;
        }

        resp = NULL;
    }

    if(!resp) {
        dbglog(DBG_ERROR, "mie: code upload timeout %04x\n", addr);
    }
    else {
        dbglog(DBG_ERROR, "mie: code upload bad ack %04x\n", addr);
    }

    return false;
}

static bool mie_upload_code_range(uint16_t memloc, const uint8_t *bin,
                                       size_t len, bool poll_first) {
    size_t remain = len;
    size_t chunk;
    const uint8_t *binloc = bin;

    if(poll_first && !mie_poll_idle()) {
        return false;
    }

    while(remain > 0) {
        chunk = remain > MIE_CODE_CHUNK ? MIE_CODE_CHUNK : remain;

        if(!mie_upload_code_chunk(memloc, binloc, (int)chunk)) {
            return false;
        }

        binloc += chunk;
        memloc = (uint16_t)(memloc + chunk);
        remain -= chunk;
    }

    return true;
}

/* 0xFFFFFFFF payload tells the bridge to jump to uploaded code in RAM. */
static bool mie_upload_code_exec(void) {
    uint8_t execdata[4] = { 0xff, 0xff, 0xff, 0xff };
    maple_response_t *resp;
    int attempt;

    for(attempt = 0; attempt < MIE_UPLOAD_CHUNK_RETRIES; attempt++) {
        if(attempt > 0) {
            thd_sleep(MIE_UPLOAD_RETRY_DELAY_MS);
        }

        resp = mie_cmd_resp(MIE_CMD_UPLOAD_FW, execdata, 1);
        if(!resp) {
            continue;
        }

        if(resp->response == MAPLE_RESPONSE_AGAIN ||
                resp->response == MAPLE_RESPONSE_NONE) {
            continue;
        }

        if((uint8_t)resp->response == MAPLE_RESPONSE_OK) {
            return true;
        }
    }

    dbglog(DBG_ERROR, "mie: code exec failed\n");
    return false;
}

/* Reset bridge, upload Z80 image to RAM in 24-byte chunks, then jump. */
static bool mie_upload_code(const uint8_t *bin, size_t len) {
    mie_maple_dev_reset();
    mie_self_test();

    if(!mie_upload_code_range(MIE_Z80_RAM_BASE, bin, len, true)) {
        return false;
    }

    return mie_upload_code_exec();
}

static bool mie_upload_code_file(const char *path) {
    file_t fd;
    ssize_t size;
    ssize_t rd;
    uint8_t *buf;
    bool ok = false;

    fd = fs_open(path, O_RDONLY);
    if(fd == FILEHND_INVALID) {
        return false;
    }

    size = fs_total(fd);
    if(size <= 0 || size > MIE_CODE_MAX) {
        goto out;
    }

    buf = aligned_alloc(32, size);
    if(!buf) {
        goto out;
    }

    rd = fs_read(fd, buf, size);
    if(rd != size) {
        goto out_free;
    }

    ok = mie_upload_code(buf, size);

out_free:
    free(buf);
out:
    fs_close(fd);
    return ok;
}

/* Broadcast JVS reset, then setaddr (retried until Maple ACK). */
static bool mie_z80_jvs_reset(void) {
    uint8_t reset_buf[12];
    uint8_t setaddr_buf[12];
    int i;

    mie_build_jvs_reset(reset_buf);
    if(!mie_jvs_io_cmd(reset_buf, 3)) {
        return false;
    }

    mie_build_jvs_setaddr(setaddr_buf);
    for(i = 0; i < 50; i++) {
        thd_sleep(20);

        if(mie_jvs_io_cmd(setaddr_buf, 3)) {
            return true;
        }
    }

    return false;
}

static bool mie_fetch_has_func_check(maple_response_t *resp) {
    const uint8_t *data;
    int len;

    assert(resp);

    if(resp->data_len < MIE_JVS_FETCH_MIN_WORDS) {
        return false;
    }

    if(((uint8_t *)resp->data)[0] == MIE_IOR_JVS_EMPTY) {
        return false;
    }

    data = resp->data;
    len = (int)resp->data_len * 4;
    return mie_ior_jvs_report_ready(data, len);
}

bool mie_jvs_function_check(void) {
    uint8_t req[12];
    maple_response_t *resp;
    mie_jvs_poll_cfg_t cfg;
    const uint8_t *data;
    int len;

    mie_build_jvs_func_check(req);
    if(!mie_jvs_io_cmd(req, 3)) {
        return false;
    }

    if(!mie_poll_idle()) {
        return false;
    }

    resp = mie_jvs_fetch_retry(mie_fetch_has_func_check);
    if(!resp) {
        return false;
    }

    data = resp->data;
    len = (int)resp->data_len * 4;
    if(!mie_parse_jvs_func_check(data, len, &cfg)) {
        return false;
    }

    mie_jvs_poll_cfg_apply(&cfg);
    return true;
}

/* Probe live firmware; on failure JVS-reset and probe again. */
bool mie_z80_try_active(maple_device_t *dev) {
    (void)dev;

    if(mie_jvs_function_check()) {
        return true;
    }

    if(!mie_z80_jvs_reset()) {
        return false;
    }

    return mie_jvs_function_check();
}

/* Try running firmware first; upload from file and JVS-reset only if needed. */
bool mie_z80_init(const char *fw_path, maple_device_t *dev) {
    assert(dev);

    mie_poll_idle();

    if(mie_z80_try_active(dev)) {
        return true;
    }

    if(!mie_upload_code_file(fw_path)) {
        dbglog(DBG_ERROR, "mie: fw upload failed\n");
        return false;
    }

    mie_poll_idle();
    thd_sleep(30);

    if(!mie_z80_jvs_reset()) {
        dbglog(DBG_ERROR, "mie: jvs reset failed after upload firmware\n");
        return false;
    }

    if(!mie_jvs_function_check()) {
        dbglog(DBG_ERROR, "mie: JVS I/O board is not responding\n");
        return false;
    }

    return true;
}
