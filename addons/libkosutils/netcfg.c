/* KallistiOS ##version##

   netcfg.c
   Copyright (C)2003,2004 Megan Potter

*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <kos/fs.h>
#include <kos/netcfg.h>
#include <dc/flashrom.h>
#include <dc/vmu_pkg.h>

#include "netcfg_icon.h"

#if 0
#define dbgp printf
#else
#define dbgp(...)
#endif

void netcfg_vmuify(const char *filename_in, const char *filename_out) {
    int fd, pkg_size;
    uint8_t  *buf;
    uint8_t  *pkg_out;
    vmu_pkg_t pkg;

    dbgp("Opening source file\n");
    fd = fs_open(filename_in, O_RDONLY);
    buf = (uint8_t *) malloc(fs_total(fd));
    if(buf == NULL)
        return;

    fs_read(fd, buf, fs_total(fd));
    dbgp("Read %i bytes\n", fs_total(fd));

    strcpy(pkg.desc_short, "KallistiOS 2.1.0");
    strcpy(pkg.desc_long, "KOS Network Settings");
    strcpy(pkg.app_id, "KOS");
    pkg.icon_cnt = 1;
    pkg.icon_anim_speed = 1;
    memcpy(&pkg.icon_pal[0], netcfg_icon, 32);
    pkg.icon_data = (uint8_t *)(netcfg_icon + 32);
    pkg.eyecatch_type = VMUPKG_EC_NONE;
    pkg.data_len = fs_total(fd);
    pkg.data = buf;
    dbgp("Building package\n");
    vmu_pkg_build(&pkg, &pkg_out, &pkg_size);
    fs_close(fd);
    dbgp("Closing source file\n");

    dbgp("Opening output file\n");
    fd = fs_open(filename_out, O_WRONLY);
    dbgp("Writing..\n");
    fs_write(fd, pkg_out, pkg_size);
    dbgp("Closing output file\n");
    fs_close(fd);
    free(buf);
    dbgp("VMUification complete\n");
}

/* This module attempts to ferret out a valid network configuration by
   drawing on all available sources. A file stored on the VMU is tried
   first, followed by reading the settings from the DC's flashrom. If
   this fails, we try reading settings from the current dir in the VFS,
   and then from the root of the CD. If that fails, we give up. */

int netcfg_load_from(const char *fn, netcfg_t *out) {
    FILE * f;
    char buf[64], *b;
    int l;

    assert(out);

    /* Open the file */
    f = fopen(fn, "rb");

    if(!f)
        return -1;

    /* If we're reading from a VMU, seek past the header
       In the future, we could read this and use data_len to
       know how big the file really is */
    if(fn[0] == '/' && fn[1] == 'v' && fn[2] == 'm' && fn[3] == 'u') {
        /* Make sure there's a VMU header to skip. If so, skip it. */
        fread(buf, 4, 1, f);
        buf[4] = 0;

        if(strcmp(buf, "# KO"))
            fseek(f, 128 + 512, SEEK_SET);
    }

    /* Read each line... */
    while(fgets(buf, 64, f)) {
        /* Skip comments and blank lines */
        if(buf[0] == 0 || buf[0] == '#')
            continue;

        /* Strip newlines */
        l = strlen(buf);

        if(buf[l - 1] == '\n') {
            buf[l - 1] = 0;
            l--;
        }

        if(buf[l - 1] == '\r') {
            buf[l - 1] = 0;
            l--;
        }

        /* Look for an equals */
        b = strchr(buf, '=');

        if(!b)
            continue;

        *b = 0;
        b++;

        /* What was the line type? */
        if(!strcmp(buf, "driver")) {
            strcpy(out->driver, b);
        }
        else if(!strcmp(buf, "ip")) {
            out->ip = strtoul(b, NULL, 16);
        }
        else if(!strcmp(buf, "gateway")) {
            out->gateway = strtoul(b, NULL, 16);
        }
        else if(!strcmp(buf, "netmask")) {
            out->netmask = strtoul(b, NULL, 16);
        }
        else if(!strcmp(buf, "broadcast")) {
            out->broadcast = strtoul(b, NULL, 16);
        }
        else if(!strcmp(buf, "dns1")) {
            out->dns[0] = strtoul(b, NULL, 16);
        }
        else if(!strcmp(buf, "dns2")) {
            out->dns[1] = strtoul(b, NULL, 16);
        }
        else if(!strcmp(buf, "hostname")) {
            strcpy(out->hostname, b);
        }
        else if(!strcmp(buf, "email")) {
            strcpy(out->email, b);
        }
        else if(!strcmp(buf, "smtp")) {
            strcpy(out->smtp, b);
        }
        else if(!strcmp(buf, "pop3")) {
            strcpy(out->pop3, b);
        }
        else if(!strcmp(buf, "pop3_login")) {
            strcpy(out->pop3_login, b);
        }
        else if(!strcmp(buf, "pop3_passwd")) {
            strcpy(out->pop3_passwd, b);
        }
        else if(!strcmp(buf, "proxy_host")) {
            strcpy(out->proxy_host, b);
        }
        else if(!strcmp(buf, "proxy_port")) {
            out->proxy_port = strtoul(b, NULL, 10);
        }
        else if(!strcmp(buf, "ppp_login")) {
            strcpy(out->ppp_login, b);
        }
        else if(!strcmp(buf, "ppp_passwd")) {
            strcpy(out->ppp_passwd, b);
        }
        else if(!strcmp(buf, "method")) {
            if(!strcmp(b, "dhcp"))
                out->method = NETCFG_METHOD_DHCP;
            else if(!strcmp(b, "static"))
                out->method = NETCFG_METHOD_STATIC;
            else if(!strcmp(b, "pppoe"))
                out->method = NETCFG_METHOD_PPPOE;
        }
    }

    fclose(f);

    return 0;
}

int netcfg_load_flash(netcfg_t *out) {
    flashrom_ispcfg_t cfg;

    if(flashrom_get_ispcfg(&cfg) < 0)
        return -1;

    /* Start out with a clean config */
    memset(out, 0, sizeof(netcfg_t));

#define READIP(dst, src) \
    dst = ((src[0]) << 24) | \
          ((src[1]) << 16) | \
          ((src[2]) << 8) | \
          ((src[3]) << 0)

    if(cfg.valid_fields & FLASHROM_ISP_IP)
        READIP(out->ip, cfg.ip);

    if(cfg.valid_fields & FLASHROM_ISP_GATEWAY)
        READIP(out->gateway, cfg.gw);

    if(cfg.valid_fields & FLASHROM_ISP_NETMASK)
        READIP(out->netmask, cfg.nm);

    if(cfg.valid_fields & FLASHROM_ISP_BROADCAST)
        READIP(out->broadcast, cfg.bc);

    if(cfg.valid_fields & FLASHROM_ISP_DNS) {
        READIP(out->dns[0], cfg.dns[0]);
        READIP(out->dns[1], cfg.dns[1]);
    }

    if(cfg.valid_fields & FLASHROM_ISP_HOSTNAME)
        strcpy(out->hostname, cfg.hostname);

    if(cfg.valid_fields & FLASHROM_ISP_EMAIL)
        strcpy(out->email, cfg.email);

    if(cfg.valid_fields & FLASHROM_ISP_SMTP)
        strcpy(out->smtp, cfg.smtp);

    if(cfg.valid_fields & FLASHROM_ISP_POP3)
        strcpy(out->pop3, cfg.pop3);

    if(cfg.valid_fields & FLASHROM_ISP_POP3_USER)
        strcpy(out->pop3_login, cfg.pop3_login);

    if(cfg.valid_fields & FLASHROM_ISP_POP3_PASS)
        strcpy(out->pop3_passwd, cfg.pop3_passwd);

    if(cfg.valid_fields & FLASHROM_ISP_PROXY_PORT)
        out->proxy_port = cfg.proxy_port;

    if(cfg.valid_fields & FLASHROM_ISP_PROXY_HOST)
        strcpy(out->proxy_host, cfg.proxy_host);

    if(cfg.valid_fields & FLASHROM_ISP_PPP_USER)
        strcpy(out->ppp_login, cfg.ppp_login);

    if(cfg.valid_fields & FLASHROM_ISP_PPP_PASS)
        strcpy(out->ppp_passwd, cfg.ppp_passwd);

#undef READIP

    return 0;
}

int netcfg_load(netcfg_t *out) {
    file_t f;
    dirent_t * d;
    char buf[64];

    /* Scan for VMUs */
    f = fs_open("/vmu", O_RDONLY | O_DIR);

    if(f >= 0) {
        for(; ;) {
            d = fs_readdir(f);

            if(!d) {
                fs_close(f);
                break;
            }

            /* Valid names are a1, a2, b1, etc */
            sprintf(buf, "/vmu/%.*s/net.cfg", 2, d->name);

            if(netcfg_load_from(buf, out) >= 0) {
                out->src = NETCFG_SRC_VMU;
                fs_close(f);
                return 0;
            }
        }
    }

    /* Couldn't find anything. Try reading the config from flash */
    if(netcfg_load_flash(out) >= 0) {
        out->src = NETCFG_SRC_FLASH;
        return 0;
    }

    /* Didn't work out->. try the current dir. */
    if(netcfg_load_from("net.cfg", out) >= 0) {
        out->src = NETCFG_SRC_CWD;
        return 0;
    }

    /* Finally, try the CD */
    if(netcfg_load_from("/cd/net.cfg", out) >= 0) {
        out->src = NETCFG_SRC_CDROOT;
        return 0;
    }

    return -1;
}

int netcfg_save_to(const char *fn, const netcfg_t *cfg) {
    FILE * f;
    char buf[256];

    assert(cfg);

    dbgp("Saving: %s\n", fn);

    /* Open the output file */
    if(fn[0] == '/' && fn[1] == 'v' && fn[2] == 'm' && fn[3] == 'u') {
        dbgp("Saving to VMU\n");
        f = fopen("/ram/netcfg.tmp", "wb");
    }
    else {
        f = fopen(fn, "wb");
    }

    if(!f)
        return -1;

    /* Write out each line... */
    sprintf(buf, "# KOS Network Config written by netcfg_save_to\n");

    if(fwrite(buf, strlen(buf), 1, f) != 1)
        goto error;

#define WRITESTR(fmt, data) \
    sprintf(buf, fmt, data); \
    if(fwrite(buf, strlen(buf), 1, f) != 1) \
        goto error;

    WRITESTR("driver=%s\n", cfg->driver);
    WRITESTR("ip=%08lx\n", cfg->ip);
    WRITESTR("gateway=%08lx\n", cfg->gateway);
    WRITESTR("netmask=%08lx\n", cfg->netmask);
    WRITESTR("broadcast=%08lx\n", cfg->broadcast);
    WRITESTR("dns1=%08lx\n", cfg->dns[0]);
    WRITESTR("dns2=%08lx\n", cfg->dns[1]);
    WRITESTR("hostname=%s\n", cfg->hostname);
    WRITESTR("email=%s\n", cfg->email);
    WRITESTR("smtp=%s\n", cfg->smtp);
    WRITESTR("pop3=%s\n", cfg->pop3);
    WRITESTR("pop3_login=%s\n", cfg->pop3_login);
    WRITESTR("pop3_passwd=%s\n", cfg->pop3_passwd);
    WRITESTR("proxy_host=%s\n", cfg->proxy_host);
    WRITESTR("proxy_port=%d\n", cfg->proxy_port);
    WRITESTR("ppp_login=%s\n", cfg->ppp_login);
    WRITESTR("ppp_passwd=%s\n", cfg->ppp_passwd);

    switch(cfg->method) {
        case 0:
            WRITESTR("method=%s\n", "dhcp");
            break;
        case 1:
            WRITESTR("method=%s\n", "static");
            break;
        case 4:
            WRITESTR("method=%s\n", "pppoe");
            break;
    }

#undef WRITESTR

    fclose(f);

    /* If we're saving to a VMU, tack on the header and send it out */
    if(fn[0] == '/' && fn[1] == 'v' && fn[2] == 'm' && fn[3] == 'u') {
        netcfg_vmuify("/ram/netcfg.tmp", fn);
        unlink("/ram/netcfg.tmp");
    }

    return 0;

error:
    fclose(f);
    return -1;
}

int netcfg_save(const netcfg_t *cfg) {
    file_t f;
    dirent_t * d;
    char buf[64];

    /* Scan for a VMU */
    f = fs_open("/vmu", O_RDONLY | O_DIR);

    if(f < 0)
        return -1;

    d = fs_readdir(f);

    if(!d) {
        fs_close(f);
        return -1;
    }

    /* Valid names are a1, a2, b1, etc */
    sprintf(buf, "/vmu/%.*s/net.cfg", 2, d->name);
    fs_close(f);

    return netcfg_save_to(buf, cfg);
}

