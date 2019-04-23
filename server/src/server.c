// server.c

#include <string.h>         // strcmp
#include <errno.h>      		// perror
#include <sys/types.h>      // socket
#include <sys/socket.h>     // socket
#include <netinet/in.h>     // inet_ntoa
#include <arpa/inet.h>      // inet_ntoa
#include <unistd.h>         // close

#include <server.h>

/* receivAll e sendAll são funções auxiliares utulizadas
   na função forwardMsg, garante que os dados sejam
   todos enviados/recebidos */
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
		if(bytesReceived == 0)	// cliente desconectou
		{
			free(packet);
			return NULL;
		}

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
		unsigned int bytesSent = 0;

		// enviar dados
		while (len > 0)
		{
			puts("Enviando...");
			if((bytesSent = send(sockOut, pBuff, len, 0)) == -1)
				return false;
			pBuff += bytesSent;
			len -= bytesSent;
		}

		return true;
}

/*
 * faz o encaminhamento das mensagens entre os cliente conectados
 */
int forwardMsg(int sock, PACKET *admins, PACKET *clientes)
{
	int i;
  bool localizado = false;
	PACKET *packet = (PACKET *) malloc(sizeof(PACKET));

	packet = receivAll(sock);
	if(packet == NULL){
		free(packet);
		return DISCONECTED;
	}


	if(!strcmp(packet->type, "CL"))
	{
		for(i = 0; i < MAXCLIENT; i++)
		{
			if(admins[i].id == packet->id)
      {
        localizado = true;
			  if(!sendAll(admins[i].sock, packet->data, packet->len + 1))
			  {
				  admins[i].id = 0;
				  return FAILSEND;
        }
      }
		} // não foi encontrado par com mesmo ID
    if (!localizado) return WAITPAIR;
	}
	else if(!strcmp(packet->type, "AD"))
	{
		for(i = 0; i < MAXCLIENT; i++)
		{
			if(clientes[i].id == packet->id)
      {
        localizado = true;
        if(!sendAll(clientes[i].sock, packet->data, packet->len + 1))
  			{
  				clientes[i].id = 0;
  				return FAILSEND;
  			}
      }
		} // não foi encontrado par com mesmo ID
    if (!localizado) return WAITPAIR;
	}
	else
	{
		close(sock);
		free(packet);
	}

	return 0;
}


int validar(fd_set *activeFdSet, PACKET *admins, PACKET *clientes)
 {
	 int i;
	 int status = 0;		// utilizada para verificar se o ID da conexão ja existe
	 int sockCon = 0;		// file descriptor socket cliente
	 struct sockaddr_in dadosCl; /* recebe os dados do cliente, por accept() */
	 socklen_t tDadosCl = sizeof(dadosCl);
	 PACKET *packet = malloc(sizeof(PACKET));

	 if ((sockCon = accept(fSockSv,(SA *) &dadosCl, &tDadosCl)) < 0)
	 		erro("accpet");

	 packet = receivAll(sockCon);
   if (packet == NULL)
    return false;

   // identifica conexão
	 if(!strcmp(packet->type, "CL"))	// caso o pacote recebido seja do tipo cliente
	 {
		 puts("Conexão cliente recebida");
     packet->adress  = inet_ntoa(dadosCl.sin_addr);
     packet->port   = ntohs(dadosCl.sin_port);

		 for(i = 0; i < MAXCLIENT; i++)		// verifica se o id ja esta na lista de clientes
		 {
			 if(clientes[i].id == packet->id)
			 {
				 status = 1;
				 close(sockCon);
				 puts("Cliente ja esta no sistema");
				 break;
			 }
		 } // for localizar cliente

		 if(!status)
		 {
			 for(i = 0; i < MAXCLIENT; i++)
			 {
				 if(clientes[i].id == 0)
				 {
					 clientes[i]        = *packet;
					 free(packet);
					 clientes[i].sock   = sockCon;
           clientes[i].adress = packet->adress;
           clientes[i].port   = packet->port;
					 FD_SET(clientes[i].sock, activeFdSet);
           printf("Cliente conectado %s:%d ID - %d\n",
                 clientes[i].adress, clientes[i].port, clientes[i].id);
					 break;
				 }
			 }
		 }
	 }

	 else if(!strcmp(packet->type, "AD"))
	 {
		 puts("Conexão admin recebida");
     packet->adress  = inet_ntoa(dadosCl.sin_addr);
     packet->port   = ntohs(dadosCl.sin_port);

		 for(i = 0; i < MAXCLIENT; i++)
		 {
			 if(admins[i].id == packet->id)
			 {
				 status = 1;
				 close(sockCon);
				 free(packet);
				 printf("ID %d admin duplicado, deconectado ultimna solicitação\n", admins[i].id);
				 break;
			 }
		 }

		 if(!status)
		 {
			 for(i = 0; i < MAXCLIENT; i++)
			 {
				 if(admins[i].id == 0)
				 {
					 admins[i]        = *packet;
					 free(packet);
					 admins[i].sock   = sockCon;
           admins[i].adress = packet->adress;
           admins[i].port   = packet->port;
					 FD_SET(admins[i].sock, activeFdSet);
           printf("Admin conectado %s:%d ID - %d\n",
                 admins[i].adress, admins[i].port, admins[i].id);
					 break;
				 }
			 }
		 }
	 }
	 else
	 { // desconecta caso a conexão não possa ser identificada
 	 		close(sockCon);
    	free(packet);
			return false;
	 }
	 return sockCon;
 } // validar()


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
	if ((bind(fSockSv, (SA *)&dadosSv, tDadosSv)) < 0)
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
