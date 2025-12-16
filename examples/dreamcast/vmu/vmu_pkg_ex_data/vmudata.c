#include <assert.h>
#include <errno.h>
#include <malloc.h>
#include <string.h>

#include <arch/arch.h>
#include <dc/biosfont.h>
#include <dc/fs_vmu.h>
#include <dc/maple.h>
#include <dc/maple/controller.h>
#include <dc/video.h>
#include <dc/vmu_pkg.h>
#include <kos/dbglog.h>
#include <kos/fs.h>
#include <kos/init.h>
#include <kos/thread.h>


// no stdio here, except for this one
extern int snprintf(char* s, size_t n, const char* format, ...);


static uint8_t* RAW_VMS_HEADER;
static size_t RAW_VMS_HEADER_SIZE;
static char VMU_PATH[] = "/vmu/a0/SMPLTXTSD";

static const char test_value[] = "Lorem ipsum dolor sit amet, consectetur adipiscing elit.";
static const size_t test_value_size = sizeof(test_value);

static const int8_t port = 0;
static const int8_t unit = 1;


static bool load_vms_from_romdisk() {
    file_t vms_file = fs_open("/rd/SMPLTXTSD.VMS", O_RDONLY);
    if (vms_file == FILEHND_INVALID) {
        dbglog(DBG_ERROR, "Missing file SMPLTXTSD.VMS from romdisk\n");
        return true;
    }

    RAW_VMS_HEADER_SIZE = fs_total(vms_file);
    RAW_VMS_HEADER = malloc(RAW_VMS_HEADER_SIZE);
    assert(RAW_VMS_HEADER);

    fs_read(vms_file, RAW_VMS_HEADER, RAW_VMS_HEADER_SIZE);
    fs_close(vms_file);
    return false;
}

static const char* get_vmu_path() {
    char port_name = (char){0x61 + port};
    char slot_name = (char){0x30 + unit};

    // Important: the vms filename can not be longer than 12 bytes
    VMU_PATH[5] = port_name;
    VMU_PATH[6] = slot_name;

    return VMU_PATH;
}

static uint32_t calc_crc16(const uint8_t* buffer, const size_t size, uint32_t crc) {
    for (size_t i = 0; i < size; i++) {
        crc ^= (buffer[i] << 8);

        for (size_t c = 0; c < 8; c++) {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 4129;
            else
                crc = (crc << 1);
        }
    }

    return crc;
}


static void print_string_to_fb(int x, int y, const char* str) {
    bfont_draw_str(
        &vram_s[x + (y * vid_mode->width)],
        vid_mode->width,
        1,
        str
    );
}

static void print_status(const char* str) {
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "STATUS: %s", str);

    print_string_to_fb(32, 384, "                                                    ");
    print_string_to_fb(32, 384, buffer);

    dbglog(DBG_INFO, "%s\n", buffer);
}

static void check_header_status(const char* caller, file_t vms) {
    const char* header_status;
    switch (fs_vmu_get_header_status(vms)) {
        case VMUHDR_STATUS_OK:
            header_status = "VMUHDR_STATUS_OK";
            break;
        case VMUHDR_STATUS_BADCRC:
            header_status = "VMUHDR_STATUS_BADCRC";
            break;
        case VMUHDR_STATUS_NEWFILE:
            header_status = "VMUHDR_STATUS_NEWFILE";
            break;
        default:
            header_status = "<unknown status>";
            break;
    }
    dbglog(DBG_INFO, "%s() header status: %s\n", caller, header_status);
}


static void raw_write_test(bool with_bad_crc) {
    print_status(with_bad_crc ? "Writing with bad CRC..." : "Writing...");

    vmu_hdr_t* header = (vmu_hdr_t*)RAW_VMS_HEADER;
    header->data_len = test_value_size;
    header->crc = 0x00;

    uint32_t crc16 = calc_crc16(RAW_VMS_HEADER, RAW_VMS_HEADER_SIZE, 0x0000);
    crc16 = calc_crc16((const uint8_t*)test_value, test_value_size, crc16);
    header->crc = crc16 & 0xffff;

    if (with_bad_crc) {
        header->crc = 0x0000;
    }

    //
    // Notes:
    //  * Write test is RAW, there is no header management performed by KallistiOS.
    //  * fs_write() not always write all bytes, but should be guaranteed for VMU.
    //

    file_t vms = fs_open(get_vmu_path(), O_WRONLY | O_TRUNC | O_META);
    if (vms == FILEHND_INVALID) {
        print_status("File opening failed");
        dbglog(DBG_ERROR, "%s() call to fs_open() failed. errno=0x%x\n", __FUNCTION__, errno);
        return;
    }

    check_header_status(__FUNCTION__, vms);

    ssize_t written = fs_write(vms, RAW_VMS_HEADER, RAW_VMS_HEADER_SIZE);
    assert(written == RAW_VMS_HEADER_SIZE);

    written = fs_write(vms, test_value, test_value_size);
    assert(written == test_value_size);

    dbglog(
        DBG_INFO, "%s fs_total=%i fs_vmu_get_file_size=%i\n",
        __FUNCTION__, (int)fs_total(vms), fs_vmu_get_file_size(vms)
    );

    bool ret = fs_close(vms) == 0;

    print_status(ret ? "Write success" : "Write failed");
}

static void pkg_write_test(bool using_default_header) {
    print_status(using_default_header ? "Writing with default..." : "Writing...");

    // set data_len and crc otherwise vmu_pkg_parse_ex() will fail
    vmu_pkg_crc_set(RAW_VMS_HEADER, 0, 0);

    vmu_pkg_t pkg = {0};
    assert(vmu_pkg_parse_ex(RAW_VMS_HEADER, RAW_VMS_HEADER_SIZE, &pkg, false, false) == 0);

    //
    // Notes:
    //  * fs_write() not always write all bytes, but should be guaranteed for VMU.
    //

    file_t vms = fs_open(get_vmu_path(), O_WRONLY);
    if (vms == FILEHND_INVALID) {
        print_status("File creation failed");
        dbglog(DBG_ERROR, "%s() call to fs_open() failed. errno=0x%x\n", __FUNCTION__, errno);
        return;
    }

    check_header_status(__FUNCTION__, vms);

    ssize_t written = fs_write(vms, test_value, test_value_size);
    assert(written == test_value_size);

    // set header
    if (using_default_header) {
        assert(fs_vmu_set_default_header(&pkg) == 0);
    } else {
        assert(fs_vmu_set_header(vms, &pkg) == 0);
    }

    dbglog(
        DBG_INFO, "%s fs_total=%i fs_vmu_get_file_size=%i\n",
        __FUNCTION__, (int)fs_total(vms), fs_vmu_get_file_size(vms)
    );

    bool ret = fs_close(vms) == 0;

    if (using_default_header) {
        // remove default to finalize the test
        assert(fs_vmu_set_default_header(NULL) == 0);
    }

    if (using_default_header)
        print_status(ret ? "Write with default success" : "Write with default failed");
    else
        print_status(ret ? "Write success" : "Write failed");
}

static void read_test() {
    const char* vms_path = get_vmu_path();

    //
    // Notes:
    //  * Header parse and checksum is performed by KallistiOS.
    //  * fs_read() not always read all bytes, but should be guaranteed for VMU.
    //

    errno = 0;
    file_t vms = fs_open(vms_path, O_RDONLY);
    if (vms == FILEHND_INVALID) {
        print_status("\xBF"
                     "File not found?");
        dbglog(DBG_ERROR, "%s() call to fs_open() failed. errno=0x%x\n", __FUNCTION__, errno);
        return;
    }

    check_header_status(__FUNCTION__, vms);

    uint8_t value[test_value_size];

    ssize_t readed = fs_read(vms, value, test_value_size);
    if (readed < 0) {
        print_status("Read failed");
        fs_close(vms);
        return;
    }

    dbglog(
        DBG_INFO, "%s fs_total=%i fs_vmu_get_file_size=%i\n",
        __FUNCTION__, (int)fs_total(vms), fs_vmu_get_file_size(vms)
    );

    if (readed == test_value_size) {
        bool equals = memcmp(value, test_value, test_value_size) == 0;
        print_status(equals ? "Read success" : "Read missmatch");
    } else {
        print_status("File size missmatch");
    }

    errno = 0;
    int ret = fs_close(vms);
    if (ret != 0) {
        dbglog(DBG_ERROR, "%s() call to fs_close() failed. return=%i errno=0x%x\n", __FUNCTION__, ret, errno);
    }
}

static void delete_test() {
    print_status("Deleting...");
    const char* vms_path = get_vmu_path();
    int ret = fs_unlink(vms_path);

    print_status(ret == 0 ? "Delete success" : "File not found");
}


int main(int argc, char* argv[]) {
    bool menu_common = true;

    dbglog_set_level(DBG_INFO);

    if (load_vms_from_romdisk()) {
        arch_menu();
        __unreachable();
        return 1;
    }

    print_string_to_fb(0, 0, "VMS data file example by kapodamy");
    print_string_to_fb(48, BFONT_HEIGHT * 2, "Filename: SMPLTXTSD    Filetype: DATA");

L_print_menu:
    if (menu_common) {
        print_string_to_fb(
            0, 128,
            "Press:\n"
            "[Y]  (raw) write test file                          \n"
            "[X]  (raw) write test file with bad CRC             \n"
            "[B]  to read and check contents                     \n"
            "[A]  <more tests>                                   \n"
            "[START] return to BIOS"
        );
    } else {
        print_string_to_fb(
            0, 128,
            "Press:\n"
            "[Y]  (vmu_pkg) write test file                     \n"
            "[X]  (vmu_pkg) write test file with default header \n"
            "[B]  delete test file                              \n"
            "[A]  <common tests>                                \n"
            "[START] return to BIOS"
        );
    }

    print_status("Waiting user action...");
    thd_sleep(500);

    while (true) {
        maple_device_t* cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);

        if (!cont) {
            print_status("No controller connected");
            while (!(cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER)));
            print_status("Waiting user action...");
        }

        cont_state_t* state = (cont_state_t*)maple_dev_status(cont);
        if (!state) continue;

        if (state->buttons & CONT_Y) {
            if (menu_common) {
                raw_write_test(false);
            } else {
                pkg_write_test(false);
            }
        } else if (state->buttons & CONT_X) {
            if (menu_common) {
                raw_write_test(true);
            } else {
                pkg_write_test(true);
            }
        } else if (state->buttons & CONT_B) {
            if (menu_common) {
                read_test();
            } else {
                delete_test();
            }
        } else if (state->buttons & CONT_A) {
            menu_common = !menu_common;
            goto L_print_menu;
        } else if (state->buttons & CONT_START) {
            dbglog(DBG_INFO, "returning to BIOS...\n");
            break;
        } else {
            // nothing to do
            continue;
        }

        // buttons antibounce
        state->buttons = 0x00;
        thd_sleep(500);
    }

    arch_menu();
    return 0;
}
