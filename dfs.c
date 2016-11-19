
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

#define BUFSIZE 4096
#define QUESIZE 4// maximum number of client connections
char DocumentRoot[200] = "./";
char username[100][16];
char password[100][32];
int index = 0;

int main(int argc, char * argv[])
{
	struct timeval timeout;
	if (argc < 2)
	{
		printf("USAGE:  <directory> <server_port>\n");
		exit(1);
	}
	strcat(DocumentRoot, argv[0]);
	int lsfd, cnfd, *sock;
	struct sockaddr_in server, client;
	lsfd = socket(AF_INET, SOCK_STREAM, 0);
	if (lsfd == -1) {
		perror("Create sock failed");
		return 1;
	}	
	int rec = set_config();

	puts("Socket created");
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(argv[1]);
	if (bind(lsfd, (struct sockaddr *)&server, sizeof(server)) < 0) {
		perror("Bind failed");
		return 1;
	}
	puts("Socket bind");
	listen(lsfd, QUESIZE);
	puts("Listenning...");
	int csize = sizeof(client);
	while (1) {
		cnfd = accept(lsfd, (struct sockaddr *)&client, (socklen_t*)&csize);
		puts("connection appected");
		int* pfd = (int*)malloc(sizeof(int));
		*pfd = cnfd;
		pthread_t new_thread;
		if (pthread_create(&new_thread, NULL, connection_handler, (void*)pfd) < 0)
		{
			perror("Create thread failed");
			return 1;
		}
	}
	return 0;
}
void *connection_handler(void *sockfd) {
	struct timeval timeout;
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	struct timeval notimeout;
	notimeout.tv_sec = 0;
	notimeout.tv_usec = 0;
	int read_size;
	int clfd = *(int*)sockfd;
	char readbuf[BUFSIZE];
	char sendbuf[BUFSIZE];
	memset(readbuf, 0, BUFSIZE);
	memset(sendbuf, 0, BUFSIZE);
	char filepath[200];
	int login = 0;
	while (login == 0)
	{
		//sendbuf = "Please enter your username: ";
		//write(clfd, sendbuf, strlen(sendbuf));
		recv(clfd, readbuf, BUFSIZE, 0);
		int i = 0;
		int validUser[100];//mark matched username
		for (i = 0; i < index; i++) {
			validUser[i] = 0;
		}
		for (i = 0; i < index; i++) {
			if (strcmp(username[i], readbuf)==0) {
				username[i] = 1;
			}
		}
		//sendbuf = "Please enter your password: ";
		//write(clfd, sendbuf, strlen(sendbuf));
		int lastMathedUserIndex = 0;
		recv(clfd, readbuf, BUFSIZE, 0);
		for (i = 0; i < index; i++) {
			if (validUser[i] == 1) {
				if (strcmp(readbuf, password[i]) == 0) {
					login = 1;
					lastMathedUserIndex = i;
				}
			}
		}
		if (login==0) {
			sendbuf = "Invalid Username/Password. \n";
			write(clfd, sendbuf, strlen(sendbuf));
			break;
		}
		else {
			strcat(filepath, "//");
			strcat(filepath, username[lastMathedUserIndex]);
			sendbuf = "logged";
			write(clfd, sendbuf, strlen(sendbuf));
		}
	}
	while(login == 1) {
		read_size = recv(clfd, readbuf, BUFSIZE, 0);
		if (read_size <= 0) {
			puts("disconnected");
			break;
		}
		char *pch = readbuf;
		pch = strtok(readbuf, " ");
		if(strlen(pch)!=0 && strcmp(pch,"PUT")){
			char filename[200];
			memset(filename, 0, sizeof(filename));
			strcpy(filename, filepath);
			strcat(filename, "//");
			pch = strtok(NULL, " ");
			strcat(filename, pch);
			pch = strtok(NULL, " ");
			char subdir[200];
			strcpy(subdir, pch);
			if (strlen(subdir) != 0 && subdir[strlen(subdir) - 1] == '/')
			{
				strcat(filename, subdir);
				strcat(filename, "/");
			}
			File* fd;
			fd = fopen(filename, "w");
			if (!fd)
				perror("fail to open file");
			int n = 0;
			while (n = recv(clfd, readbuf, BUFSIZE, 0))
			{
				fwrite(readbuf, sizeof(char), n, (FILE *)fd);
			}
			fclose(fd);
		}
		else if (strlen(pch) != 0 && strcmp(pch, "GET")) {
			char filename[200];
			memset(filename, 0, sizeof(filename));
			strcpy(filename, filepath);
			strcat(filename, "//");
			pch = strtok(NULL, " ");
			strcat(filename, pch);
			int filensize = 0;
			pch = strtok(NULL, " ");
			File* fd;
			fd = fopen(filename, "r");
			if (fd)
			{
				while ((n=fread(sendbuf,sizeof(char),BUFSIZE, (FILE *)fd))>0)
				{
					write(clfd, sendbuf, n);
				}
				fclose(fd);
			}
			write(clfd, sendbuf, 0);
			
		}
		else if (strlen(pch) != 0 && strcmp(pch, "LIST")) {
			DIR *dp;
			struct dirent *dir;
			d = opendir(filepath);
			if (d) {
				while ((dir = readdir(d)) != NULL) {
					sprint(sendbuf, "%s\n", dir->d_name);
					write(clfd, sendbuf, strlen(sendbuf));
				}
				closedir(d)
			}
		}
	}
	if(read_size==0)
		puts("Disconnected");
	fflush(stdout);
	free(sockfd);
	pthread_exit(0);
}

int set_config()
{
	FILE *fp;
	fp = fopen("dfs.conf", "r");
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
		strcpy(username[index], pch);
		pch = strstok(NULL, "");
		strcpy(password[index++], pch);
	}
	fclose(fp);
	return 0;
}