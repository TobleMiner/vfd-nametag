#ifndef _WIFI_H_
#define _WIFI_H_

#define WIFI_PASSWD_LEN 12

#define WIFI_PASSWD_CHARS "AFHL073E2CWIUT4"

void wifi_generate_password(char* buf, size_t len);
char* wifi_alloc_password(size_t len);

#endif
