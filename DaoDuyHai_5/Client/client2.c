/** Client - download files from server.
 * @Full name: Dao Duy Hai
 * @Student code: 15020951
 * @Description:
 * Allow users to download file from a server sequentially.
 * Server's IP, port, and filenames are input from the keyboard.
 * Users type in "QUIT" to quit the program.
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
#include <time.h>
#include <errno.h>
#include "../sockio.h"

const char TERM_STR[] = "QUIT";

int main(int argc, char **argv){
	FILE *stream = NULL;
	if (argc > 1){
		if (!strcmp(argv[1], "--debug")){
			stream = stderr;
		} else {
			printf("unkown option: %s\n", argv[1]);
			exit(1);
		}
	} else 
		stream = fopen("/dev/null", "w");

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

	/* set time out */
	struct timeval tv;
	tv.tv_sec = 5;
	tv.tv_usec= 0;
	setsockopt(servsock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

	if (connect(servsock, (struct sockaddr*) &servsin, sizeof(servsin)) < 0){
		perror("connect");
		return 1;
	}

	printf("Enter file name you want to download, each file name must be seperated by <Enter>\n");
	printf("Enter \"QUIT\" to exit\n");
	char filename[256];
	while (1){
		printf("file: ");
		fgets(filename, sizeof(filename), stdin);
		uint32_t buf_len = strlen(filename);

		/* user only press Enter without input anything */
		if (buf_len <= 1)
			continue;

		/* filename includes '\n', so remove it */
		filename[buf_len - 1] = '\0';
		--buf_len;

		/* quit */
		if (!strcmp(filename, TERM_STR)){
			break;
		}

		++buf_len;	/* includes '\0' in the message 
					   instead of inserting it at the
					   server side */

		uint32_t buf_len_n = htons(buf_len);

		struct timespec begin, end;
		clock_gettime(CLOCK_MONOTONIC_RAW, &begin);

		/* send file name */
		if (writeBytes(servsock, &buf_len_n, sizeof(buf_len_n)) <= 0 ||
			writeBytes(servsock, filename, buf_len) <= 0)
		{
			perror("write to socket");
			break;
		}

		uint32_t file_size = 0;
		if (readBytes(servsock, &file_size, sizeof(file_size)) <= 0){
			perror("read from socket");
			break;
		}
		file_size = ntohl(file_size);
		if (file_size == 0){
			fprintf(stdout, "cannot download file \'%s\'\n", filename);
			continue;
		}

		FILE *file = fopen(filename, "wb");
		if (file == NULL){
			perror("open file");
			break;
		}

		fprintf(stdout, "file size: %u\n", file_size);
		uint32_t remain = file_size;
		uint32_t n_read = 0;
		while (remain > 0){
			if ((n_read = read(servsock, buf, MAX_LEN)) <= 0){
				break;
			}
			if (fwrite(buf, 1, n_read, file) < n_read){
				perror("write file");
				fclose(file);
				remove(filename);
				return 1;
			}
			remain -= n_read;
			fprintf(stream, "wrote %u/%u bytes of %s\n",
					file_size - remain, file_size, filename);
		}
		fclose(file);
		if (errno == EWOULDBLOCK){
			/* socket timeout => file error at the server side */
			perror("read from socket");
			continue;
		}
		if (n_read == 0){
			/* server closed connection */
			remove(filename);
			fprintf(stderr, "server closed connection\n");
			break;
		}
		if (n_read < 0){
			/* other error */
			remove(filename);
			break;
		}

		clock_gettime(CLOCK_MONOTONIC_RAW, &end);
		long duration = (end.tv_sec - begin.tv_sec)*1e3 
						+ (end.tv_nsec - begin.tv_nsec)/1e6;
		fprintf(stdout, "received \'%s\' successfully in %ld mili seconds\n", 
				filename, duration);
	}
	close(servsock);
	if (argc > 1)
		fclose(stream);
	return 0;
}
