#ifndef hexdump
#include <strings.h>
#define hexdump(data, len) do { if (len < 1024) { char hexdumpbuf[3072]; bzero(hexdumpbuf, 3072); for (uint32 i = 0; i < (uint32)len; i ++) { sprintf(hexdumpbuf + i * 3, "%02x ", *(uint8 *)(((uint8 *)data) + i));} puts(hexdumpbuf);} else { char * hexdumpbuf = (char *) malloc(len * 3 + 1); if (hexdumpbuf != NULL) {bzero(hexdumpbuf, len * 3 + 1); for (uint32 i = 0; i < (uint32)len; i ++) { sprintf(hexdumpbuf + i * 3, "%02x ", *(uint8 *)(((uint8 *)data) + i));} puts(hexdumpbuf); free(hexdumpbuf);} else puts("Too long, ignoring");}} while(0)
#endif
