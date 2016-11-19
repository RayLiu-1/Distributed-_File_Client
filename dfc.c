#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>


#define BUFSIZE 4096
#define QUESIZE 4// maximum number of client connections
char DocumentRoot[200] = "/";
char server[4][100];
char serverIp[4][100];
char username[16];
char password[32];
int index = 0;

void encrypt(char content[], char password[], int len)
{
	unsigned int j = 0;
	for (int i = 0; i<len; i++)
	{
		if ((int)content[i] - (int)password[j] < CHAR_MIN) {
			content[i] = ((int)content[i] - (int)password[j] + 256);
		}
		else {
			content[i] = ((int)content[i] - (int)password[j]);
		}
		j = (j + 1) % strlen(password);
	}
}

void decrypt(char content[], char password[], int lenth)
{
	unsigned int j = 0;
	for (int i = 0; i<lenth; i++)
	{
		if (content[i] + password[j] > CHAR_MAX) {
			content[i] = (content[i] + password[j] - 256);
		}
		else {
			content[i] = (content[i] + password[j]);
		}
		j = (j + 1) % strlen(password);
	}
}

int main(int argc, char * argv[]) {
	if (argc <1)
	{
		printf("USAGE:  <config file>\n");
		exit(1);
	}
	char file[200];
	strspy(file, argv[0]);
	set_config(file);
	struct timeval timeout;
	timeout.tv_sec = 10;
	timeout.tv_usec = 0;
	int sock[4];
	struct sockaddr_in server[4];
	char readbuf[4][BUFSIZE];
	char sendbuf[4][BUFSIZE];
	int connect[4];
	struct timeval timeout;
	timeout.tv_sec = 10;
	timeout.tv_usec = 0;

	for (int i = 0; i < 4; i++) {
		connect[i] = set_server(sock[i], server[i], i);
		if (connect[i] == 1) {
			if (setsockopt(sock[i], SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
				sizeof(timeout)) < 0)
				error("setsockopt failed\n");
		}
	}
	
	while (1) {
		puts("loging...");
		for (int i = 0; i < 4; i++) {
			if (connect[i] == 1) {
				write(sock[i], username, strlen(username));
				write(sock[i], password, strlen(username));
				int n = recv(sock[i], readbuf, BUFSIZE, 0);
				if (n = -1) {
					connect[i] == 0;
					puts("server%d disconnected");
					close(sock[i])
				}
				if(strcpy())
			}
		}
		printf(Please enter command : );
		
		scanf("%s", sendbuf);
	}
	return 0;
}
int set_server(int &sock, struct sockaddr_in &server,int index) {
	sock = socket(AF_INET, SOCK_STREAM, 0);
	f(sock == -1)
	{
		printf("Could not create socket");
	}
	printf("Socket%d created\n",index+1);
	char pch = serverIp[index];
	pch = strtok(serverIp[index], ":");
	server.sin_addr.s_addr = inet_addr(pch);
	server.sin_family = AF_INET;
	pch = strtok(NULL, ":");
	server.sin_port = htons(pch);
	if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
	{
		printf("connect server%d failed.\n",index+1);
		return 0;
	}

	printf("Connected Server%d\n",index+1);
	return 1;
}

int set_config(char file[]) {
	FILE *fp;
	fp = fopen(file, "r");
	if (fp == NULL) {
		perror("failed file opening");
		return 1;
	}
	char readBuf[BUFSIZE];
	index = 0;
	while (fgets(readBuf, BUFSIZE, (FILE*)fp))
	{
		char * pch;
		pch = strstok(readBuf, " ");
		if (strcmp(pch, "server")) {
			pch = strstok(NULL, " ");
			strcpy(server[index], pch);
			pch = strstok(NULL, "");
			strcpy(serverIp[index++], pch);
		}
		else if (strcmp(pch, "Username:")) {
			pch = strstok(NULL, " ");
			strcpy(username, pch);
		}
		else if (strcmp(pch, "Password:")) {
			pch = strstok(NULL, " ");
			strcpy(password, pch);
		}
	}
	fclose(fp);
	return 0;
}