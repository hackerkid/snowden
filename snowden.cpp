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
static int errMessageLen = 31; /* Length of errMessage */

const char *errMessage = "HTTP/1.0 500 INTERNAL ERROR\r\n\r\n";

void *connection_handler(void *);



static int createClientSocket(char *pcAddress, char *pcPort) {
  struct addrinfo aHints, *paRes;
  int iSockfd;

  /* Get address information for stream socket on input port */
  memset(&aHints, 0, sizeof(aHints));
  aHints.ai_family = AF_UNSPEC;
  aHints.ai_socktype = SOCK_STREAM;
  if (getaddrinfo(pcAddress, pcPort, &aHints, &paRes) != 0) {
    perror("GETADDR error");
    exit(EXIT_FAILURE);
  }

  /* Create and connect */
  if ((iSockfd = socket(paRes->ai_family, paRes->ai_socktype, paRes->ai_protocol)) < 0) {
    perror("CREATE error");
    exit(EXIT_FAILURE);
  }
  if (connect(iSockfd, paRes->ai_addr, paRes->ai_addrlen) < 0) {
    perror("CONNECT error");
    exit(EXIT_FAILURE);
  }

  /* Free paRes, which was dynamically allocated by getaddrinfo */
  freeaddrinfo(paRes);

  return iSockfd;
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
 //   puts("Socket created");
     
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
  //  puts("bind done");
     
    //Listen
    listen(socket_desc , 3);
     
    //Accept and incoming connection
//    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);
     
     
    //Accept and incoming connection
  //  puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);
    pthread_t thread_id;
    
    while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
    {
      //  puts("Connection accepted");
         
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

//





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
    cout << "man man " << buff << endl;
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
   // cout << "total bytes " << count << endl;
   // cout << "epic \n \n " << ans << endl;

    for (unsigned int i = 0; i < ans.size(); i++) {
        output[i] = ans[i];
    }



    output[ans.size() - 1] = '\0';
    buf[numbytes] = '\0';

    fstream fcout;
    fcout.open("out.html", ios::out);
    fcout << ans;

    printf("client: received '%s'\n",buf);

   // close(sockfd);

    return 0;
}

/*
 * This will handle connection for each client
 * */
static void writeToSocket (const char *buf, int sockfd, int otherfd, int *len) {
    int iSent;
    int iTotalSent = 0;

    while (iTotalSent < *len) {
        if ((iSent = send(sockfd, (void *) (buf + iTotalSent), *len - iTotalSent, 0)) < 0) {
            perror("SEND error");
            shutdown(sockfd, SHUT_RDWR);
            close(sockfd);
            if (otherfd != -1) {
                shutdown(otherfd, SHUT_RDWR);
                close(otherfd);
            }
            exit(EXIT_FAILURE);
        }
        iTotalSent += iSent;
    }
}

 static void writeToClient (int iClientfd, int iServerfd) {
    enum {BUF_SIZE = 4096};

    int iRecv;
    char buf[BUF_SIZE];

    while ((iRecv = recv(iServerfd, buf, BUF_SIZE, 0)) > 0)
          writeToSocket(buf, iClientfd, iServerfd, &iRecv);

    /* Error handling */
    if (iRecv < 0) {
      //writeToSocket(errMessage, iClientfd, iServerfd, &errMessageLen);
      shutdown(iClientfd, SHUT_RDWR);
      shutdown(iServerfd, SHUT_RDWR);
      close(iClientfd);
      close(iServerfd);
      exit(EXIT_FAILURE);
    }
}
static char *getServerReq (struct ParsedRequest *req, int iClientfd, int *reqLen) {
    int iHeadersLen;
    char *serverReq;
    char *headersBuf;

    /* Set headers */
    ParsedHeader_set(req, "Host", req->host);
    ParsedHeader_set(req, "Connection", "close");

    /* Prepare the headers that the client gave */
    iHeadersLen = ParsedHeader_headersLen(req);
    headersBuf = (char *) malloc(iHeadersLen + 1);
    if (headersBuf == NULL) {
        writeToSocket(errMessage, iClientfd, -1, &errMessageLen);
        shutdown(iClientfd, SHUT_RDWR);
        close(iClientfd);
        exit(EXIT_FAILURE);
    }

    ParsedRequest_unparse_headers(req, headersBuf, iHeadersLen);
    headersBuf[iHeadersLen] = '\0';

    /* Allocate memory for request to server */
    *reqLen = strlen(req->method) + strlen(req->path) + strlen(req->version) + iHeadersLen + 4;
    serverReq = (char *) malloc(*reqLen + 1);
    if (serverReq == NULL) {
        writeToSocket(errMessage, iClientfd, -1, &errMessageLen);
        shutdown(iClientfd, SHUT_RDWR);
        close(iClientfd);
        exit(EXIT_FAILURE);
    }

    /* Build the request */
    serverReq[0] = '\0';
    strcpy(serverReq, req->method);
    strcat(serverReq, " ");
    strcat(serverReq, req->path);
    strcat(serverReq, " ");
    strcat(serverReq, req->version);
    strcat(serverReq, "\r\n");
    strcat(serverReq, headersBuf);

    free(headersBuf);

    return serverReq;
}


void *connection_handler(void *socket_desc)
{
    int sock = *(int*)socket_desc;
    int read_size;
    char clientReq[100000];
  
  //  int ok;
    int iClientfd = sock;
   // ok = 1;
    while(((read_size = recv(sock , clientReq , 20000 , 0)) > 0) )
    {
        clientReq[read_size] = '\0';
        
        
        struct ParsedRequest *req;
        int iReqLen = 0;
        int *reqLen = &iReqLen;

        /* A copy of iSockfd is useless in the child process */
        //close(iSockfd);

        

        /* Parse client request */
        req = ParsedRequest_create();
        if (ParsedRequest_parse(req, clientReq, strlen(clientReq)) < 0) {
            writeToSocket(errMessage, iClientfd, -1, &errMessageLen);
            shutdown(iClientfd, SHUT_RDWR);
            close(iClientfd);
            break;
            //exit(EXIT_FAILURE);
        }
        if (req->port == NULL) req->port = (char *) "80";

        /* Act as proxy between client and server */
        char * serverReq = getServerReq(req, iClientfd, reqLen);

        char serv[] = "172.31.1.4";
        char port[] = "8080";
        int iServerfd = createClientSocket( serv, port);
        writeToSocket(serverReq, iServerfd, iClientfd, reqLen);
        writeToClient(iClientfd, iServerfd);

        /* Free memory and clean up */
        ParsedRequest_destroy(req);
        free(serverReq);

        shutdown(iClientfd, SHUT_RDWR);
        shutdown(iServerfd, SHUT_RDWR);
        close(iClientfd);
        close(iServerfd);

     //   ok = 0;

         if(strstr(clientReq, "\r\n\r\n") == NULL) {
         //   break;
        }
        


    }


    close(sock);
   // cout << "closet closed \n";
     
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
