#ifndef _SOCK_IO_H
#define _SOCK_IO_H

#include <stdio.h>
#include <ctype.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <strings.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <stdlib.h>

#define MAX_LEN 2000
const char TERM_STR[] = "quit";

int readBytes(int sock, void* buf, unsigned int nbytes);
int writeBytes(int sock, void* buf, unsigned int nbytes);
void stringToUpper(char *str);


int readBytes(int sock, void* buf, unsigned int nbytes){
	unsigned int read_bytes = 0;
	while (1){
		read_bytes = read(sock, buf, nbytes);
		if (read_bytes <= 0)
			return read_bytes;
		if (read_bytes >= nbytes)
			break;
		nbytes -= read_bytes;
		buf += read_bytes;
	}
	return 1;
}

int writeBytes(int sock, void* buf, unsigned int nbytes){
	unsigned int wrote_bytes = 0;
	while (1) {
		wrote_bytes = write(sock, buf, nbytes);
		if (wrote_bytes <= 0)
			return wrote_bytes;
		if (wrote_bytes >= nbytes)
			break;
		nbytes -= wrote_bytes;
		buf += wrote_bytes;
	}	
	return 1;
}

void stringToUpper(char *str){
	for (int i = 0; str[i] != '\0'; i++){
		str[i] = toupper(str[i]);
	}
}

#endif
