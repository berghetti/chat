#include <stdio.h>
#include <stdlib.h>
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

#define TRUE 1
#define FALSE 0
#define PORT    5000				// porta de escuta
#define MAXBUFF 10240

// prototipos
int forwardMsg(int, int);
int startSocket(void);
int validarCon();
void erro(char *);

// var global
int fSockSv;
char buff[MAXBUFF];

typedef struct {
	char type[2];		// identifica o cliente se é 'CL' ou 'AD'
	int id;					// id que identifica o cliente especifico
	int len;	// tamanho do conteudo a ser recebido
	int sock;				// socket associado ao cliente
} PACKET;


int main(void)
{
	setlocale(LC_ALL, "");

	int maxFd = 0;
	fd_set activeFdSet;   /* estrutura que recebe descritores ativos */
	fd_set readFdSet;     /* estrutura que é atualizada a cada iteração */
	PACKET packetAdmin = {0};
	PACKET packetClient = {0};

	if((fSockSv = startSocket()) < 0)
	{
		erro("socket");
	}

	FD_ZERO(&activeFdSet); /* zera fd_set */

	puts("Escutando...");

	while(TRUE)
	{
		// char *adress = inet_ntoa(dadosCl.sin_addr);
		// int port = ntohs(dadosCl.sin_port);

		maxFd = validarCon(&activeFdSet, &packetAdmin, &packetClient);

		while(packetClient.sock != 0 && packetAdmin.sock != 0)
		{
				readFdSet = activeFdSet;

				if(select(maxFd + 1, &readFdSet, NULL, NULL, NULL) < 0)
						erro("select");

				if(FD_ISSET(packetClient.sock, &readFdSet))
				{
						puts("recendo dados cliente");
						if(!forwardMsg(packetClient.sock, packetAdmin.sock))
						{
							FD_CLR(packetClient.sock, &activeFdSet);
							packetClient.sock = 0;
							erro("forward");
						}
				}
				else if(FD_ISSET(packetAdmin.sock, &readFdSet))
				{
						puts("recendo dados admin");
						if(!forwardMsg(packetAdmin.sock, packetClient.sock))
						{
							FD_CLR(packetAdmin.sock, &activeFdSet);
							packetAdmin.sock = 0;
							erro("forward");
						}
				}

		} // while sockCl != 0 && sockAd != 0

	} // while externo

	return 0;
}

/*
 * faz o encaminhamento das mensagens entre os cliente conectados
 */
int forwardMsg(int sockIn, int sockOut)
{
	char buff[MAXBUFF] = {0};
	char *pBuff = buff;
	ssize_t bytesReceived = 0;
	ssize_t bytesSent = 0;

	// receber dados
	bytesReceived = recv(sockIn, buff, MAXBUFF, 0);
	if(bytesReceived <= 0)
		return FALSE;

	// enviar dados
	while (bytesReceived > 0)
	{
		if((bytesSent = send(sockOut, pBuff, bytesReceived, 0)) == -1)
			return FALSE;
		pBuff += bytesSent;
		bytesReceived -= bytesSent;
	}

	return TRUE;
}


 int validarCon(fd_set *activeFdSet, PACKET *packetAdmin, PACKET *packetClient)
 {
	 int maxFd = 0; 						// maior file descriptor
	 int sockCon = 0;				// file descriptor socket cliente
	 struct sockaddr_in dadosCl; /* recebe os dados do cliente, por accept() */
	 socklen_t tDadosCl = sizeof(dadosCl);
	 PACKET *packetGeneric;

	 if ((sockCon = accept(fSockSv,(struct sockaddr *) &dadosCl, &tDadosCl)) < 0)
	 {
			 erro("accpet");
	 }

	 packetGeneric = malloc(sizeof(PACKET));

	 recv(sockCon, buff, MAXBUFF, 0);
	 // identifica conexão
	 for(int i = 0; i < 2; i++)
	 {
		 packetGeneric->type[i] = buff[i];
	 }

	 if(!strcmp(packetGeneric->type, "CL"))
	 {
			 puts("conexao cliente recebida");
			 packetClient->sock = sockCon;

			 memset(buff, 0x0, MAXBUFF);
			 FD_SET(packetClient->sock, activeFdSet);
			 maxFd = (sockCon > maxFd) ? sockCon : maxFd;
	 }
	 else if(!strcmp(packetGeneric->type, "AD"))
	 {
			 puts("conexão admin recebida");
			 packetAdmin->sock = sockCon;

			 memset(buff, 0x0, MAXBUFF);
			 FD_SET(packetAdmin->sock, activeFdSet);
			 maxFd = (sockCon > maxFd) ? sockCon : maxFd;
	 }
	 else{
	 	close(sockCon); // desconecta caso a conexão no possa ser idnetificada

	}
		return maxFd;
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
