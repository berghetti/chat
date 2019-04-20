#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>      		// for perror()
#include <sys/types.h>      // for socket()
#include <sys/socket.h>     // for socket()
#include <netinet/in.h>     // for socket()
#include <string.h>         // for strlen()
#include <arpa/inet.h>      // for inet_ntoa()
#include <unistd.h>         // for close()
#include <sys/time.h>       // for select()
#include <sys/types.h>      // for select()
#include <locale.h>      		// for setlocale()


#define PORT    5000				// porta de escuta
#define MAXBUFF 10240
#define MAXCLIENT 10

typedef struct {
	char *data;			// dados da mensagem excluido o cabeçaho
	int id;					// id que identifica o cliente especifico
	unsigned int len;				// tamanho do conteudo a ser recebido
	int sock;				// socket associado ao cliente
	char type[2];		// identifica o cliente se é 'CL' ou 'AD'

} PACKET;

// prototipos
bool sendAll(int sockOut, char *buff, int len);
PACKET * receivAll(int sockIn);
bool forwardMsg(int sock, PACKET *admins, PACKET *clientes);
int startSocket(void);
bool validar(fd_set *activeFdSet, int *, PACKET *clientes, PACKET *admins);
void erro(char *);

// var global
int fSockSv;
char buff[MAXBUFF];


int main(void)
{
	setlocale(LC_ALL, "");

	// cria vetor com tamanho MAXCLIENT
	// acessa cada espaço com clientes[i].campo
	PACKET *clientes = (PACKET *) malloc(sizeof(PACKET) * MAXCLIENT);
	PACKET *admins = (PACKET *) malloc(sizeof(PACKET) * MAXCLIENT);


	int i;
	int maxFd;
	fd_set activeFdSet;   /* estrutura que recebe descritores ativos */
	fd_set readFdSet;     /* estrutura que é atualizada a cada iteração */

	if((fSockSv = startSocket()) < 0)
		erro("socket");

	FD_ZERO(&activeFdSet); /* zera fd_set */
	FD_SET(fSockSv, &activeFdSet);

	maxFd = fSockSv;

	puts("Escutando...");


	while(true)
	{
		// char *adress = inet_ntoa(dadosCl.sin_addr);
		// int port = ntohs(dadosCl.sin_port);

				readFdSet = activeFdSet;

				if(select(maxFd + 1, &readFdSet, NULL, NULL, NULL) < 0)
						erro("select");

				for(i = 0; i <= maxFd; i++)
				{
					if(FD_ISSET(i, &readFdSet))
					{
						if(i == fSockSv)
						{
							validar(&activeFdSet, &maxFd, admins, clientes);
						}
						else
						{
							puts("recebendo dados...");
							if(!forwardMsg(i, admins, clientes))
							{
								FD_CLR(i, &activeFdSet);
								for(int j = 0; j < MAXCLIENT; j++)
								{
									if(clientes[j].sock == i){
										clientes[j].id = 0;
										break;
									}
									else if (admins[j].sock == i)
									{
										admins[j].id = 0;
										break;
									}
								}	// for
							}
						}
					}
				}

	}

	return 0;
}

/*
 * faz o encaminhamento das mensagens entre os cliente conectados
 */
bool forwardMsg(int sock, PACKET *admins, PACKET *clientes)
{
	int i;
	PACKET *packet = (PACKET *) malloc(sizeof(PACKET));

	packet = receivAll(sock);
	if(packet == NULL)
		return false;

	if(!strcmp(packet->type, "CL"))
	{
		for(i = 0; i < MAXCLIENT; i++)
		{
			if(admins[i].id == packet->id)

			if(!sendAll(admins[i].sock, packet->data, packet->len + 1))
			{
				admins[i].id = 0;
				return false;
			}
		}
	}
	else if(!strcmp(packet->type, "AD"))
	{
		for(i = 0; i < MAXCLIENT; i++)
		{
			if(clientes[i].id == packet->id)
			if(!sendAll(clientes[i].sock, packet->data, packet->len + 1))
			{
				clientes[i].id = 0;
				return false;
			}

		}
	}

	return true;
}

PACKET * receivAll(int sockIn)
{
	PACKET *packet = (PACKET *) malloc(sizeof(PACKET));
	unsigned int bytesReceived = 0;
	char buff[MAXBUFF] = {0};
	char *pBuff = buff;

	if(packet == NULL)
		erro("falha ao alocar memoria");

	do
	{
		bytesReceived = recv(sockIn, pBuff, MAXBUFF, 0);
		if(bytesReceived == 0)
			return NULL;

		packet->len 	= buff[3];
		bytesReceived += bytesReceived;
		pBuff 				+= bytesReceived;
	}while(bytesReceived < packet->len);

	packet->type[0] = buff[0];
	packet->type[1] = buff[1];
	packet->id 			= buff[2];
	packet->data 		= buff + 4;
	packet->data[packet->len] = '\0';

	return packet;
}


bool sendAll(int sockOut, char *buff, int len)
{
		char *pBuff = buff;
		ssize_t bytesSent = 0;

		// enviar dados
		while (len > 0)
		{
			puts("enviando...");
			if((bytesSent = send(sockOut, pBuff, len, 0)) == -1)
				return false;
			pBuff += bytesSent;
			len -= bytesSent;
		}

		return true;
}


 bool validar(fd_set *activeFdSet, int *maxFd, PACKET *admins, PACKET *clientes)
 {
	 int i;
	 int status = 0;
	 int sockCon = 0;				// file descriptor socket cliente
	 struct sockaddr_in dadosCl; /* recebe os dados do cliente, por accept() */
	 socklen_t tDadosCl = sizeof(dadosCl);
	 PACKET *packet = malloc(sizeof(PACKET));

	 if ((sockCon = accept(fSockSv,(struct sockaddr *) &dadosCl, &tDadosCl)) < 0)
	 		erro("accpet");

	 recv(sockCon, buff, MAXBUFF, 0);
	 // identifica conexão
	 packet->type[0] = buff[0];
	 packet->type[1] = buff[1];
	 packet->id 		 = buff[2];
	 packet->len 		 = buff[3];

	 if(!strcmp(packet->type, "CL"))	// caso o pacote recebido seja do tipo cliente
	 {
		 puts("conexão cliente recebida");
		 for(i = 0; i < MAXCLIENT; i++)		// verifica se o id ja esta na lista de clientes
		 {
			 if(clientes[i].id == packet->id)
			 {
				 puts("cliente ja esta no sistema");
				 status = 1;
				 break;
			 }
		 } // for localizar cliente

		 if(!status)
		 {
			 for(i = 0; i < MAXCLIENT; i++)
			 {
				 if(clientes[i].id == 0)
				 {
					 clientes[i] = *packet;
					 clientes[i].sock = sockCon;
					 FD_SET(clientes[i].sock, activeFdSet);
					 puts("cliente adicionado no vetor clientes");
					 break;
				 }
			 }
		 }
			*maxFd = (sockCon > *maxFd) ? sockCon : *maxFd;
	 }

	 else if(!strcmp(packet->type, "AD"))
	 {
		 puts("conexão admin recebida");
		 for(i = 0; i < MAXCLIENT; i++)
		 {
			 if(admins[i].id == packet->id)
			 {
				 status = 1;
				 puts("admin ja esta no sistema");
				 break;
			 }
		 }

		 if(!status)
		 {
			 for(i = 0; i < MAXCLIENT; i++)
			 {
				 if(admins[i].id == 0)
				 {
					 admins[i] = *packet;
					 admins[i].sock = sockCon;
					 FD_SET(admins[i].sock, activeFdSet);
					 puts("admin adicionado no sistema");
					 break;
				 }
			 }
		 }

			 *maxFd = (sockCon > *maxFd) ? sockCon : *maxFd;
	 }
	 else
	 {
 	 	close(sockCon); // desconecta caso a conexão no possa ser idnetificada
		return false;
	 }


		return true;
 }


int startSocket(void)
{
	socklen_t tDadosSv;					/* armazena tamanha da estrutura */
	struct sockaddr_in dadosSv; /* recebe ip/porta de escuta */

	if ((fSockSv = socket(AF_INET, SOCK_STREAM, 0)) < 1)
	{
		erro("socket");
	}

	dadosSv.sin_family = AF_INET;
	dadosSv.sin_port = htons(PORT);
	dadosSv.sin_addr.s_addr = INADDR_ANY; // INADDR_ANY = localhost
	memset(&dadosSv.sin_zero, 0, sizeof(dadosSv.sin_zero));

	/* permite que o socket seja vinculado a um endereço ja em uso,
	bom para casos que é necessario reiniciar o software */
	int reuse = 1;
	if ((setsockopt(fSockSv,
		 							SOL_SOCKET, SO_REUSEADDR,
		 							&reuse,
									sizeof(reuse))) < 0)
	{
		erro("setsockopt");
	}

	tDadosSv = sizeof(dadosSv);

	/* da socket ao 'fSockSv' o endereço e porta de
	 comunição definida na struct 'dadosSv' */
	if ((bind(fSockSv, (struct sockaddr *)&dadosSv, tDadosSv)) < 0)
	{
		erro("bind");
	}

	/* habilita o socket a receber conexões */
	if ((listen(fSockSv, 128) < 0))
	{
		erro("listen");
	}

	return fSockSv;
}

void erro(char *msg)
{
	perror(msg);
	//exit(EXIT_FAILURE);
}
