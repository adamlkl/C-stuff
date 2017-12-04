#include <iostream>
#include "option_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
using namespace std;

void help_usage (int n){
    if (n==0)exit(0);
    else{
    cout <<"usage: ./knock -h host -p port [-H] [-w] [-f file]" << endl;
    cout << "-------------------ListingOptions-------------------------" << endl;
    cout << "h, host: the DNS name or IP address of the host to contact" << endl;
    cout << "p, port: the TCP port on the server to which to connect" << endl;
    cout << "w, web: make an HTTP GET request for the \"/\" resource" << endl;
    cout << "f, file: store any output received from socket in file " << endl;
    cout << "H, help: produce a usage message such as that below" << endl;
    cout << "?, help: produce a usage message such as that below" << endl;
    }
}

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int main (int argc, char **argv){
    OptionHandler::Handler h = OptionHandler::Handler (argc,argv);
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    sockfd=socket(AF_INET,SOCK_STREAM,0);

    try{
        h.add_option('h', "host",OptionHandler::REQUIRED, false);
        h.add_option('p', "port",OptionHandler::REQUIRED, false);
        h.add_option('w', "web",OptionHandler::OPTIONAL, false);
        h.add_option('f', "file",OptionHandler::REQUIRED, false);
        h.add_option('H',"help",OptionHandler::NONE,false);
        h.add_option('?', "help",OptionHandler::NONE, false);
        }catch (const std::exception &e) {
            std::cerr << "exception caught: " << e.what() << std::endl;
         }

    if (h.get_option("help")||h.get_option("H")||h.get_option("?"))
    {
        help_usage(1);
        return 0;
    }
  
    std::string host;
     if (h.get_option("host")||h.get_option("h"))
    {
        host = h.get_argument("host");
        server = gethostbyname(host.c_str());
        if (server == NULL) {
            fprintf(stderr,"ERROR, no such host\n");
            exit(0);
        }
    }
    else 
    {
        help_usage(0);
        exit(0);
    } 
    
    std::string port;
    if (h.get_option("port")||h.get_option("p"))
    {
        port =  h.get_argument("port");;
        portno = atoi(port.c_str());
    }
    else 
    {
        help_usage(0);
        exit(0);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);

    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
    {
        error("ERROR connecting");
        exit(0);
    }

     std::string web;
     char buffer [1024];
     char *pointer;
    if (h.get_option("web")||h.get_option("w"))
    {
        web=h.get_argument("web");
    }
    else 
    {
        web="\r\n\r\n";
    }

    snprintf(buffer,1024,
    "GET /%s HTTP/1.1\r\n"
    "Host: %s\r\n"
    "\r\n",web.c_str(),host.c_str());
 
    n = write(sockfd, buffer, strlen(buffer));
    if (n < 0) 
    {
         error("ERROR writing to socket");
         exit(0);
    }
    bzero(buffer,1024);
    n = read(sockfd, buffer, 1024);
    if (n < 0) 
    {
         error("ERROR reading from socket");
         exit(0);
    }

    std::string file;
    if (h.get_option("file")||h.get_option("f"))
    {
        file = h.get_argument("file");
        FILE *fileA;
        fileA=fopen(file.c_str(),"w");
        fprintf(fileA,"%s", buffer);
    }
    else 
    {
         fprintf(stdout,"%s", buffer);
    }
    close(sockfd);
    return 0;
}

