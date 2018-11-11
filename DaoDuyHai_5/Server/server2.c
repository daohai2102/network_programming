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
#include <sys/select.h>
#include "../sockio.h"

#define MAX_N_BYTES (2048)					/* number of bytes to be read/sent 
											   at each call to select() */
#define SIZE_OF_FILENAME (sizeof(uint32_t))	/* number of the first bytes
											   of each request that present 
											   the filename's length */
#define FILE_RETURN_CODE 999999999			/* send_file() return this code when
											   there is an error occured on the file
											   or the file has been sent completely */
#define READ_FILENAME_DONE 2				/* read_filename() return this code
											   to indicate that the filename
											   has been read successfully */
struct net_info{
	int sockfd;
	char addr[22]; // ip:port
};

/* save info from the previous call to select() */
struct temp_mem{
	unsigned int n_bytes_received;	/* number of bytes received from the socket */
	uint32_t filename_length;
	FILE *file;						/* pointer to the file to be sent */
	char filename[256];
	int sent_filesize;				/* equal 1 if the filesize has been sent,
									   otherwise equal 0 */
	unsigned int n_files_sent;		/* number of files the client sent */
};

uint32_t get_file_size(FILE *file);
int read_filename(struct net_info *cli_info, struct temp_mem *mem);
int send_file(struct net_info *cli_info, struct temp_mem *mem);
void remove_sockfd(int sock_index);

static unsigned int nfile = 0;
static FILE *stream = NULL;
static struct net_info cli_info[256];	/* array of connections */
static struct temp_mem cli_mem[256];	/* temp memory used for handling clients */
static unsigned int n_clis = 0;			/* number of active clients */
static fd_set afds_r, rfds, afds_w, wfds, afds_e, efds;

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

	FD_ZERO(&afds_r);
	FD_ZERO(&afds_w);
	FD_ZERO(&afds_e);
	FD_SET(servsock, &afds_r);

	struct sockaddr_in clisin;
	unsigned int sin_len = sizeof(clisin);

	/* ignore EPIPE to avoid crashing when writing to a closed socket */
	signal (SIGPIPE, SIG_IGN);

	/* serve multiple clients simultaneously */
	while (1){
		memcpy(&rfds, &afds_r, sizeof(afds_r));
		memcpy(&wfds, &afds_w, sizeof(afds_w));
		memcpy(&efds, &afds_e, sizeof(afds_e));

		fprintf(stream, "calling select()\n");
		if (select(FD_SETSIZE, &rfds, &wfds, &efds, (struct timeval *)0) < 0){
			perror("select");
			exit(1);
		}
		
		/* new client */
		if (FD_ISSET(servsock, &rfds)){
			fprintf(stream, "can read from listening socket\n");
			cli_info[n_clis].sockfd = accept(servsock, 
											 (struct sockaddr*) &clisin, 
											 &sin_len);
			if (cli_info[n_clis].sockfd < 0){
				perror("accept connection");
			} else {
				// erase temp memory for new client
				bzero(&(cli_mem[n_clis]), sizeof(struct temp_mem));

				char ip_addr[16];
				uint16_t port;
				inet_ntop(AF_INET, &(clisin.sin_addr), ip_addr, sizeof(ip_addr));
				port = ntohs(clisin.sin_port);
				sprintf(cli_info[n_clis].addr, "%s:%u", ip_addr, port);
				printf("connection from client: %s\n", cli_info[n_clis].addr);

				FD_SET(cli_info[n_clis].sockfd, &afds_r);
				//FD_SET(cli_info[n_clis].sockfd, &afds_w);
				FD_SET(cli_info[n_clis].sockfd, &afds_e);

				++n_clis;
			}
			bzero(&clisin, sizeof(clisin));
			FD_CLR(servsock, &rfds);
		}

		/* explore readset and exceptionset */
		int i = 0;
		for ( ; i < n_clis; ++i){
			int socket_error = 0;
			char err_mess[256];
			int read_stat = -1;

			if (FD_ISSET(cli_info[i].sockfd, &efds)){
				perror("exception set");
				socket_error = 1;
			}

			if (!socket_error && FD_ISSET(cli_info[i].sockfd, &rfds)){
				fprintf(stream, "can read from %s\n", cli_info[i].addr);
				read_stat = read_filename(&cli_info[i], &cli_mem[i]);
				if (read_stat <= 0){
					socket_error = 1;
				} else if (read_stat == READ_FILENAME_DONE){
					FD_SET(cli_info[i].sockfd, &afds_w);
				}
			}

			if (socket_error){
				/* an error has occured 
				 * or the socket has been closed by the client 
				 * => delete all data about the client */
				if (read_stat < 0){
					/* unexpected socket error */
					strerror_r(errno, err_mess, sizeof(err_mess));
					fprintf(stderr, "read from socket %s: %s\n", 
							cli_info[i].addr, err_mess);
				} else {
					/* connection closed at the client side */
					printf("client %s closed connection\n", cli_info[i].addr);
				}
				remove_sockfd(i);
			}
		}

		/* explore writeset */
		i = 0;
		for ( ; i < n_clis; ++i){
			if (FD_ISSET(cli_info[i].sockfd, &wfds)){
				fprintf(stream, "can write to %s\n", cli_info[i].addr);
				int write_stat = -1;
				char err_mess[256];
				write_stat = send_file(&cli_info[i], &cli_mem[i]);
				if (write_stat < 0){
					/* unexpected socket error */
					strerror_r(errno, err_mess, sizeof(err_mess));
					fprintf(stderr, "write to socket %s: %s\n", 
							cli_info[i].addr, err_mess);
					remove_sockfd(i);
				} else if (write_stat == 0){
					/* connection closed at the client side */
					printf("client %s closed connection\n", cli_info[i].addr);
					remove_sockfd(i);
				} else if (write_stat == FILE_RETURN_CODE){
					FD_CLR(cli_info[i].sockfd, &afds_w);
				}
			}
		}
	}
	close(servsock);

	if (argc > 1)
		fclose(stream);
	return 0;
}


uint32_t get_file_size(FILE *file){
	fflush(file);
	uint32_t current_pos = ftell(file);
	fseek(file, 0L, SEEK_END);
	uint32_t sz = ftell(file);
	fseek(file, current_pos, SEEK_SET);
	return sz;
}

int read_filename(struct net_info *cli_info, struct temp_mem *mem){
	char buf[MAX_N_BYTES];
	unsigned int n_bytes = read(cli_info->sockfd, (void*)buf, MAX_N_BYTES);

	if (n_bytes <= 0){
		return n_bytes;
	}

	fprintf(stream, "received %u bytes from the client %s\n", 
			n_bytes, cli_info->addr);

	unsigned int n_bytes_previous = mem->n_bytes_received;
	mem->n_bytes_received += n_bytes;

	/* Read the filename length first,
	 * then read the filename */
	if (n_bytes_previous < SIZE_OF_FILENAME){
		/* Still has not gotten enough bytes for 'filename_length' */
		// copy the first bytes from 'buf' to fill in the 'filename_length'
		void *dst = ((void*)&(mem->filename_length)) + n_bytes_previous;
		void *src = (void*)buf;
		unsigned int n = (n_bytes < (SIZE_OF_FILENAME - n_bytes_previous)) 
						? n_bytes : SIZE_OF_FILENAME - n_bytes_previous;
		memcpy(dst, src, n);
		fprintf(stream, "with %s: copied %u bytes to \'filename_length\'\n",
				cli_info->addr, n);
		if (n_bytes_previous < SIZE_OF_FILENAME)
			fprintf(stream, "with %s: filename length = %u\n", 
					cli_info->addr, ntohs(mem->filename_length));
	}

	if (mem->n_bytes_received > SIZE_OF_FILENAME){
		/* Has got enough bytes for 'filename_length' */
		// copy last bytes from 'buf' to fill in the 'filename'
		void *dst = ((void*)mem->filename);
		void *src = (void*)buf;
		unsigned int n = 0;

		if (n_bytes_previous < SIZE_OF_FILENAME){
			/* This is the first time the 'filename' is filled */
			src += SIZE_OF_FILENAME - n_bytes_previous;
			n = n_bytes - (SIZE_OF_FILENAME - n_bytes_previous);
		} else {
			dst += n_bytes_previous - SIZE_OF_FILENAME;
			n = n_bytes;
		}

		memcpy(dst, src, n);
		fprintf(stream, "with %s: copied %u bytes to \'filename\'\n",
				cli_info->addr, n);
	}

	if (ntohs(mem->filename_length) == (mem->n_bytes_received - SIZE_OF_FILENAME)){
		/* Has got 'filename' completely.
		 * Next is to send file content to the client */
		printf("%s required file: %s\n", cli_info->addr, mem->filename);
		return READ_FILENAME_DONE;
	}

	return 1;
}

int send_file(struct net_info *cli_info, struct temp_mem *mem){
	char err_mess[256];
	unsigned int n_bytes;
	
	if (mem->file == NULL){
		/* open the file for the first time */
		mem->file = fopen(mem->filename, "rb");
		
		if (mem->file == NULL) {
			strerror_r(errno, err_mess, sizeof(err_mess));
			fprintf(stderr, "open %s for %s: %s\n", 
					mem->filename, cli_info->addr, err_mess);
			/* send the file size of 0 to notify the client that
			 * the file cannot be opened */
			int buf_len = 0;
			n_bytes = writeBytes(cli_info->sockfd, &buf_len, sizeof(buf_len));
			if (n_bytes <= 0)
				return n_bytes;

			mem->file = NULL;
			mem->filename[0] = '\0';
			mem->filename_length = 0;
			mem->n_bytes_received = 0;
			mem->sent_filesize = 0;
			return FILE_RETURN_CODE;
		}
	}

	if (!mem->sent_filesize){
		/* send the filesize first */
		uint32_t file_size = (uint32_t) get_file_size(mem->file);
		uint32_t file_size_n = htonl(file_size);
		n_bytes = writeBytes(cli_info->sockfd, &file_size_n, sizeof(file_size_n));
		if (n_bytes <= 0){
			fclose(mem->file);
			return n_bytes;
		}
		mem->sent_filesize = 1;
	}

	/* sent file content,
	 * only send MAX_N_BYTES bytes at each time */
	char buf[MAX_N_BYTES];

	unsigned int buf_len = fread(buf, 1, MAX_N_BYTES, mem->file);
	fprintf(stream, "with %s: read %u bytes from file\n", 
			cli_info->addr, buf_len);

	int read_file_done = 0;
	if (buf_len < MAX_N_BYTES){
		/* EOF or file ERROR */
		if (ferror(mem->file)){
			/* read file error */
			strerror_r(errno, err_mess, sizeof(err_mess));
			fprintf(stderr, "read file \'%s\' for %s: %s\n", 
					mem->filename, cli_info->addr, err_mess);
			fclose(mem->file);

			mem->file = NULL;
			mem->filename[0] = '\0';
			mem->filename_length = 0;
			mem->n_bytes_received = 0;
			mem->sent_filesize = 0;
			return FILE_RETURN_CODE;
		} else {
			read_file_done = 1;
		}
	}

	/* need to check EOF */
	if (buf_len > 0)
		n_bytes = writeBytes(cli_info->sockfd, buf, buf_len);

	if (n_bytes <= 0){
		fclose(mem->file);
		return n_bytes;
	}

	fflush(mem->file);
	/* current position of the file pointer is equal to the number of bytes sent */
	unsigned int n_bytes_sent = ftell(mem->file);
	fprintf(stream, "sent %u/%u bytes of %s to %s\n", 
			n_bytes_sent, get_file_size(mem->file), mem->filename, cli_info->addr);

	if (read_file_done){
		printf("\'%s\' was sent successfully to %s!\n", 
				mem->filename, cli_info->addr);
		nfile ++;
		mem->n_files_sent ++;
		printf("sent %u files to the client: %s\n", 
				mem->n_files_sent, cli_info->addr);
		printf("total files sent: %u\n", nfile);
		fclose(mem->file);

		/* clean temp memory to prepare for the new request */
		mem->file = NULL;
		mem->filename[0] = '\0';
		mem->filename_length = 0;
		mem->n_bytes_received = 0;
		mem->sent_filesize = 0;

		return FILE_RETURN_CODE;
	}
	return 1;
}


void remove_sockfd(int sock_index){
	/* unset the corresponding bit */
	FD_CLR(cli_info[sock_index].sockfd, &afds_r);
	FD_CLR(cli_info[sock_index].sockfd, &afds_w);
	FD_CLR(cli_info[sock_index].sockfd, &afds_e);

	printf("closing connection from %s\n", cli_info[sock_index].addr);
	close(cli_info[sock_index].sockfd);
	printf("connection from %s closed\n", cli_info[sock_index].addr);

	/* remove the socket from the accepted socket list */
	int j = sock_index;
	for ( ;j < n_clis; ++j){
		cli_info[j] = cli_info[j+1];
		cli_mem[j] = cli_mem[j+1];
	}
	--n_clis;
}
