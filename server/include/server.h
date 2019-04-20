// server.h

#ifndef _SERVER_H_
#define _SERVER_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>   // for select()
#include <sys/types.h>  // for select()


#define PORT    5000				// porta de escuta
#define MAXBUFF 10240
#define MAXCLIENT 10        // maximo de cliente que o servidor vai tratar

// códigos de erro
enum COD_ERRO{
  FAILSEND = 1, // falha ao enviar mensagem
  WAITPAIR      // aguardar par da conexão para encaminhar mensagem
};
// var global
int fSockSv;

typedef struct sockaddr SA;

// estrutura da mensagem que o servidor deve receber
typedef struct {
	char *data;			   // dados da mensagem excluido o cabeçaho
  char *adress;      // endereço do cliente
  int port;          // porta de conexão do cliente
	int id;					   // id que identifica o cliente especifico
	unsigned int len;	 // tamanho do conteudo a ser recebido
	int sock;				   // socket associado ao cliente
	char type[2];		   // identifica o cliente se é 'CL' ou 'AD'
} PACKET;


/* faz o ecaminhamento  de mensagens entre cliente e admin
   baseado no id que identifica a conexao, caso
   o encaminhamento da mensagem falhe, a conexão
   que falhou é retirada da lista ativa */
int forwardMsg(int sock, PACKET *admins, PACKET *clientes);

// inicia o socket do servidor
int startSocket(void);

/*valida uma nova conexão
  caso a conexão seja valida, adiciona o identificador
  no vetor apropriado, clientes ou admnis */
bool validar(fd_set *activeFdSet, int *, PACKET *clientes, PACKET *admins);

// função de erro padrão
void erro(char *);

#endif //SERVER_H
