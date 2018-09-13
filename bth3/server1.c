#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <netinet/in.h>
#include <strings.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <string.h>

int main(){
	//open a socket to listen
	int server_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (server_sock < 0){
		perror("socket");
		return 1;
	}

	struct sockaddr_in serveraddr;
	bzero(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(6969);
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(server_sock, (struct sockaddr*) &serveraddr, sizeof(serveraddr)) < 0){
		perror("bind");	
		return 1;
	}

	if (listen(server_sock, 10) < 0) {
		perror("listen");
		return 1;
	}	

	int newfd;
	struct sockaddr_in cliaddr;
	unsigned int cliaddr_len = sizeof(cliaddr);
	newfd = accept(server_sock, (struct sockaddr*) &cliaddr, &cliaddr_len);
	if (newfd < 0){
		perror("accept");
		return 1;
	} 
	char *ip = inet_ntoa(cliaddr.sin_addr);
	uint16_t cliport = ntohs(cliaddr.sin_port);
	printf("client: %s:%u\n", ip, cliport);
	
	char buf[512];
	int buf_len = sizeof(buf);
	int nbytes = 0;
	nbytes = read(newfd, buf, buf_len);
	if ((nbytes < 0)){
		perror("read");
		return 1;
	}

	printf("received %d bytes from client\n", nbytes);
	printf("message from client: %s\n", buf);

	char send_messg[512] = "Hello client!";
	int send_messg_len = strlen(send_messg) + 1;	//include '\0'
	nbytes = write(newfd, send_messg, send_messg_len);
	if (nbytes < 0){
		perror("write");
		return 1;
	}
	printf("sent %d bytes to client\n", nbytes);

	sleep(15);
	close(newfd);
	close(server_sock);
	return 0;
}
