#include "client_attr_new.c"

bool validate()
{
    return true;
}

int main(int argc, char *argv[])
{
	char file_name[100];
	char server_ip[100];
	char server_port_temp[10];
	int server_port;
	if(argc == 5)
	{
		if( strcmp(argv[1],"get")==0 || strcmp(argv[1],"put")==0 || strcmp(argv[1],"mget")==0 || strcmp(argv[1],"mput")==0)
		{
			strcpy(file_name,argv[2]);
            strcpy(server_ip,argv[3]);
            strcpy(server_port_temp,argv[4]);
            server_port = atoi(server_port_temp);
		}
		else
		{
            printf("invalid input!");
            return -1;
        }
	}
	else
    {
        printf("invalid input!");
        return -1;
    }

    printf("%s\n",file_name);
	printf("%s\n",server_ip);
	printf("%s\n",server_port_temp);

    //connecting to the server
	int sockfd = 0;
	struct sockaddr_in serv_addr;
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0))< 0)
    {
        printf("\n Error : Could not create socket \n");
        return 1;
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server_port);
    serv_addr.sin_addr.s_addr = inet_addr(server_ip);

    if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0)
    {
        printf("\n Error : Connect Failed \n");
        return 1;
    }
    else
        printf("connection made!\n");


    if( strcmp(argv[1],"put")==0)
    {
        //start by sending a simple message
        send_msg(argv[1],argv[2],sockfd);

        bool check = read_msg(sockfd);  //to check if server already has that file or not!
        if(check==true)
        {
            //file exist
            //Check whether user wants to rewrite the code or not
            printf("The file already exist. Do you want to rewrite it?(Y=1/N=0): ");
            int user_in;
            scanf("%d",&user_in);
            if(user_in==1)
            {
                //rewrite the file
                printf("rewriting the file...\n");
                if(send_file(sockfd,argv[2]))
                    printf("File sent successfully!\n");
            }
            else
            {
                printf("Not sending the file! \n");
                //send a message to server that file is not being send!
                send_msg("NOT_SENDING",file_name,sockfd);
            }
        }
        else
        {
            //THE FILE does not exist
            printf("The file do not exist on the server. Sending the file: ");
            if(send_file(sockfd,argv[2]))
                    printf("file send successfully!");
        }
    }
    else if( strcmp(argv[1],"get")==0)
    {
        send_msg(argv[1],argv[2],sockfd);
        bool check = read_msg(sockfd);

        if(check == true)
        {
            //the file exist on the server
            if(read_file(sockfd,argv[2]))
                printf("File read successfully!\n");
        }
        else
        {
            //the file doesn't exist on the server
            //abort the system
            printf("The file doesn't exist on the server!\n");
        }

    }
    else if( strcmp(argv[1],"mput")==0)
    {
        //here we need to send all the files with a given extension
        char file_extension[100];
        strcpy(file_extension,file_name);
        printf("We need to send the file of extension %s \n",file_extension);
        send_msg(argv[1]," ",sockfd);
        if(mput(sockfd,file_extension))
            printf("All the files with the extension %s has been sent!",file_extension);
        //send the message that client can now abort the services!
        send_msg("ABORT"," ",sockfd);
    }
    else if( strcmp(argv[1],"mget")==0)
    {
        char file_extension[100];
        strcpy(file_extension,file_name);
        printf("We need to get the file of extension %s \n",file_extension);
        if(mget(sockfd,file_extension))
            printf("All the files with the extension %s has been sent!",file_extension);
    }

	return 0;
}
