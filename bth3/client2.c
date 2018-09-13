#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <stdlib.h>
#include "sockio.h"


int main(){
	struct sockaddr_in servsin;
	bzero(&servsin, sizeof(servsin));

	servsin.sin_family = AF_INET;

	char *servip;
	char input[MAX_LEN];
	printf("Enter server's IP addr and port in the form of <IP>:<port>\n");
	scanf("%s", input);
	getchar();
	servip = strtok(input, ":");
	servsin.sin_addr.s_addr = inet_addr(servip);
	servsin.sin_port = htons(atoi(strtok(NULL, ":")));

	int servsock = socket(AF_INET, SOCK_STREAM, 0);
	if (servsock < 0){
		perror("socket");
		return 1;
	}

	if (connect(servsock, (struct sockaddr*) &servsin, sizeof(servsin)) < 0){
		perror("connect");
		return 1;
	}

	printf("Enter messages to send to server, each message must be seperated by <Enter>\n");
	printf("Enter \"quit\" to exit\n");
	while (1){
		fgets(input, sizeof(input), stdin);
		input[strlen(input) - 1] = '\0';

		if (!strcmp(input, TERM_STR)){
			break;
		}

		uint16_t input_len = strlen(input) + 1; //include '\0'
		if (writeBytes(servsock, &input_len, sizeof(input_len)) <= 0 ||
			writeBytes(servsock, input, input_len) <= 0 ||
			readBytes(servsock, &input_len, sizeof(input_len)) <= 0 ||
			readBytes(servsock, input, input_len) <= 0)
		{
			perror("socket IO error");
			break;
		}
		printf("%s\n", input);
	}
	close(servsock);
}
