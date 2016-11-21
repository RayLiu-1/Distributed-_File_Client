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

char DocumentRoot[200] = "/";
char server[4][100];
char serverIp[4][100];
char username[16];
char password[32];
int userindex = 0;

int set_server(int *sock, struct sockaddr_in *server, int index);
int set_config(char *file);
int hash_md5(char *file);
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
									strcpy(filelist[filesize], readbuf[i] + 3);
									int part = readbuf[i][1] - '1';
									int bitpart = 1 << part;
									filemark[filesize++] |= bitpart;
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
			if ((strncmp(commond, "PUT ", 4) == 0) && strlen(commond)>4) {
				char* pch = commond;
				pch = strtok(commond, " \n");
				pch = strtok(NULL, " \n");
				char filename[200];
				strncpy(filename, pch, 200);
				char subdir[200]="";
				pch = strtok(NULL, " \n");
				if (pch != NULL) {
					strncpy(subdir, pch, 200);
				}
				int hashvalue = hash_md5(filename);				

				FILE* fp = fopen(filename, "r");
				if (fp == NULL) {
					puts("Cann't find file");
					continue;
				}
				fseek(fp, 0L, SEEK_END);
				int filelenth = (int)ftell(fp);
				filelenth /= 4;
				printf("%d\n", filelenth);
				int filelenth1 = filelenth;
				rewind(fp);
				int i = (hashvalue + 4 - 1) % 4;
				int j = hashvalue;

				char filename1[200] = ".1.";
				strcat(filename1, filename);
				strcpy(sendbuf[i], "PUT ");
				strcat(sendbuf[i], filename1);
				strcat(sendbuf[i], subdir);
				write(sock[i], sendbuf[i], strlen(sendbuf[i])+ 1);
				write(sock[j], sendbuf[i], strlen(sendbuf[i]) + 1);
				int read = 0;
				do {
					if (BUFSIZE < filelenth) {
						read = fread(sendbuf[i], 1, BUFSIZE, fp);
						puts(sendbuf[i]);
						if(connect[i]==1)
							write(sock[i], sendbuf[i], read);
						if(connect[j]==1)
							write(sock[j], sendbuf[i], read);
						filelenth -= BUFSIZE;
					}
					else {
						read = fread(sendbuf[i], 1, filelenth, fp);
						puts(sendbuf[i]);
						if (connect[i] == 1)
							write(sock[i], sendbuf[i], read);						if (connect[j] == 1)
						if (connect[j] == 1)
							write(sock[j], sendbuf[i], read);
						filelenth = 0;
					}
				} while (filelenth > 0);
				recv(sock[i], readbuf[i], BUFSIZE, 0);
				puts(readbuf[i]);

				i = hashvalue;
				j = (hashvalue + 1) % 4;
				strcpy(filename1 , ".2.");
				strcat(filename1, filename);
				strcpy(sendbuf[i], "PUT ");
				strcat(sendbuf[i], subdir);
				strcat(sendbuf[i], filename1);
				write(sock[i], sendbuf[i], strlen(sendbuf[i]) + 1);
				write(sock[j], sendbuf[i], strlen(sendbuf[i]) + 1);
				filelenth = filelenth1;
				do {
					if (BUFSIZE < filelenth) {
						read = fread(sendbuf[i], 1, BUFSIZE, fp);
						if (connect[i] == 1)
							write(sock[i], sendbuf[i], read);
						if (connect[j] == 1)
							write(sock[j], sendbuf[i], read);
						filelenth -= BUFSIZE;
					}
					else {
						read = fread(sendbuf[i], 1, filelenth, fp);
						if (connect[i] == 1)
							write(sock[i], sendbuf[i], read);						if (connect[j] == 1)
						if (connect[j] == 1)
							write(sock[j], sendbuf[i], read);
						filelenth = 0;
					}
				} while (filelenth > 0);
				write(sock[i], sendbuf[i], 0);
				write(sock[j], sendbuf[i], 0);
				recv(sock[i], readbuf[i], BUFSIZE, 0);
				puts(readbuf[i]);
				i = (hashvalue+1)%4;
				j = (hashvalue + 2) % 4;
				strcpy(filename1 , ".3.");
				strcat(filename1, filename);
				strcpy(sendbuf[i], "PUT ");
				strcat(sendbuf[i], subdir);
				strcat(sendbuf[i], filename1);
				write(sock[i], sendbuf[i], strlen(sendbuf[i]) + 1);
				write(sock[j], sendbuf[i], strlen(sendbuf[i]) + 1);
				filelenth = filelenth1;
				do {
					if (BUFSIZE < filelenth) {
						read = fread(sendbuf[i], 1, BUFSIZE, fp);
						if (connect[i] == 1)
							write(sock[i], sendbuf[i], read);
						if (connect[j] == 1)
							write(sock[j], sendbuf[i], read);
						filelenth -= BUFSIZE;
					}
					else {
						read = fread(sendbuf[i], 1, filelenth, fp);
						if (connect[i] == 1)
							write(sock[i], sendbuf[i], read);						if (connect[j] == 1)
							if (connect[j] == 1)
							write(sock[j], sendbuf[i], read);
						filelenth = 0;
					}
				} while (filelenth > 0);
				recv(sock[i], readbuf[i], BUFSIZE, 0);
				puts(readbuf[i]);

				i = (hashvalue + 2) % 4;
				j = (hashvalue + 3) % 4;
				strcpy(filename1 ,".4.");
				strcat(filename1, filename);
				strcpy(sendbuf[i], "PUT ");
				strcat(sendbuf[i], subdir);
				strcat(sendbuf[i], filename1);
				write(sock[i], sendbuf[i], strlen(sendbuf[i]) + 1);
				write(sock[j], sendbuf[i], strlen(sendbuf[i]) + 1);
				filelenth = filelenth1;
				while (read = fread(sendbuf[i], 1, BUFSIZE, fp)>0) {
					if (connect[i] == 1)
						write(sock[i], sendbuf[i], read);
					if (connect[j] == 1)
						write(sock[j], sendbuf[i], read);
				} 
				recv(sock[i], readbuf[i], BUFSIZE, 0);
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

int hash_md5(char *file) {
	char cmd[200];
	sprintf(cmd, "md5sum %s", file);
	FILE *fp = popen(cmd, "r");
	char md5[200];
	fscanf(fp, "%s", &md5);
	//printf("%s", md5);
	char lbit[2];
	strncpy(lbit, md5+ strlen(md5) - 1, 1);
	fclose(fp);
	return strtol(lbit, NULL, 16) % 4;
}

//void *connection_handler(void *sockfd) {}