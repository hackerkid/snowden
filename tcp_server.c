#include <stdio.h>
#include <string.h>    //strlen
#include <stdlib.h>    //strlen
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>    //write
#include <pthread.h> //for threading , link with lpthread
#include <iostream>
#include "proxy_parse.h"
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
 
/*
 * This will handle connection for each client
 * */
void *connection_handler(void *socket_desc)
{
    //Get the socket descriptor
    int sock = *(int*)socket_desc;
    int read_size;
    char  client_message[2000];
    char message[] = "Now type something and i shall repeat what you type \n";
     
    //Send some messages to the client
     
    write(sock , message , strlen(message));
     
    //Receive a message from client
    int ok;
    ok = 1;
    while( ok and ((read_size = recv(sock , client_message , 2000 , 0)) > 0) )
    {
        //end of string marker
        client_message[read_size] = '\0';
        
        //Send the message back to client
        write(sock , client_message , strlen(client_message));

        //char *tempmessage;

      //  strcpy(tempmessage, client_message);
        cout << client_message << endl; 
        
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
        
        cout << "here comes the king = >";
        for (int i = 0; i < *serverRequestLength; i++) {
            cout << serverRequest[i];
        }
        cout << endl;
        memset(client_message, 0, 2000);
        ok = 0;

    }

    cout << "out yahoo\n";
    close(sock);

     
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
