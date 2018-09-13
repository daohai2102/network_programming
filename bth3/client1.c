#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <stdlib.h>


int main(){
	struct sockaddr_in servsockaddr;
	bzero(&servsockaddr, sizeof(servsockaddr));
	servsockaddr.sin_family = AF_INET;
	
	char *servaddr;
	char input[22];
	printf("Enter server's IP addr and port in the form of <IP>:<port>\n");
	scanf("%s", input);
	servaddr = strtok(input, ":");
	servsockaddr.sin_addr.s_addr = inet_addr(servaddr);
	servsockaddr.sin_port = htons(atoi(strtok(NULL, ":")));
	
	int servsock = socket(AF_INET, SOCK_STREAM, 0);
	if (servsock < 0){
		perror("socket");
		return 1;
	}

	if (connect(servsock, (struct sockaddr*) &servsockaddr, sizeof(servsockaddr)) < 0){
		perror("connect");
		return 1;
	}

	//send message to server
	char messg[] = "Hello server!";
	unsigned int messg_len = sizeof(messg);
	int nbytes = 0;
	nbytes = write(servsock, messg, messg_len);
	if ((nbytes < 0)){
		perror("write");
		return 1;
	}
	printf("sent %d bytes\n", nbytes);

	//receive message from server
	char buf[512];
	int buf_len = sizeof(buf);
	nbytes = read(servsock, buf, buf_len);
	if (nbytes < 0){
		perror("read");
		return 1;
	}
	printf("message from server: %s\n", buf);

	close(servsock);
	return 0;
}
