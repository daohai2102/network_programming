/** Client - send messages to server.
 * @Full name: Dao Duy Hai
 * @Student code: 15020951
 * @Description:
 * Input server's IP address and port from the keyboard.
 * User types in some arbitrary messages, the program sends those to the server.
 * Server send back converted messages, print those messages.
 * Type in 'quit' to exit the program.
 */
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <stdlib.h>
#include "../sockio.h"

const char TERM_STR[] = "quit";

int main(){
	struct sockaddr_in servsin;
	bzero(&servsin, sizeof(servsin));

	servsin.sin_family = AF_INET;

	char *servip;
	char buf[MAX_LEN];
	printf("Enter server's IP addr and port in the form of <IP>:<port>\n");
	scanf("%s", buf);
	getchar();	/* remove '\n' character from the stdin */

	/* parse the buf to get ip address and port */
	servip = strtok(buf, ":");
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
		fgets(buf, sizeof(buf), stdin);
		uint16_t buf_len = strlen(buf);

		/* user only press Enter without input anything */
		if (buf_len <= 1)
			continue;

		/* buf includes '\n', so remove it */
		buf[strlen(buf) - 1] = '\0';
		--buf_len;

		if (!strcmp(buf, TERM_STR)){
			break;
		}

		/* send message length before sending the actual message
		 * to synchronize with the server */
		++buf_len;	/* includes '\0' in the message 
					   instead of inserting it at the
					   server side */
		uint16_t buf_len_n = htons(buf_len);
		if (writeBytes(servsock, &buf_len_n, sizeof(buf_len_n)) <= 0 ||
			writeBytes(servsock, buf, buf_len) <= 0 ||
			readBytes(servsock, &buf_len, sizeof(buf_len)) <= 0 ||
			readBytes(servsock, buf, ntohs(buf_len))<= 0)
		{
			perror("socket IO error");
			break;
		}
		printf("%s\n", buf);
	}
	close(servsock);
}
