#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>	    // for inet_ntoa
#include <unistd.h>	    // for close()

#define  MAXBUFF 10240

char buff[MAXBUFF];

int startSocket(void);
void erro(char *);

int fSock;

int main(void){

    fSock = startSocket();
    fd_set activeFdSet, readFdSet;
    int maxFd = 0;
    int bytesread = 0;


    maxFd = (fSock > maxFd) ? fSock : maxFd;

    FD_ZERO(&activeFdSet);
    FD_SET(fileno(stdin), &activeFdSet);
    FD_SET(maxFd, &activeFdSet);


    send(fSock, "AD", 2, 0);
    while(1)
    {
        readFdSet = activeFdSet;

        if (select(maxFd + 1, &readFdSet, NULL, NULL, NULL) < 0)
          erro("select");

        if(FD_ISSET(fSock, &readFdSet)){
            bytesread = recv(fSock, buff, MAXBUFF, 0);
            buff[bytesread] = '\0';
            printf("%s", buff);
            fflush(stdout);
        }
        else if(FD_ISSET(fileno(stdin), &readFdSet)){
          bytesread = read(fileno(stdin), buff, 100);
          send(fSock, buff, bytesread, 0);
        }

    }

    return 0;
}

int startSocket(void){
  // file descriptor que identifica o socket local
  fSock = socket(AF_INET, SOCK_STREAM, 0);

  // escrutura que ira receber os parametros do servidor
  struct sockaddr_in dadosSv;
  dadosSv.sin_family = AF_INET;
  dadosSv.sin_port = htons(5000);
  dadosSv.sin_addr.s_addr = inet_addr("192.168.2.105"); // INADDR_ANY = localhost
  memset(&dadosSv.sin_zero, 0, sizeof(dadosSv.sin_zero));

  socklen_t tDadosSv= sizeof(dadosSv);

  if ((connect(fSock, (struct sockaddr *)&dadosSv, tDadosSv)) < 0)
      erro("erro ao conectar");

  return fSock;
}

void erro(char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}
