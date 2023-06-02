/* KallistiOS ##version##

   isp-settings.c
   Copyright (C) 2004 Megan Potter
   Copyright (C) 2015 Lawrence Sebald
*/

#include <stdio.h>
#include <dc/flashrom.h>

/* This uses the new flashrom ISP settings code, which is based
   heavily on the info from Sam Steele. Thanks Sam! */

int main(int argc, char **argv) {
    flashrom_ispcfg_t cfg;

    // Get the flashrom config, if it's there.
    if(flashrom_get_ispcfg(&cfg) < 0) {
        printf("Couldn't find any IP-related config info!\n");
    }
    else {
        if((cfg.valid_fields & FLASHROM_ISP_IP)) {
            static const char * methods[] = {
                "Dialup(?)",
                "DHCP",
                "PPPoE"
                "Static",
            };
            /* Check that cfg.method is set to a known value */
            if(cfg.method < (sizeof(methods) / sizeof(methods[0])))
                printf("Method:   %s\n", methods[cfg.method]);
            else
                printf("Method:   unknown(%i)\n", cfg.method);
            printf("IP:       %i.%i.%i.%i\n", cfg.ip[0], cfg.ip[1], cfg.ip[2], cfg.ip[3]);
            printf("Netmask:  %i.%i.%i.%i\n", cfg.nm[0], cfg.nm[1], cfg.nm[2], cfg.nm[3]);
            printf("Gateway:  %i.%i.%i.%i\n", cfg.gw[0], cfg.gw[1], cfg.gw[2], cfg.gw[3]);
            printf("Hostname: '%s'\n", cfg.hostname);
        }

        if((cfg.valid_fields & FLASHROM_ISP_DNS)) {
            printf("DNS 1:    %i.%i.%i.%i\n", cfg.dns[0][0], cfg.dns[0][1], cfg.dns[0][2], cfg.dns[0][3]);
            printf("DNS 2:    %i.%i.%i.%i\n", cfg.dns[1][0], cfg.dns[1][1], cfg.dns[1][2], cfg.dns[1][3]);
        }

        if((cfg.valid_fields & FLASHROM_ISP_EMAIL))
            printf("Email:    '%s'\n", cfg.email);

        if((cfg.valid_fields & FLASHROM_ISP_SMTP))
            printf("SMTP:     '%s'\n", cfg.smtp);

        if((cfg.valid_fields & FLASHROM_ISP_POP3))
            printf("POP3:     '%s'\n", cfg.pop3);

        if((cfg.valid_fields & FLASHROM_ISP_POP3_USER))
            printf("POP3 Login: '%s'\n", cfg.pop3_login);

        if((cfg.valid_fields & FLASHROM_ISP_POP3_PASS))
            printf("POP3 Pass:  '%s'\n", cfg.pop3_passwd);

        if((cfg.valid_fields & FLASHROM_ISP_PROXY_HOST))
            printf("Proxy Host: '%s'\n", cfg.proxy_host);

        if((cfg.valid_fields & FLASHROM_ISP_PROXY_PORT))
            printf("Proxy Port: %d\n", cfg.proxy_port);

        if((cfg.valid_fields & FLASHROM_ISP_PPP_USER))
            printf("PPP Login:  '%s'\n", cfg.ppp_login);

        if((cfg.valid_fields & FLASHROM_ISP_PPP_PASS))
            printf("PPP Pass:   '%s'\n", cfg.ppp_passwd);

        if((cfg.valid_fields & FLASHROM_ISP_PHONE1))
            printf("PPP Phone1:   '%s'\n", cfg.phone1);
    }

    return 0;
}
