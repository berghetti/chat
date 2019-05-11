// server.h

#ifndef _SERVER_H_
#define _SERVER_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <packet.h>


#define PORT    5000  // porta de escuta
#define MAXBUFF 10240
#define MAXCLIENT 10  // maximo de cliente que o servidor vai tratar

// var global
extern int fSockSv;

extern PACKET *clientes;  // array dinamico que aloca os clientes conectados
extern PACKET *admins;    // array dinamico que aloca os admins conectados

extern uint32_t totalClientes;
extern uint32_t totalAdmins;
// extern char *ADMIN;
// extern char *CLIENTE;

// códigos de erro
enum COD_ERRO{
  FAILSEND = 1,       // falha ao enviar mensagem
  PEER_NOT_FOUND,     // aguardar par da conexão para encaminhar mensagem
  DISCONECTED,        // cliente desconectado
  INVALID,            // pacote invalido
  ERRO_MEMORY
};


typedef struct sockaddr SA;
typedef struct sockaddr_in SA_I;

int clearData(int sock);

bool deserialize(PACKET *, uint8_t *);

/* faz o ecaminhamento  de mensagens entre cliente e admin
   baseado no id que identifica a conexao, caso
   o encaminhamento da mensagem falhe, a conexão
   que falhou é retirada da lista ativa */
int forwardMsg(int sock);

// inicia o socket do servidor
int startSocket(void);

/*valida uma nova conexão
  caso a conexão seja valida, adiciona o identificador
  no vetor apropriado - clientes ou admnis - e retorna o
  file descriptor da conexão */
int validar();

// função de erro padrão
void erro(char *);

#endif //SERVER_H
