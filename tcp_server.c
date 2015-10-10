#include <stdio.h>
#include <arpa/inet.h> //inet_addr
#include <pthread.h> //for threading , link with lpthread
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <fstream>
#include "proxy_parse.h"

#define MAXDATASIZE 2000000

using namespace std;

void *connection_handler(void *);

static char *makeRequestForServer (struct ParsedRequest *req, int *requestLength) {
    int headerLength;
    char *serverRequest;
    char *finalHeaders;

    /* Set headers */
    ParsedHeader_set(req, "Host", req->host);
    ParsedHeader_set(req, "Connection", "close");

    /* Prepare the headers that the client gave */
    headerLength = ParsedHeader_headersLen(req);
    finalHeaders = (char *) malloc(headerLength + 1);
    if (finalHeaders == NULL) {
        exit(EXIT_FAILURE);
    }

    ParsedRequest_unparse_headers(req, finalHeaders, headerLength);
    finalHeaders[headerLength] = '\0';

    /* Allocate memory for request to server */
    *requestLength = strlen(req->method) + strlen(req->path) + strlen(req->version) + headerLength + 4;
    serverRequest = (char *) malloc(*requestLength + 1);
    if (serverRequest == NULL) {
        exit(EXIT_FAILURE);
    }

    /* Build the request */
    serverRequest[0] = '\0';
    strcpy(serverRequest, req->method);
    strcat(serverRequest, " ");
    strcat(serverRequest, req->path);
    strcat(serverRequest, " ");
    strcat(serverRequest, req->version);
    strcat(serverRequest, "\r\n");
    strcat(serverRequest, finalHeaders);

    free(finalHeaders);

    return serverRequest;
}

int main(int argc , char *argv[])
{
    int socket_desc , client_sock , c;
    struct sockaddr_in server , client;
     
    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");
     
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( 8888 );
     
    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        perror("bind failed. Error");
        return 1;
    }
    puts("bind done");
     
    //Listen
    listen(socket_desc , 3);
     
    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);
     
     
    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);
    pthread_t thread_id;
    
    while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
    {
        puts("Connection accepted");
         
        if( pthread_create( &thread_id , NULL ,  connection_handler , (void*) &client_sock) < 0)
        {
            perror("could not create thread");
            return 1;
        }
         
        //Now join the thread , so that we dont terminate before the thread
        //pthread_join( thread_id , NULL);
        puts("Handler assigned");
    }
     
    if (client_sock < 0)
    {
        perror("accept failed");
        return 1;
    }
     
    return 0;
}
 
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int contactWebsite(char buff[], const char *PORT, const char * webhost, char output[])
{
    int sockfd, numbytes;  
    char buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

   

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(webhost, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
            s, sizeof s);
    printf("client: connecting to %s\n", s);

    freeaddrinfo(servinfo); // all done with this structure
    int length = strlen(buff);

    if ((send(sockfd, buff, length, 0)) < 0) {
            perror("SEND error");
            shutdown(sockfd, SHUT_RDWR);
            close(sockfd);
           
            exit(EXIT_FAILURE);
        }

    char wow[MAXDATASIZE];
    string ans;
    long long count;
    count = 0;
    cout << " message send waiting for response \n";
    
    while ((numbytes = recv(sockfd, wow, MAXDATASIZE-1, 0)) > 0 ) {
        ans = ans + wow;
    //    cout << "=====" << wow << endl << endl;

        //cout << "lol =====" << ans << endl;
//        cout << numbytes << " " << wow[0] << wow[1] << wow[2] << endl;
        cout << numbytes << endl;
        count += numbytes;

        if(strstr(wow, "\r\n\r\n") == NULL) {
         //   break;
        }
        
        
    }
    cout << "total bytes " << count << endl;
   // cout << "epic \n \n " << ans << endl;

    for (unsigned int i = 0; i < ans.size(); i++) {
        output[i] = ans[i];
    }

    fstream fcout;
    fcout.open("out.html", ios::out);
    fcout << ans;

    output[ans.size() - 1] = '\0';
    buf[numbytes] = '\0';

    printf("client: received '%s'\n",buf);

   // close(sockfd);

    return 0;
}

/*
 * This will handle connection for each client
 * */
void *connection_handler(void *socket_desc)
{
    //Get the socket descriptor
    int sock = *(int*)socket_desc;
    int read_size;
    char  client_message[20000];
   // char message[] = "Now type something and i shall repeat what you type \n";
     
    //Send some messages to the client
     
  //  write(sock , message , strlen(message));
     
    //Receive a message from client
    int ok;
    ok = 1;
    while( ok and ((read_size = recv(sock , client_message , 20000 , 0)) > 0) )
    {
        //end of string marker
        client_message[read_size] = '\0';
        
        //Send the message back to client

        //char *tempmessage;

      //  strcpy(tempmessage, client_message);
      //  cout << "messa => " <<client_message << endl; 
        
        struct ParsedRequest *request;
        request = ParsedRequest_create();
        if (ParsedRequest_parse(request, client_message, strlen(client_message)) < 0) {
         //   writeToSocket(errMessage, iClientfd, -1, &errMessageLen);
          //  shutdown(iClientfd, SHUT_RDWR);
          //  close(iClientfd);
            cout << "invalid request\n";
            exit(EXIT_FAILURE);
        }

        if (request->port == NULL) request->port = (char *) "80";
        
        int garb;

        int * serverRequestLength = &garb;
        char * serverRequest = makeRequestForServer(request, serverRequestLength);
        
        cout << "final beautified request = >";
        //upto here we got the request from the client which has been parsed beautifully into server request

        char buff[2000];

        for (int i = 0; i < *serverRequestLength; i++) {
            cout << serverRequest[i];
            buff[i] = serverRequest[i];
        }

        buff[*serverRequestLength] = '\0';
        cout << endl << endl;
        memset(client_message, 0, 2000);
        ok = 0;
        
        char output[200000];

        contactWebsite(buff, "8080", "172.31.1.4", output);
        write(sock , output , strlen(output));
        cout << " almost done \n \n";


     //   cout << output << endl;

    }


    close(sock);
    cout << "closet closed \n";
     
    if(read_size == 0)
    {
        puts("Client disconnected");
        fflush(stdout);
    }
    else if(read_size == -1)
    {
        perror("recv failed");
    }

         
    return 0;
} 
