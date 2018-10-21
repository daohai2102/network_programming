/** Server
 * @Fullname: Dao Duy Hai
 * @Student code: 15020951
 * @Description:
 * Listen on port 6969.
 * Send files clients requested (if possible).
 * Can serve multiple clients simultaneously by using multithread.
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
#include "../sockio.h"

struct net_info{
	int sockfd;
	char ip_add[16];
	uint16_t port;
};

uint32_t get_file_size(FILE *file);
void *doit(void *arg);
void *display_counter(void *arg);

unsigned int nfile = 0;
FILE *stream = NULL;
static pthread_mutex_t lock_nfile = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t nfile_cond = PTHREAD_COND_INITIALIZER;

int main(int argc, char **argv){

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

	struct net_info *cli_info = malloc(sizeof(struct net_info));
	struct sockaddr_in clisin;
	unsigned int sin_len = sizeof(clisin);
	bzero(&clisin, sizeof(clisin));

	/* ignore EPIPE to avoid crashing when writing to a closed socket */
	signal (SIGPIPE, SIG_IGN);

	/* create a thread to display file counter */
	pthread_t counter_tid;
	int thr = pthread_create(&counter_tid, NULL, &display_counter, NULL);
	if (thr != 0){
		perror("create a thread to display file counter");
		exit(1);
	}
	
	/* serve multiple clients simultaneously */
	while (1){
		cli_info->sockfd = accept(servsock, (struct sockaddr*) &clisin, &sin_len);
		if (cli_info->sockfd < 0){
			perror("accept connection");
			continue;
		}

		inet_ntop(AF_INET, &(clisin.sin_addr), cli_info->ip_add, sizeof(cli_info->ip_add));
		cli_info->port = ntohs(clisin.sin_port);

		/* create a new thread for each client */
		pthread_t tid;
		int thr = pthread_create(&tid, NULL, &doit, (void*)cli_info);
		if (thr != 0){
			char err_mess[255];
			strerror_r(errno, err_mess, sizeof(err_mess));
			fprintf(stderr, "create thread to handle %s:%u: %s\n", 
					cli_info->ip_add, cli_info->port, err_mess);
			close(cli_info->sockfd);
			continue;
		}
		cli_info = malloc(sizeof(struct net_info));
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

void *display_counter(void *arg){
	pthread_detach(pthread_self());
	while (1){
		pthread_cond_wait(&nfile_cond, &lock_nfile);
		printf("number of files sent: %u\n", nfile);
		pthread_mutex_unlock(&lock_nfile);
	}
}

void *doit(void *arg){
	pthread_detach(pthread_self());
	struct net_info cli_info = *((struct net_info*) arg);
	free(arg);
	char err_mess[256];
	uint32_t buf_len;
	char buf[MAX_LEN];
	char filename[256];
	char cli_addr[256];
	sprintf(cli_addr, "%s:%u", cli_info.ip_add, cli_info.port);

	printf("connection from client: %s\n", cli_addr);

	/* Read the file name length first,
	 * then read the file name */
	while (readBytes(cli_info.sockfd, &buf_len, sizeof(buf_len)) > 0 &&
			readBytes(cli_info.sockfd, filename, ntohs(buf_len)) > 0){
		printf("%s required file: %s\n", cli_addr, filename);

		FILE *file = fopen(filename, "rb");
		if (file == NULL) {
			strerror_r(errno, err_mess, sizeof(err_mess));
			fprintf(stderr, "open %s for %s: %s\n", filename, cli_addr, err_mess);
			/* send the file size of 0 to notify the client that
			 * the file cannot be opened */
			buf_len = 0;
			writeBytes(cli_info.sockfd, &buf_len, sizeof(buf_len));
			continue;
		}

		/* send the file size first */
		uint32_t file_size = (uint32_t) get_file_size(file);
		uint32_t file_size_n = htonl(file_size);
		if (writeBytes(cli_info.sockfd, &file_size_n, sizeof(file_size_n)) <= 0){
			strerror_r(errno, err_mess, sizeof(err_mess));
			fprintf(stderr, "write to socket %s: %s\n", cli_addr, err_mess);
			break;
		}

		/* send file */
		uint32_t n_sent = 0;
		short socket_err = 0;
		while (1) {
			buf_len = fread(buf, 1, MAX_LEN, file);
			if (writeBytes(cli_info.sockfd, buf, buf_len) <= 0){
				strerror_r(errno, err_mess, sizeof(err_mess));
				fprintf(stderr, "write to socket %s: %s\n", cli_addr, err_mess);
				socket_err = 1;
				break;
			}
			n_sent += buf_len;
			fprintf(stream, "sent %u/%u bytes of %s to %s\n", 
					n_sent, file_size, filename, cli_addr);
			if (buf_len < MAX_LEN){
				/* EOF or file ERROR */
				break;
			}
		}

		if (ferror(file)){
			/* read file error */
			strerror_r(errno, err_mess, sizeof(err_mess));
			fprintf(stderr, "read file \'%s\' for %s: %s\n", filename, cli_addr, err_mess);
			fclose(file);
			continue;
		}

		if (socket_err){
			fclose(file);
			break;
		}

		pthread_mutex_lock(&lock_nfile);
		nfile ++;
		fprintf(stderr, "thread %lu increased counter to %d\n", pthread_self(), nfile);
		pthread_cond_signal(&nfile_cond);
		pthread_mutex_unlock(&lock_nfile);

		printf("\'%s\' was sent successfully to %s!\n", filename, cli_addr);
		fclose(file);
	}
	printf("closing connection from %s\n", cli_addr);
	close(cli_info.sockfd);
	printf("connection from %s closed\n", cli_addr);
	return NULL;
}
