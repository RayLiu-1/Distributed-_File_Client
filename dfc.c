#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
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
int userindex = 0;

int set_server(int *sock, struct sockaddr_in *server, int index);
int set_config(char file[]);
//void *connection_handler(void *sockfd);

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
	if (argc <2)
	{
		perror("USAGE:  <config file>");
		exit(1);
	}
	set_config(argv[1]);
	struct timeval timeout;
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	int sock[4];
	struct sockaddr_in server[4];
	char readbuf[4][BUFSIZE];
	char sendbuf[4][BUFSIZE];
	int connect[4];

	for (int i = 0; i < 4; i++) {
		connect[i] = set_server(&(sock[i]), &(server[i]), i);
		if (connect[i] == 1) {
			if (setsockopt(sock[i], SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
				sizeof(timeout)) < 0)
				perror("setsockopt failed\n");
		}
	}
	puts("loging...");
	for (int i = 0; i < 4; i++) {
		if (connect[i] == 1) {
			write(sock[i], username, strlen(username));
			int n = recv(sock[i], readbuf[i], BUFSIZE, 0);
			if (n == -1) {
				connect[i] = 0;
				printf("server%d disconnected\n", i + 1);
				close(sock[i]);
				continue;
			}
			write(sock[i], password, strlen(password));
			n = recv(sock[i], readbuf[i], BUFSIZE, 0);
			if (n == -1) {
				connect[i] = 0;
				printf("server%d disconnected\n", i + 1);
				close(sock[i]);
				continue;
			}

		}
	}
	if (connect[0]||connect[1]||connect[2]||connect[3])
	{
		puts("logged");	
		while (1) {
			int read_size = 0;
			printf("Please enter command: ");
			char commond[BUFSIZE];
			fgets(commond, BUFSIZE, stdin);
			if ((strncmp(commond, "LIST ",5) == 0)||(strcmp(commond,"LIST\n")==0)) {
				char filelist[200][256];
				int filemark[200];
				int filesize = 0;
				for (int i = 0; i < 200; i++) {
					filemark[i] = 0;
				}
				for (int i = 0; i < 4; i++)
				{	
					if (connect[i] == 1)
						write(sock[i], commond, strlen(commond));
				}
				for (int i = 0; i < 4; i++) {
					if (connect[i] == 1) {
						while (read_size = recv(sock[i], readbuf[i], BUFSIZE, 0)>0) {
							if (strlen(readbuf[i])>4 && readbuf[i][2]=='.'&&readbuf[i][0] == '.' && (readbuf[i][1] == '1' || readbuf[i][1] == '2' || readbuf[i][1] == '3' || readbuf[i][1] == '4')) {
								int find = 0;
								for (int j = 0; j < filesize; j++) {
									if (strcmp(readbuf[i] + 3, filelist[j])==0) {
										int part = readbuf[i][1]-'1';
										int bitpart = 1 << part;
										filemark[j] |= bitpart;
										find = 1;
									}
								}
								if (find == 0) {
									strcpy(filelist[filesize++], readbuf[i] + 3);
									int part = readbuf[i][1] - '1';
									int bitpart = 1 << part;
									filemark[j] |= bitpart;
								}
							}
						}if (read_size == -1) {
							connect[i] = 0;
							printf("server%d disconnected\n", i + 1);
							close(sock[i]);
							continue;
						}
					}
				}
				for (int i = 0; i < filesize; i++) {
					if ((filemark[i] & 15) == 15)
						puts(filelist[i]);
					else
					{
						printf("%s[incomplete]\n", filelist[i]);
					}
				}

			}
			
	}
	return 0;
		
	}
	else {
		puts("Wrong username//pasword");
	}

}

int set_server(int *sock, struct sockaddr_in *server,int serverindex) {
	*sock = socket(AF_INET, SOCK_STREAM, 0);
	if(*sock == -1)
	{
		printf("Could not create socket");
	}
	printf("Socket%d created\n", serverindex +1);
	char * pch = serverIp[serverindex];
	pch = strtok(serverIp[serverindex], ":");
	server->sin_addr.s_addr = inet_addr(pch);
	server->sin_family = AF_INET;
	pch = strtok(NULL, ":");
	server->sin_port = htons(atoi(pch));
	if (connect(*sock, (struct sockaddr *)server, sizeof(*server)) < 0)
	{
		printf("connect server%d failed.\n", serverindex +1);
		return 0;
	}

	printf("Connected Server%d\n",serverindex+1);
	return 1;
}

int set_config(char* file) {
	FILE *fp;
	fp = fopen(file, "r");

	if (fp == NULL) {
		perror("failed file opening");
		return 1;
	}
	char readBuf[BUFSIZE];
	userindex = 0;
	while (fgets(readBuf, BUFSIZE, (FILE*)fp))
	{
		char * pch;
		pch = strtok(readBuf, " ");
		if (strcmp(pch, "Server")==0) {
			pch = strtok(NULL, " ");
			strcpy(server[userindex], pch);
			pch = strtok(NULL, " ");
			strcpy(serverIp[userindex++], pch);
		}
		else if (strcmp(pch, "Username:")==0) {
			pch = strtok(NULL, " \n");
			strcpy(username, pch);
		}
		else if (strcmp(pch, "Password:")==0) {
			pch = strtok(NULL, " \n");
			strcpy(password, pch);
		}
	}
	fclose(fp);
	return 0;
}

//void *connection_handler(void *sockfd) {}