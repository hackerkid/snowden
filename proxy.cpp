#include <pthread.h>
#include <stdio.h>
#include <arpa/inet.h>
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

#define BUFFER 100000
#define MAXDATASIZE 2000000

using namespace std;


void *connection_handler(void *);
void *get_in_addr(struct sockaddr *sa);
inline int contactWebsite(char buff[], const char *PORT, const char * webhost, char output[]);
inline void sendTo (const char *request, int targetSocket, int clientSocket, int *length);
inline char *makeRequestForServer (struct ParsedRequest *requestStruct, int ClientSock, int *reqLen);
inline int createClientSocket(char *pcAddress, char *pcPort);
static int errorLength = 31;


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
        perror("bind failed. Error");
        return 1;
    }
     
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
       // puts("Handler assigned");
    }
     
    if (client_sock < 0)
    {
        perror("accept failed");
        return 1;
    }
     
    return 0;
}
 

void *connection_handler(void *socket_desc)
{
    int sock = *(int*)socket_desc;
    int read_size;
    char clientReq[100000];
    char buf[BUFFER];
    int count;
         
  
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
            sendTo("HTTP/1.0 500 INTERNAL ERROR\r\n\r\n", iClientfd, -1, &errorLength);
            shutdown(iClientfd, SHUT_RDWR);
            close(iClientfd);
            break;
            //exit(EXIT_FAILURE);
        }
        if (req->port == NULL) req->port = (char *) "80";

        /* Act as proxy between client and server */
        char * serverReq = makeRequestForServer(req, iClientfd, reqLen);

        char serv[] = "172.31.1.4";
        char port[] = "8080";
        int iServerfd = createClientSocket( serv, port);
        sendTo(serverReq, iServerfd, iClientfd, reqLen);
        //riteToClient(iClientfd, iServerfd);

         while ((count = recv(iServerfd, buf, BUFFER, 0)) > 0)
          sendTo(buf, iClientfd, iServerfd, &count);

        if (count < 0) {
            shutdown(iClientfd, SHUT_RDWR);
            shutdown(iServerfd, SHUT_RDWR);
            close(iClientfd);
            close(iServerfd);
            exit(EXIT_FAILURE);
        }

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
        //puts("Client disconnected");
        fflush(stdout);
    }
    else if(read_size == -1)
    {
        //perror("recv failed");
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

inline int contactWebsite(char buff[], const char *PORT, const char * webhost, char output[])
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
  //  printf("client: connecting to %s\n", s);

    freeaddrinfo(servinfo); // all done with this structure
    int length = strlen(buff);
    //cout << "man man " << buff << endl;
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
inline void sendTo (const char *request, int targetSocket, int clientSocket, int *length) {
    int recieved;
    int totalSend = 0;

    while (totalSend < *length) {
        if ((recieved = send(targetSocket, (void *) (request + totalSend), *length - totalSend, 0)) < 0) {
            perror("SEND error");
            shutdown(targetSocket, SHUT_RDWR);
            close(targetSocket);
            if (clientSocket != -1) {
                shutdown(clientSocket, SHUT_RDWR);
                close(clientSocket);
            }
            exit(EXIT_FAILURE);
        }
        totalSend += recieved;
    }
}


inline char *makeRequestForServer (struct ParsedRequest *requestStruct, int ClientSock, int *reqLen) {
    int headerLength;
    char *requestToServer;
    char *headersString;

    ParsedHeader_set(requestStruct, "Host", requestStruct->host);
    ParsedHeader_set(requestStruct, "Connection", "close");

    headerLength = ParsedHeader_headersLen(requestStruct);
    headersString = (char *) malloc(headerLength + 1);
    if (headersString == NULL) {
        sendTo("HTTP/1.0 500 INTERNAL ERROR\r\n\r\n", ClientSock, -1, &errorLength);
        shutdown(ClientSock, SHUT_RDWR);
        close(ClientSock);
        exit(EXIT_FAILURE);
    }

    ParsedRequest_unparse_headers(requestStruct, headersString, headerLength);
    headersString[headerLength] = '\0';

    *reqLen = strlen(requestStruct->method) + strlen(requestStruct->path) + strlen(requestStruct->version) + headerLength + 4;
    requestToServer = (char *) malloc(*reqLen + 1);
    if (requestToServer == NULL) {
        sendTo("HTTP/1.0 500 INTERNAL ERROR\r\n\r\n", ClientSock, -1, &errorLength);
        shutdown(ClientSock, SHUT_RDWR);
        close(ClientSock);
        exit(EXIT_FAILURE);
    }

    requestToServer[0] = '\0';
    string ans;

    ans = requestStruct->method;
    ans = ans + " ";
    ans = ans + requestStruct->path;
    ans = ans + " ";
    ans = ans + requestStruct->version;
    ans = ans + "\r\n";
    ans = ans + headersString;

    for (unsigned int i = 0; i < ans.length(); i++) {
        requestToServer[i] = ans[i];
    }

    requestToServer[ans.length()] = '\0';
    
    free(headersString);

    return requestToServer;
}


int createClientSocket2(char *host, char * port)
{
    int sockfd = 0;
    char recvBuff[1024];
    struct sockaddr_in serv_addr;


    memset(recvBuff, '0',sizeof(recvBuff));

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Error : Could not create socket \n");
        return 1;
    }

    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = atoi(port);

    if(inet_pton(AF_INET, host, &serv_addr.sin_addr)<=0)
    {
        printf("\n inet_pton error occured\n");
        return 1;
    }

   
    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\n Error : Connect Failed \n");
        return 1;
    }

    
    return sockfd;
}


inline int createClientSocket(char *pcAddress, char *pcPort) {
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

