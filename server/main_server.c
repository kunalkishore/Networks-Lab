#include "server_attr_new.c"

int main(int argc, char *argv[])
{
    char server_port_temp[10];
	int server_port;
    if(argc == 2)
    {
        strcpy(server_port_temp,argv[1]);
        server_port = atoi(server_port_temp);
        printf("The port used is: %d\n",server_port);
    }
    else
    {
        //to set a default value of port
        server_port = 5000;
        printf("The default port used is: %d\n",server_port);
    }


    int listenfd = 0,connfd = 0;
    struct sockaddr_in serv_addr;
    int sin_size;
    struct sockaddr_in client_addr;
    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("Error while acquiring socket descriptor\n");
        exit(1);
    }
    // fcntl(listenfd, F_SETFL, O_NONBLOCK);
    //printf("socket retrieve success\n");

    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(server_port);

    if((bind(listenfd, (struct sockaddr*)&serv_addr,sizeof(serv_addr))) == -1){
        perror("bind error\n");
        exit(1);
    }

    if(listen(listenfd, 10) == -1){
        perror("Failed to listen\n");
        exit(1);
    }

    //The process of eliminating zombie processes
    if (signal(SIGCHLD, SIG_IGN) == SIG_ERR) {
        perror(0);
        exit(1);
    }

    while(1)
    {
        sin_size = sizeof(struct sockaddr_in);
        if((connfd = accept(listenfd, (struct sockaddr *) &client_addr ,&sin_size)) == -1) {
            perror("accept error\n");
            continue;
        }

        printf("Server: got connection from %s\n", inet_ntoa(client_addr.sin_addr));

        //create a child process
        if(!fork()){
            close(listenfd);
            struct message r = read_msg(connfd);

            if(strcmp(r.cmd,"put")==0)
            {
                //the code for uploading the file
                if(r.exist==true)
                {
                    //then file doesn't exist
                    send_msg(connfd,"SUCCESS");
                    struct message ftp = read_msg(connfd);
                    if(strcmp(ftp.cmd,"SEND")==0)
                    {
                        if(read_file(connfd,ftp.file_name))
                            printf("file read successfully!\n");
                        //send_msg(connfd,"DONE");
                    }
                }
                else
                {
                    //file exist
                    send_msg(connfd,"EXIST");
                    struct message ftp = read_msg(connfd);
                    if(strcmp(ftp.cmd,"SEND")==0)
                    {
                        if(read_file(connfd,ftp.file_name))
                            printf("file read successfully!\n");
                    }
                    else
                    {
                        printf("Not rewriting the file!\n");
                    }
                }
            }
            else if( strcmp(r.cmd,"get")==0)
            {
                if(r.exist==true)
                {
                    //the file doesn't exist on the server
                    send_msg(connfd,"FAILURE");
                }
                else
                {
                    //the file exist on the server
                    //the server sending a message saying that file exist
                    send_msg(connfd,"EXIST");
                    struct message ftp = read_msg(connfd);
                    if(strcmp(ftp.cmd,"SEND")==0)
                    {
                        //sending the file
                        if(send_file(connfd,ftp.file_name))
                            printf("file sent successfully!\n");
                    }
                    else
                    {
                        printf("Not sending the file!\n");
                    }
                }
            }
            else if( strcmp(r.cmd,"mput")==0)
            {
                //this is exactly same as put
                struct message first = read_msg(connfd);
                while(strcmp(first.cmd,"ABORT")!=0)
                {
                    if(first.exist==true)
                    {
                        //then file doesn't exist
                        send_msg(connfd,"SUCCESS");
                        struct message ftp = read_msg(connfd);
                        if(strcmp(ftp.cmd,"SEND")==0)
                        {
                            if(read_file(connfd,ftp.file_name))
                                printf("file read successfully!");
                            //send_msg(connfd,"DONE");
                        }
                    }
                    else
                    {
                        //file exist
                        send_msg(connfd,"EXIST");
                        struct message ftp = read_msg(connfd);
                        if(strcmp(ftp.cmd,"SEND")==0)
                        {
                            if(read_file(connfd,ftp.file_name))
                                printf("file read successfully!");
                            //send_msg(connfd,"DONE");
                        }
                        else
                        {
                            printf("Not rewriting the file!\n");
                        }
                    }
                    first = read_msg(connfd);
                    printf("Command from server is %s\n",first.cmd);
                }
                printf("All files are sent!\n");
            }
            else if( strcmp(r.cmd,"mget")==0)
            {
                //we will get a file extension here!
                char *file_extension = strdup(r.file_name);
                if(mget(connfd,file_extension))
                    printf("All the files with the extension %s has been sent!",file_extension);
                else
                {
                    //no files were sent!
                    printf("none of the file were of the desired extension. Abort");
                }
            }
            close(connfd);
            exit(0);
        }
        close(connfd);
        //printf("Server Closed.Going off to sleep!!\n");
        //sleep(1);
    }

    return 1;
}
