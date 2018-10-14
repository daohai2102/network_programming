/** Server
 * @Fullname: Dao Duy Hai
 * @Student code: 15020951
 * @Description:
 * Listen on port 6969.
 * Send files on which clients requested (if possible).
 * Serve multiple clients simultaneously.
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
#include <pthread.h>
#include <sys/wait.h>
#include "../sockio.h"

uint32_t get_file_size(FILE *file);
void sig_child(int signo);

int main(int argc, char **argv){
	signal(SIGCHLD, sig_child);
	FILE *stream;
	if (argc > 1){
		if (!strcmp(argv[1], "--debug"))
			stream = stderr;
		else{
			printf("unkown option: %s\n", argv[1]);
			exit(1);
		}
	}else
		stream = fopen("/dev/null", "w");

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



	/* ignore EPIPE to avoid crashing when writing to a closed socket */
	signal (SIGPIPE, SIG_IGN);

	/* serve each client at a time */
	while (1){
		clisock = accept(servsock, (struct sockaddr*) &clisin, &clisin_len);
		if (clisock < 0){
			if (errno != EINTR)
				perror("accept error");
			continue;
		}

		pid_t pid = fork();
		if (!pid){
			close(servsock);

			uint32_t buf_len;
			char buf[MAX_LEN];
			char filename[256];

			char *cli_ip = inet_ntoa(clisin.sin_addr);
			uint16_t cliport = ntohs(clisin.sin_port);
			printf("connection from client: %s:%u\n", cli_ip, cliport);

			char cli_info[25];
			sprintf(cli_info, "%s:%u", cli_ip, cliport);

			/* Read the file name length first,
			 * then read the file name */
			while (readBytes(clisock, &buf_len, sizeof(buf_len)) > 0 &&
					readBytes(clisock, filename, ntohs(buf_len)) > 0){
				printf("client %s required file: %s\n", cli_info, filename);

				FILE *file = fopen(filename, "rb");
				if (file == NULL) {
					char err_mess[256];
					sprintf(err_mess, "open file %s for %s", filename, cli_info);
					perror(err_mess);
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
					char err_mess[256];
					sprintf(err_mess, "write to socket %s", cli_info);
					perror(err_mess);
					break;
				}

				/* send file */
				uint32_t n_sent = 0;
				short socket_err = 0;
				while (1) {
					buf_len = fread(buf, 1, MAX_LEN, file);
					if (writeBytes(clisock, buf, buf_len) <= 0){
						char err_mess[256];
						sprintf(err_mess, "write to socket %s", cli_info);
						perror(err_mess);
						socket_err = 1;
						break;
					}
					n_sent += buf_len;
					fprintf(stream, "sent %u/%u bytes of %s to %s\n", 
							n_sent, file_size, filename, cli_info);
					if (buf_len < MAX_LEN){
						/* EOF or file ERROR */
						break;
					}
				}

				if (ferror(file)){
					/* reading file error */
					char err_mess[256];
					sprintf(err_mess, "read file %s for %s", filename, cli_info);
					perror(err_mess);
					fclose(file);
					continue;
				}

				if (socket_err){
					fclose(file);
					break;
				}

				printf("\'%s\' was sent successfully to %s!\n", filename, cli_info);
				fclose(file);
			}
			printf("closing connection from %s\n", cli_info);
			close(clisock);
			printf("connection from %s closed\n", cli_info);
			exit(0);
		}
		close(clisock);
	}
	close(servsock);
	if (argc > 1)
		fclose(stream);
	return 0;
}

uint32_t get_file_size(FILE *file){
	fseek(file, 0L, SEEK_END);
	uint32_t sz = ftell(file);
	rewind(file);
	return sz;
}

void sig_child(int signo){
	pid_t pid;
	int status;
	pid = waitpid(-1, &status, WNOHANG);
	/* The code below is copied from waitpid(2) - Linux man page */
	if (pid == -1) {
		perror("waitpid");
	} 
    if (WIFEXITED(status)) {
		printf("%d exited, status=%d\n", pid, WEXITSTATUS(status));
	} else if (WIFSIGNALED(status)) {
		printf("%d killed by signal %d\n", pid, WTERMSIG(status));
	} else if (WIFSTOPPED(status)) {
		printf("%d stopped by signal %d\n", pid, WSTOPSIG(status));
	} else if (WIFCONTINUED(status)) {
		printf("%d continued\n", pid);
	}
}
