/* cliente.c
   cabeçalho
    2 bytes - tipos
    4 bytes - id
    4 bytes - len */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>  // socket
#include <sys/socket.h> // socket, inet_*
#include <sys/select.h> // select
#include <locale.h>     // setlocale
#include <string.h>     // memcpy
#include <arpa/inet.h>	// inet_*, htonl
#include <netinet/in.h> // inet_*
#include <unistd.h>	    // close
#include <stdint.h>     // definições de tipos

#define  MAXBUFF 10240
#define LEN_HEADER 10   // tamanho do cabeçaho

struct header {
  uint32_t id;
  uint32_t len;
  uint16_t type;
};

struct packet{
  struct header head;
  uint8_t *data;
};

const char *CLIENTE = "CL";     // identificador do pacote
uint8_t buff2[MAXBUFF] = {0};

int startSocket(void);
void erro(char *);
uint8_t *serialize(struct packet *);

int fSock;

int main(void){
    setlocale(LC_ALL, "");

    struct packet *pacote;
    fd_set activeFdSet, readFdSet;
    int maxFd;
    ssize_t bytesread;
    uint8_t *dataSend;

    pacote = (struct packet *) malloc(sizeof(struct packet));

    memcpy(&pacote->head.type, CLIENTE, sizeof(pacote->head.type));
    pacote->head.type = htons(pacote->head.type);
    pacote->head.id = htonl(1);


    fSock = startSocket();

    maxFd = 0;
    maxFd = (fSock > maxFd) ? fSock : maxFd;

    FD_ZERO(&activeFdSet);
    FD_SET(fileno(stdin), &activeFdSet);
    FD_SET(maxFd, &activeFdSet);

    //send(fSock, header, 4, 0);
    while(1)
    {
      readFdSet = activeFdSet;

      if (select(maxFd + 1, &readFdSet, NULL, NULL, NULL) < 0)
        erro("select");

      if (FD_ISSET(fSock, &readFdSet)){
          bytesread = recv(fSock, buff2, MAXBUFF, 0);
          buff2[bytesread] = '\0';
          printf("%s", buff2);
          fflush(stdout);
      }
      else if(FD_ISSET(fileno(stdin), &readFdSet)){
        bytesread = read(fileno(stdin), buff2, MAXBUFF);
        if (bytesread == -1)
          erro("read");
        if (bytesread > 0)
          bytesread++; // + 1 para incluir o caracter nulo

        buff2[bytesread] = '\0';
        pacote->head.len = (uint32_t) bytesread;
        pacote->data = (uint8_t *) buff2;

        dataSend = serialize(pacote);
        printf("%ld\n", sizeof(pacote->head.type));
        send(fSock, dataSend, LEN_HEADER + bytesread, 0);
      }

    }

    return 0;
}
uint8_t * serialize(struct packet *pacote)
{
  uint8_t *buff;
  uint32_t len;
  uint8_t padding;

  // tamanho do bufer com base no tamanho da mensagem e no tamanho do cabeçalho
  buff = (uint8_t *) malloc(LEN_HEADER + pacote->head.len);


  // salva tamanho da mensagem antes de ordenar os bytes para big endian
  len = pacote->head.len;

  // ordena os bytes para o modo de rede
  pacote->head.len  = htonl(pacote->head.len);

  // serializa os dados
  memcpy(buff, &pacote->head.type, sizeof(pacote->head.type));

  padding = sizeof(pacote->head.type);
  memcpy(buff + padding, &pacote->head.id, sizeof(pacote->head.id));

  padding += sizeof(pacote->head.id);
  memcpy(buff + padding, &pacote->head.len, sizeof(pacote->head.len));

  padding += sizeof(pacote->head.len);
  memcpy(buff + padding, pacote->data, len);

  return buff;
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

  if ((connect(fSock, (struct sockaddr *)&dadosSv, sizeof(dadosSv))) < 0)
      erro("connect");

  return fSock;
}

void erro(char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}
