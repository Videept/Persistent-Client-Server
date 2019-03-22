#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <iostream>
#include <fstream>
#include <strings.h>
#include <string>
#include <pthread.h>
#include <sstream>

using namespace std;

void *comm(void *ptr);
std::string http_parsers(char* buffer2);
static int clientSockfd;
std::string url;
int sockfd = socket(AF_INET, SOCK_STREAM, 0);
int
main(int atgc,const char* argv[])
{

  pthread_t threads[15];
  // create a socket using TCP IP
 

  // allow others to reuse the address
  int yes= 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
    perror("setsockopt");
    return 1;
  }

  // bind address to socket
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(40000);     // short, network byte order
  addr.sin_addr.s_addr = INADDR_ANY;
  memset(addr.sin_zero, '\0', sizeof(addr.sin_zero));

  if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    perror("bind");
    return 2;
  }

  // set socket to listen status
  if (listen(sockfd, 5) == -1) {
    perror("listen");
    return 3;
  }

  int ThreadCount=0;
  while(ThreadCount < 10)
  {
  // accept a new connection
  /*struct sockaddr_in clientAddr;
  socklen_t clientAddrSize = sizeof(clientAddr);
  int clientSockfd = accept(sockfd, (struct sockaddr*)&clientAddr, &clientAddrSize);

  if (clientSockfd == -1) {
    perror("accept");
    return 4;
  }*/
	url=argv[1];  
  pthread_create(&threads[ThreadCount],NULL,comm,NULL);
  ThreadCount++;
  }
  for(int i = 0; i < 15; i++)
    {
        pthread_join(threads[i], NULL);
    }

  return 0;
  }
    


  void *comm (void *pt )
  {
  sleep(1);
  struct sockaddr_in clientAddr;
  socklen_t clientAddrSize = sizeof(clientAddr);
  int clientSockfd = accept(sockfd, (struct sockaddr*)&clientAddr, &clientAddrSize);

  if (clientSockfd == -1) {
   perror("accept");
  }
  
  char ipstr[INET_ADDRSTRLEN] = {'\0'};
  inet_ntop(clientAddr.sin_family, &clientAddr.sin_addr, ipstr, sizeof(ipstr));
  std::cout << "Accept a connection from: " << ipstr << ":" <<
    ntohs(clientAddr.sin_port) << std::endl;

  // read/write data from/into the connection
  bool isEnd = false;
  char buffer2[65536] = {0};
  std::stringstream ss;
int var=1;
// while (var) {
sleep(1);
    memset(buffer2, '\0', sizeof(buffer2));

    if (recv(clientSockfd, buffer2, 65536, 0) == -1) {
      perror("recv");
     // break;
    }

    ss << buffer2 << std::endl;
    std::cout << buffer2 << std::endl;

    FILE *inputf;
    long lSize;
    char * buffer1;
    size_t res;
std::string temp=url;
//strcpy((char*)temp.c_str(),(char*)url.c_str());
std::string ffile=strcat((char*)url.c_str(),http_parsers(buffer2).c_str());
std::cout<<ffile;
  inputf = fopen ( ffile.c_str(), "r" );
  if (inputf==NULL) {fputs ("File error",stderr);
char arr[]="HTTP/1.0--- 200 OK\r\n\r\n404 file not found";
if (send(clientSockfd,arr, 44, 0) == -1) {
      perror("send");
     // break;
exit(1);
    }
}
url=temp;
  // obtain file size:
  fseek (inputf , 0 , SEEK_END);
  lSize = ftell (inputf);
  rewind (inputf);

  // allocate memory to contain the whole file:
  buffer1 = (char*) malloc (sizeof(char)*lSize);
  if (buffer1 == NULL) {fputs ("Memory error",stderr); exit (2);}

  // copy the file into the buffer:
  res = fread (buffer1,1,lSize,inputf);
  if (res != lSize) {fputs ("Reading error",stderr); exit (3);}

  /* the whole file is now loaded in the memory buffer. */

  // terminate
  fclose (inputf);
  ss << buffer1 << std::endl;
    std::cout << buffer1 << std::endl;

    char buffer3[]="HTTP/1.0 200 OK\r\n\r\n";
    std::string str=strcat(buffer3,buffer1);
    if (send(clientSockfd, str.c_str(), str.size(), 0) == -1) {
      perror("send");
     // break;
    }
var--;

    if (ss.str() == "close\n")
      //break;

    ss.str("");
  //}

	

  close(clientSockfd);
pthread_exit(NULL);

}
std::string http_parsers(char* buffer2){
    char ret_file[256];
    memset(ret_file, '\0', sizeof(ret_file));
    int n=0;
    
    buffer2 = buffer2 + 4;
    
    while(*buffer2 !=  ' '){
      ret_file[n] = *buffer2;
      n++;
      buffer2++;
   }


    return ret_file;
}
