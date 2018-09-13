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
#include "sockio.h"



int main(){
	int servsock = socket(AF_INET, SOCK_STREAM, 0);
	if (servsock < 0){
		perror("socket");
		return 1;
	}

	struct sockaddr_in servsin;
	bzero(&servsin, sizeof(servsin));
	servsin.sin_addr.s_addr = htonl(INADDR_ANY);
	servsin.sin_family = AF_INET;
	servsin.sin_port = htons(6969);

	if ((bind(servsock, (struct sockaddr*) &servsin, sizeof(servsin))) < 0){
		perror("bind");
		return 1;
	}

	if ((listen(servsock, 10)) < 0){
		perror("listen");
		return 1;
	}

	struct sockaddr_in clisin;
	unsigned int clisin_len;
	int clisock = accept(servsock, (struct sockaddr*) &clisin, &clisin_len);
	if (clisock < 0){
		perror("accept");
		return 1;
	}

	char *cli_ip = inet_ntoa(clisin.sin_addr);
	uint16_t cliport = ntohs(clisin.sin_port);
	printf("connection from client: %s:%u\n", cli_ip, cliport);

	uint16_t mess_len;
	char mess[MAX_LEN];
	while (readBytes(clisock, &mess_len, sizeof(mess_len)) > 0 &&
			readBytes(clisock, mess, mess_len) > 0){

		printf("received this message: %s\n", mess);

		stringToUpper(mess);

		if ( !(writeBytes(clisock, &mess_len, sizeof(mess_len)) > 0 &&
				writeBytes(clisock, mess, mess_len) > 0)){
			perror("writeBytes error");
			break;
		}
	}
	printf("closing connection\n");
	close(clisock);
//	sleep(10);
	close(servsock);
	return 0;
}

