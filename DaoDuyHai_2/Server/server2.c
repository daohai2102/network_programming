/** Server
 * @Fullname: Dao Duy Hai
 * @Student code: 15020951
 * @Description:
 * Listen on port 6969.
 * Send files on which clients requested (if possible).
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
#include <signal.h>
#include <errno.h>
#include "../sockio.h"


uint32_t get_file_size(FILE *file);

int main(){
	int servsock = socket(AF_INET, SOCK_STREAM, 0);
	if (servsock < 0){
		perror("socket");
		return 1;
	}

	int on = 1;
	setsockopt(servsock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
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

	uint32_t buf_len;
	char buf[MAX_LEN];
	char filename[256];


	/* ignore EPIPE to avoid crashing when writing to a closed socket */
	signal (SIGPIPE, SIG_IGN);

	/* serve each client at a time */
	while ((clisock = accept(servsock, (struct sockaddr*) &clisin, &clisin_len)) >= 0){
		char *cli_ip = inet_ntoa(clisin.sin_addr);
		uint16_t cliport = ntohs(clisin.sin_port);
		printf("connection from client: %s:%u\n", cli_ip, cliport);

		/* Read the file name length first,
		 * then read the file name */
		while (readBytes(clisock, &buf_len, sizeof(buf_len)) > 0 &&
				readBytes(clisock, filename, ntohs(buf_len)) > 0){
			printf("client required file: %s\n", filename);

			FILE *file = fopen(filename, "rb");
			if (file == NULL) {
				perror("open file");
				/* send the file size of 0 to notify the client that
				 * the file cannot be opened */
				buf_len = 0;
				writeBytes(clisock, &buf_len, sizeof(buf_len));
				continue;
			}

			/* send the file size first */
			uint32_t file_size = (uint32_t) get_file_size(file);
			uint32_t file_size_n = htonl(file_size);
			if (writeBytes(clisock, &file_size_n, sizeof(file_size_n)) <= 0){
				perror("write to socket");
				break;
			}

			/* send file */
			uint32_t n_sent = 0;
			short socket_err = 0;
			while (1) {
				buf_len = fread(buf, 1, MAX_LEN, file);
				if (writeBytes(clisock, buf, buf_len) <= 0){
					perror("write to socket");
					socket_err = 1;
					break;
				}
				n_sent += buf_len;
				fprintf(stderr, "sent %u/%u bytes of %s\n", 
						n_sent, file_size, filename);
				if (buf_len < MAX_LEN){
					/* EOF or file ERROR */
					break;
				}
			}

			if (ferror(file)){
				/* read file error */
				perror("read file");
				fclose(file);
				continue;
			}

			if (socket_err){
				fclose(file);
				break;
			}

			printf("\'%s\' was sent successfully!\n", filename);
			fclose(file);
		}
		printf("closing client connection\n");
		close(clisock);
	}
	close(servsock);
	return 0;
}

uint32_t get_file_size(FILE *file){
	fseek(file, 0L, SEEK_END);
	uint32_t sz = ftell(file);
	rewind(file);
	return sz;
}

