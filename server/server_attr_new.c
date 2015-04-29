#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

//#define SERV_DIR "files_server"
#define SERV_DIR "."
#define MSG_SIZE 100

void signalHandler(int signal)
{
	printf("Caught signal %d!\n",signal);
	if (signal==SIGCHLD) {
		printf("Child ended\n");
		wait(NULL);
	}
}

struct message
{
    char cmd[10];
    char file_name[100];
    bool exist;             //true if file doesn't exist on server
};

bool check_file(char *file_name)
{
     //to check if this file already exist in directory!
    DIR *dirp;
    struct dirent *dp;
    dirp = opendir(SERV_DIR);
    if (dirp == NULL) {
        printf("opendir failed!");
            return false;
    }
    bool flag = true;
    for (;;) {
        dp = readdir(dirp);
        if (dp == NULL)
            break;
        if(strcmp(dp->d_name,file_name)==0)
            flag = false;
    }
    if(flag==true)
        return true;    //the file doesn't exist!
    else
        return false;
}


struct message read_msg(int connfd)
{
    //message format is "ARG1#ARG2;"
    struct message r;
    char buf_msg[100];
    int p=0,n,flag=0;
    n=read(connfd,buf_msg,1);
    if (n==0) flag=1;
    while (flag==0 && buf_msg[p]!=';'){
        p++;
        n=read(connfd,&buf_msg[p],1);
        if(n==0) flag=1;
    }
    //printf("%s\n",buf_msg);

    char *tmp = strdup(buf_msg);
    char *cmd = strdup(buf_msg);
    char *file_name = strdup(buf_msg);

    cmd = strtok(tmp, "#");
    file_name = strtok(NULL, ";");

    strcpy(r.cmd,cmd);
    strcpy(r.file_name,file_name);
    if(strcmp(r.cmd,"mget")==0 || strcmp(r.cmd,"mput")==0 || strcmp(r.file_name," ")==0)     //because here we are sending the file extension and not the file name
        r.exist = false;
    else
        r.exist = check_file(file_name);
    return r;
}

void send_msg(int connfd , char *m)
{
    //message format is "ARG1;"
    char msg[100];
    strcpy(msg,m);
    strcat(msg,";");
    //printf("message is %s\n",msg);
    write(connfd,msg,strlen(msg));
    return;
}

bool read_file(int connfd,char* file_name)
{
    //the server replies to the client
    send_msg(connfd,"OK");
    //Now the server will read the file
    FILE *my_file;
    my_file = fopen(file_name,"w");

	int n,total=0,flag=0;
	char buf[MSG_SIZE];
    int size[1];
    if((n=recv(connfd, size, sizeof(int), 0)) <= 0)
        printf("Error in receiving the length of file\n");

	while((n=recv(connfd,buf,MSG_SIZE,0)) > 0)
	{
        //printf("%d\n", n);
        fwrite(buf, sizeof(char), n, my_file);
        ///printf("%d\n", n);
        total = total + n;
        //printf("%s\n\n",buf);
        bzero(buf, MSG_SIZE);
        // if(n%MSG_SIZE!=0)
        //     flag = 1;
        if(total == size[0])
            break;
	}

    printf("Total bytes read are : %d\n",total);

    int d = dup(connfd);
    send_msg(d,"DONE");

    //fclose(my_file);
	return true;
}

bool send_file(int connfd, char* file_name)
{
    FILE *my_file;
    char buf[MSG_SIZE];
    my_file = fopen(file_name, "r");
    if(my_file == NULL){
        printf("ERROR: File %s not found.\n", file_name);
        exit(1);
    }
    int n, total;
    total =0;
    bzero(buf, MSG_SIZE);


    while((n = fread(buf, sizeof(char), MSG_SIZE, my_file)) > 0)
    {
        if(send(connfd, buf, n, 0) < 0){
            printf("Error in sending %s.\n", file_name);
            break;
        }
        total = total + n;
        //printf("%s\n\n",buf);
        bzero(buf, MSG_SIZE);
    }
    printf("Total bytes sent are: %d\n",total+1);
    fclose(my_file);
    return true;
}

bool mget(int connfd, char* file_extension)
{
    int count=0;        //to keep the count of number of files we are sending
    DIR *dirp;
    struct dirent *dp;
    struct stat sb;
    dirp = opendir(SERV_DIR);
    if (dirp == NULL) {
        printf("opendir failed!");
            return false;
    }

    for (;;) {
        dp = readdir(dirp);
        if (dp == NULL)
            break;

        if (lstat(dp->d_name, &sb) == -1)
 			printf("lstat Error!\n");
		else {
			 if (stat(dp->d_name, &sb) == -1)
 				printf("stat Error!\n");;
		}

		if(S_ISREG((&sb)->st_mode))
		{
            //Then this is a regular file!
             //All this has been done to avoid files with no "." or multiple "."
            int c=0;
            char *file_name = strdup(dp->d_name);
            char *tmp = strdup(dp->d_name);
            char *temp = strdup(dp->d_name);
            temp = strtok(tmp, ".");
            while(temp!=NULL)
            {
                c++;
                strcpy(tmp,temp);
                temp = strtok(NULL, ".");
            }

            char extension[100];
            strcpy(extension,".");
            strcat(extension,tmp);

            printf("file name: %s\n",file_name);
            //printf("file extension: %s\n",extension);
            //printf("file extension: %s\n",file_extension);

            if(c>1 && strcmp(extension,file_extension)==0)
            {
                //we need to send this file!!
                //this code is same as the code to send a single file!
                //hOwever we need to send the file name as a message to the client
                //sending the file
                send_msg(connfd,file_name);
                struct message ftp = read_msg(connfd);
                if(strcmp(ftp.cmd,"SEND")==0)
                {
                    //sending the file
                    if(send_file(connfd,ftp.file_name))
                        printf("file sent successfully!");
                    struct message done = read_msg(connfd);
                    if(strcmp(done.cmd,"DONE")==0)
                        printf("Message from client is: %s\n",done.cmd);
                }
                else //if the client send "EXIST"
                {
                    printf("Not sending the file!\n");
                }

                count++;
            }
		}

    }

    //the sending of all the files is complete!
    send_msg(connfd,"ABORT");

    if(count==0)
        return false;       //implies that not a single file was sent!
    return true;
}
