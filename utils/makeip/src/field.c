/* IP creator (makeip)
 *
 * Copyright (C) 2000, 2001, 2002, 2019, 2020 The KOS Team and contributors.
 * All rights reserved.
 *
 * This code was contributed to KallistiOS (KOS) by Andress Antonio Barajas
 * (BBHoodsta). It was originally made by Marcus Comstedt (zeldin). Some
 * portions of code were made by Andrew Kieschnick (ADK/Napalm). Heavily
 * updated by SiZiOUS. Bootstrap replacement (IP.TMPL) was made by Jacob
 * Alberty (LiENUS).
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "field.h"

field_t fields[NUM_FIELDS] = {
  { HARDWARE_ID,   "Hardware ID",   "SEGA SEGAKATANA",   0x0, 0x10, _check_fixed },
  { MAKER_ID,      "Maker ID",      "SEGA ENTERPRISES", 0x10, 0x10, _check_fixed },
  { DEVICE_INFO,   "Device Info",   "0000 CD-ROM1/1",   0x20, 0x10, _check_deviceinfo },
  { AREA_SYMBOLS,  "Area Symbols",  "JUE",              0x30,  0x8, _check_areasym },
  { PERIPHERALS,   "Peripherals",   "E000F10",          0x38,  0x8, _check_peripherals },
  { PRODUCT_NO,    "Product No",    "T-00000",          0x40,  0xa, NULL },
  { VERSION,       "Version",       "V1.000",           0x4a,  0x6, _check_version },
  { RELEASE_DATE,  "Release Date",  NULL,               0x50, 0x10, _check_date },
  { BOOT_FILENAME, "Boot Filename", "1ST_READ.BIN",     0x60, 0x10, NULL },
  { SW_MAKER_NAME, "SW Maker Name", "KallistiOS",       0x70, 0x10, NULL },
  { GAME_TITLE,    "Game Title",    "GAMETITLE",        0x80, 0x80, NULL },
};

int g_field_error = 0;

char *field_values[NUM_FIELDS];

void
init_release_date()
{
  time_t now;
  struct tm ts;
  char tmpbuf[0x10];

  time(&now);
  ts = *localtime(&now);
  strftime(tmpbuf, sizeof(tmpbuf), "%Y%m%d", &ts); // YYYYMMDD

  field_set_value(RELEASE_DATE, tmpbuf);
}

void
field_initialize()
{
  // initialize field values
  for (int i = 0; i < NUM_FIELDS; i++) {
    if (fields[i].default_value != NULL) {
      field_set_value(i, fields[i].default_value);
    }
  }

  // compute default release date
  init_release_date();
}

void
field_finalize()
{
  for (int i = 0; i < NUM_FIELDS; i++) {
    free(field_values[i]);
  }
}

char *
field_get_value(int index)
{
  return field_values[index];
}

char *
field_get_pretty_value(int index)
{
  char *result = field_get_value(index);
  char *deviceinfo = NULL;

  switch (index) {
    case AREA_SYMBOLS:
      trim(result);
      break;
    case DEVICE_INFO:
      deviceinfo = strchr(result, ' ');
      result = deviceinfo + 1; // skip the space char
      break;
  }

  return result;
}

int
field_set_value(int index, char *value)
{
  field_t *f = &fields[index];

  // checking the value length for that field
  if(strlen(value) > f->length) {
    log_error("data for field \"%s\" is too long\n", f->name);
    return 0;
  }

  // destroying previous allocated memory if needed
  if (field_values[index] != NULL) {
    free(field_values[index]);
  }

  // allocating the required memory
  field_values[index] = (char *) malloc((f->length + 1) * sizeof(char));
  memset(field_values[index], ' ', f->length);
  field_values[index][f->length] = '\0';

  // storing the value
  strcpy(field_values[index], value);

  // do additional checks if required
  if(f->extra_check != NULL && !(*f->extra_check)(f, field_values[index])) {
    g_field_error = 1;
    return 0;
  }

  log_notice("setting field \"%s\" to \"%s\"\n", f->name, field_get_pretty_value(index));

  return 1;
}

int
parse_file(FILE *fh)
{
  static char buf[80];
  int i, line = 0;

  while(fgets(buf, sizeof(buf), fh)) {
    char *p;

    line++;
    trim(buf);

    if(*buf) {
      if((p = strchr(buf, ':'))) {
        *p++ = '\0';
        trim(buf);

	for(i = 0; i < NUM_FIELDS; i++) {
          if(!strcmp(buf, fields[i].name))
            break;
        }

        if(i >= NUM_FIELDS) {
          log_error("unknown field \"%s\"\n", buf);
          return 0;
        }

        while(*p == ' ' || *p == '\t')
          p++;

        if (!field_set_value(i, p))
          return 0;

      } else {
        log_error("missing colon (\":\") on line %d\n", line);
        return 0;
      }
    }
  }

  return 1;
}

void
field_load(char *in)
{
  FILE *fh = fopen(in, "r");

  if(fh == NULL) {
    halt("can't open template: \"%s\"\n", in);
  }

  log_notice("loading template \"%s\"\n", in);

  int result;
  result = parse_file(fh);

  fclose(fh);

  if (!result) {
    exit(EXIT_FAILURE);
  }
}

void
field_write(char *ip)
{
  for (int i = 0; i < NUM_FIELDS; i++) {
    memset(ip + fields[i].position, ' ', fields[i].length);
    char *p = field_get_value(i);
    memcpy(ip + fields[i].position, p, strlen(p));
  }
}

int
field_erroneous() {
  return g_field_error;
}

int
_check_areasym(field_t *f, char *value)
{
  int i, a = 0;

  for(i = 0; i < strlen(value); i++) {
    switch(value[i]) {
     case 'J':
       a |= (1<<0);
       break;
     case 'U':
       a |= (1<<1);
       break;
     case 'E':
       a |= (1<<2);
       break;
     case '\0':
     case ' ':
       break;
     default:
       log_error("field \"%s\" contains an unknown area symbol '%c'\n", f->name, value[i]);
       return 0;
    }
  }

  for(i = 0; i < f->length; i++) {
    if((a & (1 << i)) == 0)
      value[i] = ' ';
    else
      value[i] = "JUE"[i];
  }

  return 1;
}

int
_check_date(field_t *f, char *value)
{
  int is_date = is_valid_date(value);

  if (!is_date) {
    log_error("field \"%s\" is invalid date (format is \"YYYYMMDD\")\n", f->name);	  
  }

  return is_date;
}

int
_check_fixed(field_t *f, char *value)
{
  int result = !(strcmp(f->default_value, value));

  if (!result) {
    log_error("field \"%s\" is not editable (must be \"%s\")\n", f->name, f->default_value);	  
  }

  return result;
}

int
_check_version(field_t *f, char *value)
{
  // value should be Vx.yyy
  int result;
  long major, minor;

  result = (strlen(value) == 6) && (value[0] == 'V') && (value[2] == '.') &&
    substr_long_parse(value, 1, 1, &major) &&
    substr_long_parse(value, 3, 3, &minor);

  if (!result) {
    log_error("field \"%s\" is invalid version (must be Vx.yyy)\n", f->name);
  }

  return result;
}

int
_check_peripherals(field_t *f, char *value)
{
  // value should be from '0000000' to 'FFFFF11'
  int result = 0;

  if (strlen(value) == 7) {
    // check peripherals
    char buf[6];
    strncpy(buf, value, 5);
    buf[5] = '\0';

    // check WinCE (useless but supported...) and VGA flags
    result = is_valid_hex(buf) && is_strict_bool(value[5]) &&
      is_strict_bool(value[6]);
  }

  if (!result) {
    log_error("field \"%s\" contains invalid values\n", f->name);
  }

  return result;
}

int
_check_deviceinfo(field_t *f, char *value)
{
  // can be legacy value like '0000 CD-ROMx/y' or 'CD-ROMx/y'
  int result = 1;

  char *deviceinfo = strdup(value);

  if (strlen(deviceinfo) == 14) { // '0000 CD-ROMx/y'
    char *device = strchr(deviceinfo, ' ');
    *device++ = '\0'; // deviceinfo = '0000'; device = 'CD-ROMx/y'
    result = result && (strlen(deviceinfo) == 4) && is_valid_hex(deviceinfo);
    memmove(deviceinfo, device, strlen(device)); // deviceinfo = 'CD-ROMx/y'
    deviceinfo[strlen(device)] = '\0';
  }

  long dummy;
  result = result && (strlen(deviceinfo) == 9)
    && ((!strncmp(deviceinfo, "CD-ROM", 6)) ||
        (!strncmp(deviceinfo, "GD-ROM", 6)) ||
        (!strncmp(deviceinfo, "MIL CD", 6)))
    && substr_long_parse(deviceinfo, 6, 1, &dummy) && (deviceinfo[7] == '/')
	&& substr_long_parse(deviceinfo, 8, 1, &dummy);

  if (!result) {
    log_error("field \"%s\" contains invalid values (must be CD-ROMx/y)\n", f->name);
  } else if (strlen(deviceinfo) == 9) {
    // if short form (i.e. "CD-ROMx/y" only), append "0000" before
    char *buf = (char *) malloc(f->length * sizeof(char));
    strcpy(buf, "0000 "); // fake CRC that will be updated by makeip
    strcat(buf, deviceinfo); // append CD-ROMx/y
    strcpy(value, buf); // result
    free(buf);
  }

  free(deviceinfo);

  return result;
}
