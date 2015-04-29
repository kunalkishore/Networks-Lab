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
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

//#define CLI_DIR "files_client"
#define CLI_DIR "."
#define MSG_SIZE 100

bool send_msg(char *cmd , char* file_name , int sockfd)
{
    //message format is "ARG1#ARG2;"
    char msg[100];
    int n;
    strcpy(msg,cmd);
    strcat(msg,"#");
    strcat(msg,file_name);
    strcat(msg,";");
    if( (n = write(sockfd,msg,strlen(msg)))== strlen(msg) )
        return true;
    else
        return false;

}

bool check_file(char *file_name)
{
     //to check if this file already exist in directory!
    DIR *dirp;
    struct dirent *dp;
    dirp = opendir(CLI_DIR);
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

bool read_msg(int sockfd)
{
    //message format is "ARG1;"
    char buf_msg[100];
    int p=0,n,flag=0;
    n=read(sockfd,buf_msg,1);
    if (n==0) flag=1;
    while (flag==0 && buf_msg[p]!=';'){
        p++;
        n=read(sockfd,&buf_msg[p],1);
        if(n==0) flag=1;
    }

    //printf("The message was: \n%s\n\n",buf_msg);

    char *tmp = strdup(buf_msg);
    char *cmd = strdup(buf_msg);
    cmd = strtok(tmp, ";");

    if(strcmp(cmd,"EXIST")==0)
        return true;
    else if(strcmp(cmd,"SUCCESS")==0)
        return false;
    else if(strcmp(cmd,"FAILURE")==0)
        return false;
    else if(strcmp(cmd,"OK")==0)
        return true;
    else if(strcmp(cmd,"ABORT")==0)
        return false;
     else if(strcmp(cmd,"DONE")==0)
        return true;
    else
        return false;
}

bool send_file(int sockfd, char* file_name){
    FILE *my_file;
    char buf[MSG_SIZE];
    struct stat st;
    int size[1];
    stat(file_name, &st);
    size[0] = st.st_size;
    my_file = fopen(file_name, "r");
    if(my_file == NULL){
        printf("ERROR: File %s not found.\n", file_name);
        exit(1);
    }
    int n, total;
    total =0;
    bzero(buf, MSG_SIZE);
    send_msg("SEND", file_name, sockfd);
    //printf("%d", size);
    if(read_msg(sockfd)){
        //ready to send the filechrome
        send(sockfd, (char *)size, sizeof(int), 0);
        while((n = fread(buf, sizeof(char), MSG_SIZE, my_file)) > 0){
            if(send(sockfd, buf, n, 0) < 0){
                printf("Error in sending %s.\n", file_name);
                break;
            }
            total = total + n;
            //printf("%s\n\n",buf);
            bzero(buf, MSG_SIZE);
        }
        printf("Total bytes sent are: %d\n",total);
    }
    fclose(my_file);
    return true;
}

bool read_file(int sockfd , char* file_name)
{
    //the server replies to the client
    send_msg("SEND",file_name,sockfd);
    //Now the server will read the file

    FILE *my_file;
    my_file = fopen(file_name,"w");

	int n,total=0,flag=0;
	char buf[MSG_SIZE];
	while(flag==0 && (n=recv(sockfd,buf,MSG_SIZE,0))>0)
	{
        fwrite(buf, sizeof(char), n, my_file);
        total = total + n;
        //printf("%s\n\n",buf);
        bzero(buf, MSG_SIZE);
        if(n%MSG_SIZE!=0)
            flag = 1;
	}

    printf("Total bytes read are : %d\n",total);

    fclose(my_file);
	return true;
}

bool mput(int sockfd, char * file_extension)
{
    DIR *dirp;
    struct dirent *dp;
    struct stat sb;
    dirp = opendir(CLI_DIR);
    if (dirp == NULL) {
        printf("opendir failed!");
            return false;
    }

    for (;;) {
        dp = readdir(dirp);
        if (dp == NULL)
            break;

        printf("File is %s\n",dp->d_name);

        if (lstat(dp->d_name, &sb) == -1)
 			printf("lstat Error!\n");
		else {
			 if (stat(dp->d_name, &sb) == -1)
 				printf("stat Error!\n");;
		}

		if(S_ISREG((&sb)->st_mode))     //Then this is a regular file!
		{
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

            //printf("file name: %s\n",file_name);

            if(c>1 && strcmp(extension,file_extension)==0)
            {
                //we need to send this file!!
                //this code is same as the code to send a single file!
                send_msg("put",file_name,sockfd);
                bool check = read_msg(sockfd);

                if(check==true)
                {
                    //file exist
                    //Check whether user wants to rewrite the code or not
                    printf("The file already exist. Do you want to rewrite it?(Y=1/N=0): \n");
                    int user_in;
                    scanf("%d",&user_in);
                    if(user_in==1)
                    {
                        //rewrite the file
                        printf("rewriting the file: ");
                        if(send_file(sockfd,file_name))
                            printf("file sent successfully!\n");
                        int d = dup(sockfd);
                        bool done = read_msg(d);
                        if(done == true)
                            printf("Reply from server is : DONE\n");
                    }
                    else
                    {
                        printf("Do nothing! ");
                        //Do nothing
                    }
                }
                else
                {
                    //THE FILE does not exist
                    printf("The file do not exist on the server. Sending the file: ");
                    if(send_file(sockfd,file_name))
                            printf("file send successfully!");
                    int d = dup(sockfd);
                    bool done = read_msg(d);
                    if(done == true)
                        printf("Reply from server is : DONE\n");
                }
            }
		}

    }

    return true;
}

bool mget(int sockfd , char* file_extension)
{
   send_msg("mget",file_extension,sockfd);

    int flag = 0;
    //we need to read the name of the file
    char buf_msg[100];
    int p=0,n;
    n=read(sockfd,buf_msg,1);
    if (n==0) flag=1;
    while (flag==0 && buf_msg[p]!=';'){
        p++;
        n=read(sockfd,&buf_msg[p],1);
        if(n==0) flag=1;
    }
    printf("out: %s\n",buf_msg);
    char *tmp = (char*)(malloc(100));
    char *cmd = (char*)(malloc(100));
    strcpy(tmp,buf_msg);
    strcpy(cmd,buf_msg);
    cmd = strtok(tmp, ";");

    while(strcmp(cmd,"ABORT")!=0)
    {
        //some file is being sent
        //strcpy(file_name,cmd);
        printf("File sending: %s\n",cmd);

        //Here we need to check if the file exist already or not and whether we want to rewrite the file or not
        bool check = check_file(cmd);

        if(check == false)
        {
            printf("The file already exist. Do you want to rewrite it?(Y=1/N=0): \n");
            int user_in;
            scanf("%d",&user_in);
            if(user_in==1)
            {
                //rewrite the file
                printf("rewriting the file: ");
                if(read_file(sockfd,cmd))
                {
                    printf("The file %s has been read successfully!",cmd);
                }
                send_msg("DONE"," ",sockfd);
            }
            else
            {
                printf("Do nothing! ");
                //Do nothing
                //send a message to server that file is not being send!
                send_msg("NOT_SENDING",cmd,sockfd);
            }
        }
        else
        {
            //the file doesn't exist.
            if(read_file(sockfd,cmd))
            {
                printf("The file %s has been read successfully!",cmd);
            }
            send_msg("DONE"," ",sockfd);
        }

        bzero(buf_msg, 100);
        bzero(tmp, 100);
        bzero(cmd, 100);
        int p=0,n;
        flag=0;
        n=read(sockfd,buf_msg,1);
        if (n==0) flag=1;
        while (flag==0 && buf_msg[p]!=';'){
            p++;
            n=read(sockfd,&buf_msg[p],1);
            if(n==0) flag=1;
        }
        printf("out: %s\n",buf_msg);
        strcpy(tmp,buf_msg);
        strcpy(cmd,buf_msg);
        cmd = strtok(tmp, ";");
    }

    return true;
}
