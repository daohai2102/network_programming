/** Server
 * @Fullname: Dao Duy Hai
 * @Student code: 15020951
 * @Description:
 * Accept connections from clients to port 6969
 * Receive messages from them
 * Convert those messages to upper case
 * Send back converted messages to clients.
 */
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
#include "../sockio.h"


void stringToUpper(char *str);

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
	unsigned int clisin_len = sizeof(clisin);
	int clisock;

	uint16_t mess_len;
	char mess[MAX_LEN];

	/* serve each client at a time */
	while ((clisock = accept(servsock, (struct sockaddr*) &clisin, &clisin_len)) >= 0){
		char *cli_ip = inet_ntoa(clisin.sin_addr);
		uint16_t cliport = ntohs(clisin.sin_port);
		printf("connection from client: %s:%u\n", cli_ip, cliport);

		/* each message has a header of 16 bits which is the length of its
		 * following message.
		 * So, read the header first */
		while (readBytes(clisock, &mess_len, sizeof(mess_len)) > 0 &&
				readBytes(clisock, mess, ntohs(mess_len)) > 0){
			printf("received message: %s\n", mess);
			stringToUpper(mess);

			/* write the header first */
			if ( !(writeBytes(clisock, &mess_len, sizeof(mess_len)) > 0 &&
					writeBytes(clisock, mess, ntohs(mess_len)) > 0)){
				perror("writeBytes error");
				break;
			}
		}
		printf("closing client connection\n");
		close(clisock);
	}
	close(servsock);
	return 0;
}

/** stringToUpper - convert each characters in the string to the upper case.
 * @str: character array.
 */
void stringToUpper(char *str){
	for (int i = 0; str[i] != '\0'; i++){
		str[i] = toupper(str[i]);
	}
}
