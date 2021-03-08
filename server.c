#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<string.h>
#include<pthread.h>
#include<sys/stat.h>
#include<unistd.h>
#include<fcntl.h>
#include<string.h>
#include <dirent.h>
#include <limits.h>
#include <sys/wait.h>
#include "structura.h"
#include "disc.h"
#define MAX_SIZE 10

// chmod 700 server.c | gcc -Wall -D_REENTRANT -pthread -o sv server.c structura.o disc.o
int ServerSocket;
struct sockaddr_in ServerAddress;
int ClientSocket[MAX_SIZE];
char smd[30];
struct ClientData *DataClient;
int jk=0;
int port=2424;
struct node *allFiles;
pid_t child[MAX_SIZE];
void sendMessage(char *ServerMessage,int i)
{	

	if(send(ClientSocket[i],ServerMessage,strlen(ServerMessage)+1,0)<0)
	{
		printf("Eroare la trimitere Client socket %d ",i);
		//exit(4);
	}
	
}

char * receiveMessage(int i)
{
	
	recv(ClientSocket[i],&smd,sizeof(smd),0);
	printf("%s has connected\n",smd);
	return smd;
	
}


// configure allFiles for inactive hosts as well
void addAllFilesInactiveHosts(char *directorName)
{	
	DIR *director;
	if((director = opendir(directorName)) == NULL)
	{
		printf("Eroare la opendir, nu s-a putut deschide directorul\n");
		exit(500);
	}

	struct dirent *intrareDirector;
	while((intrareDirector = readdir(director)) != NULL)
	{
		char numeIntrare[30];
		strcpy(numeIntrare, intrareDirector->d_name);

		if(strcmp(numeIntrare, "..")!=0 && strcmp(numeIntrare, ".")!=0)
		{
			char fullPath[50];
			snprintf(fullPath, sizeof(fullPath), "%s/%s", directorName, numeIntrare);

			struct stat buf;
			if(lstat(fullPath, &buf) < 0)
			{
				printf("Eroare la lstat (fullPath)\n");
				exit(501);
			}

			if(S_ISDIR(buf.st_mode))
				addAllFilesInactiveHosts(fullPath);
			else
			{
				if(S_ISREG(buf.st_mode) && strcmp(numeIntrare, "clientslist") != 0)
				{
					if(!FileAlreadyExistsInAllFiles(allFiles, numeIntrare))
						allFiles=AddToAllFiles(allFiles, numeIntrare);
				}
			}
		}
	} 
}

void commands(int i,char *Name)
{
	int sw=1;
	char command[30];
	struct stat info;
	while(sw)
	{
		memset(&command,0x00,sizeof(command));

		recv(ClientSocket[i],&command,sizeof(command),0);

		
		printf("%s : %s val=%d\n",Name,command,i);
		if(strcmp(command,"stop")==0)
			sw=0;

		if(strcmp(command,"add")==0)
		{		
				char file[30];
				memset(&file, 0x00, sizeof(file));
				recv(ClientSocket[i], &file, sizeof(file), 0);
				printf("%s is trying to host a file '%s'\n", Name, file);

				char clientDirFile[50];
				snprintf(clientDirFile, 50, "server/%s/%s", Name, file);

				if(access(file, F_OK) == 0 && access(clientDirFile, F_OK) != 0)
				{
					DataClient[i]=*AdaugareFisier(&DataClient[i],file);
					if(!FileAlreadyExistsInAllFiles(allFiles, file))
						allFiles=AddToAllFiles(allFiles, file);

					if(lstat(file,&info)<0)
					{
						printf("Eroare la functia stat %s\n",file);
						exit(99);
					}
						else
						if(S_ISREG(info.st_mode))
						{
								int fd1,fd2;
								int n;
								char *buff=malloc(sizeof(char)*1025);
								if((fd1=open(file,O_RDONLY))<0)
								{
									printf("Eroare la deschidere fisier\n");
									exit(75);
								}
								else
								{	
									snprintf(buff,50,"server/%s/%s",Name,file);

									if((fd2=open(buff,O_CREAT|O_WRONLY|O_EXCL,S_IRWXU))<0)
									{
										printf("Eroare la creare fisier\n");
										exit(69);
									}

									while((n=read(fd1,buff,1024))>0)
									{
										if(write(fd2,buff,n)<0)
										{
											printf("Eroare la scrierea in fisier\n");
											exit(24);
										}
									}
								}
							printf("File succesfully added.\n");
							close(fd1);
							close(fd2);
							free(buff);
						}
				}	else
						printf("Addition failed.\n");
		}


		if(strcmp(command,"seeFiles") == 0)
		{
			PrintareAllFiles(allFiles);

	    	char msj[1024], msjAux[20];
	    	struct node *p = allFiles;
	    	int c = 0;

	        while(p != NULL)
	        {
	        	if(c == 0)
	        	{
	        		strcpy(msj, p->NumeFisier);
	        		p = p ->urm;
	        		c = 1;
	        	}
	        	else
	        	{
	        		strcpy(msjAux, " | ");
	        		strcat(msjAux, p -> NumeFisier);
	        		strcat(msj, msjAux);
	          		p = p -> urm;
	        	}
	        }
	        	send(ClientSocket[i], msj, strlen(msj)+1, 0);
		}
		
		
	}

}
void newUser(char *Name, struct ClientData * DataClient,int i)
{	int sw=1;
	DIR *director,*director2;
	struct dirent *dir,*dir2;
	struct stat info,info2;
	char *name = malloc(sizeof(char)*30);
	char *path = malloc(sizeof(char)*50);
	if(!(director=opendir("server")))
	{
		printf("Eroare la deschiderea directorului server\n");
		exit(9);
	}
	while((dir=readdir(director))>0 && sw)
	{
		strcpy(name,dir->d_name);
			if(strcmp(name, ".") == 0 || strcmp(name, "..") == 0 )
					continue;
		snprintf(path,49,"server/%s",name);
	
		if(lstat(path,&info)<0)
		{
			printf("Eroare la functia stat\n");
			exit(10);
		}

		if(S_ISDIR(info.st_mode) && (strcmp(Name,name)==0))
		{
			printf("director existent, utilizator vechi\n");
			sw=0;
			if(!(director2=opendir(path)))
			{
				printf("Eroare la deschiderea directorului 2\n");
				exit(12);
			}
			char *names=malloc(sizeof(char)*50);
			char *path2=malloc(sizeof(char)*50);
			while((dir2=readdir(director2))>0)
			{
				strcpy(names,dir2->d_name);
				if(strcmp(names, ".") == 0 || strcmp(names, "..") == 0 )
					continue;
				snprintf(path2,49,"%s/%s",path,names);
				printf("path to files : %s \n",path2);
				
				
				if(lstat(path2,&info2)<0)
				{
					printf("Eroare la lstat in dir 2\n");
					exit(13);
				}
				if(S_ISREG(info2.st_mode))
				{
						printf("Adaugare \n");
					DataClient[i]=*AdaugareFisier(&DataClient[i],names);
				}


			}
			closedir(director2);
			free(names);
			free(path2);

		}
		
		
		

	}
	if(sw==1)
		{
			printf("Utilizator nou, generam director!\n");

			pid_t child;

			if((child=fork())<0)
			{
				printf("Eroare la proces xD\n");
				exit(112);
			}else
			if(child==0)
			{
				char *xD=malloc(sizeof(char)*112);
				snprintf(xD,100,"server/%s",Name);
				execlp("mkdir","mkdir",xD,NULL);
			}


		}
	closedir(director);
	free(name);
	free(path);
}

void threadClient( int i )
{	

	char Name[25];
	
	jk++;
	printf("i=%d jk=%d\n",i,jk);
	strcpy(Name,receiveMessage(i));
	sendMessage("Connected!",i);	
	DataClient[i]=*CreereClient(Name);
	newUser(Name,DataClient,i);
	PrintareClient(&DataClient[i]);
	commands(i,Name);
	printf("%s has disconnected!\n",Name);
	sendMessage("stop",i);
	exit(0);
	
}

void WaitForOtherClients()
{		
	
	socklen_t addrsize = sizeof ServerAddress;
	while(1)
	{
		if( (ClientSocket[jk] = accept(ServerSocket,(struct sockaddr *)&ServerAddress,&addrsize)) == -1)
			printf("Accepted failed at %d",jk);
		else
			{		
					printf("A intrat\n");
					
					if((child[jk]=fork())<0)
					{
						printf("Eroare la creere proces\n");
						exit(5);
					}
					else
						if(child[jk] ==0)
							threadClient(jk);
						else
							{jk++;
						WaitForOtherClients();
						for(int j=0;j<jk;j++)
						{	int status;
							wait(&status);
 							printf("Child ended with code %d\n", WEXITSTATUS(status));}
					}						
				
			}
		
		
		
		
	}
		

}

void createConnection()
{
	DataClient=malloc(sizeof(struct ClientData)*MAX_SIZE);
	ServerSocket = socket(AF_INET,SOCK_STREAM,0);
	ServerAddress.sin_family = AF_INET;
	ServerAddress.sin_port = htons(port);
	ServerAddress.sin_addr.s_addr = INADDR_ANY;
	bind(ServerSocket,(struct sockaddr *)&ServerAddress,sizeof(ServerAddress));
	listen(ServerSocket,5);
	WaitForOtherClients();
	
}





int main()
{
	
	checkdir();
	createConnection();
	return 0;
}