// server.h

#ifndef _SERVER_H_
#define _SERVER_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


#define PORT    5000				// porta de escuta
#define MAXBUFF 10240
#define MAXCLIENT 10        // maximo de cliente que o servidor vai tratar

// extern char *ADMIN;
// extern char *CLIENTE;

// códigos de erro
enum COD_ERRO{
  FAILSEND = 1, // falha ao enviar mensagem
  WAITPAIR,     // aguardar par da conexão para encaminhar mensagem
  DISCONECTED,   // cliente desconectado
  INVALID       // pacote invalido
};

// var global
extern int fSockSv;

typedef struct sockaddr SA;

struct header{
  uint32_t id;	 // id que identifica o cliente especifico
  uint32_t len;	 // tamanho do conteudo a ser recebido
  uint16_t type; // identifica o cliente se é 'CL' ou 'AD'
};

// estrutura da mensagem que o servidor deve receber
typedef struct {
  struct header head; // cabeçaho do pacote
	uint8_t *data;			// dados da mensagem excluido o cabeçaho
  char *adress;       // endereço do cliente
  uint16_t port;      // porta de conexão do cliente
	int sock;				    // socket associado ao cliente
} PACKET;



PACKET * deserialize(uint8_t *buff);

/* faz o ecaminhamento  de mensagens entre cliente e admin
   baseado no id que identifica a conexao, caso
   o encaminhamento da mensagem falhe, a conexão
   que falhou é retirada da lista ativa */
int forwardMsg(int sock, PACKET *admins, PACKET *clientes);

// inicia o socket do servidor
int startSocket(void);

/*valida uma nova conexão
  caso a conexão seja valida, adiciona o identificador
  no vetor apropriado - clientes ou admnis - e retorna o
  file descriptor da conexão */
int validar(PACKET *clientes, PACKET *admins);

// função de erro padrão
void erro(char *);

#endif //SERVER_H
