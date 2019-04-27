// server.c

#include <stdbool.h>
#include <string.h>         // strcmp
#include <errno.h>      		// perror
#include <sys/types.h>      // socket
#include <sys/socket.h>     // socket
#include <netinet/in.h>     // inet_ntoa
#include <arpa/inet.h>      // inet_ntoa
#include <unistd.h>         // close

#include <server.h>

// char *ADMIN = "AD";
// char *CLIENTE = "CL";

/*cabeçalho
 2 bytes - tipos
 4 bytes - id
 4 bytes - len */
PACKET * deserialize(uint8_t *buff)
{
	PACKET *packet = (PACKET *) malloc(sizeof(PACKET));
	if (packet == NULL)
		return NULL;

	uint8_t padding;

	memcpy(&packet->head.type, buff, sizeof(packet->head.type));

	padding = sizeof(packet->head.type);
	memcpy(&packet->head.id, buff + padding, sizeof(packet->head.id));

	padding += sizeof(packet->head.id);
	memcpy(&packet->head.len, buff + padding, sizeof(packet->head.len));

	printf("type - %x\n", (unsigned int) packet->head.type);
	printf("id   - %d\n", (unsigned int) packet->head.id);
	printf("len  - %d\n", (unsigned int) packet->head.len);

	// converte ordem dos bytes para formato do host
	packet->head.type = ntohs(packet->head.type);
	packet->head.id 	= ntohl(packet->head.id);
	packet->head.len 	= ntohl(packet->head.len);

	printf("type - %x\n", (unsigned int) packet->head.type);
	printf("id   - %d\n", (unsigned int) packet->head.id);
	printf("len  - %d\n", (unsigned int) packet->head.len);

	padding += sizeof(packet->head.len);
	packet->data = (char *) malloc(packet->head.len); //verificar cast
	memcpy(packet->data, buff + padding, packet->head.len);

	return packet;
}


/* receivAll e sendAll são funções auxiliares utulizadas
   na função forwardMsg, garante que os dados sejam
   todos enviados/recebidos */
void receivAll(int sockIn, uint8_t *buff)
{
	ssize_t bytesReceived;
	uint32_t len;
	uint8_t *p_buff = buff;	// ponteiro utilzado para percorrer 'buff'

	do
	{
		bytesReceived = recv(sockIn, p_buff, MAXBUFF, 0);
		if(bytesReceived == 0)	// cliente desconectou
			buff = NULL;

		len					 	= *(buff + 3);
		bytesReceived += bytesReceived;
		p_buff 				+= bytesReceived;
	}while(bytesReceived < len);
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
	PACKET *packet;
	uint8_t *recv_buff;
	int i;
  bool localizado = false;

	recv_buff = (uint8_t *) malloc(MAXBUFF);
	receivAll(sock, recv_buff);
	if(recv_buff == NULL){
		free(recv_buff);
		return DISCONECTED;
	}

	packet = deserialize(recv_buff);
	if (packet == NULL)
		erro("deserialize");


	if(!memcmp(&packet->head.type, "CL", sizeof(packet->head.type)))
	{
		puts("AUI");
		for(i = 0; i < MAXCLIENT; i++)
		{
			if(admins[i].head.id == packet->head.id)
      {
        localizado = true;
			  if(!sendAll(admins[i].sock, packet->data, packet->head.len + 1))
			  {
				  admins[i].head.id = 0;
				  return FAILSEND;
        }
      }
		} // não foi encontrado par com mesmo ID
    if (!localizado) return WAITPAIR;
	}
	else if(!memcmp(&packet->head.type, "AD", sizeof(packet->head.type)))
	{
		for(i = 0; i < MAXCLIENT; i++)
		{
			if(clientes[i].head.id == packet->head.id)
      {
        localizado = true;
        if(!sendAll(clientes[i].sock, packet->data, packet->head.len + 1))
  			{
  				clientes[i].head.id = 0;
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
	 PACKET *packet;
	 struct sockaddr_in dadosCl; /* recebe os dados do cliente, por accept() */
	 uint8_t *recv_buff;

	 int i;
	 int localizado = 0;		// utilizada para verificar se o ID da conexão ja existe
	 int sockCon = 0;		// file descriptor socket cliente
	 socklen_t tDadosCl = sizeof(dadosCl);


	 if ((sockCon = accept(fSockSv,(SA *) &dadosCl, &tDadosCl)) < 0)
	 		erro("accpet");

	 recv_buff = (uint8_t *) malloc(MAXBUFF);
	 if (recv_buff == NULL)
		erro("memoria");

	 receivAll(sockCon, recv_buff);
   if (recv_buff == NULL)
    return false;

	packet = deserialize(recv_buff);
	if(packet == NULL)
		erro("memoria");

   // identifica conexão
	 // caso o packet recebido seja do tipo cliente
	 if(!memcmp(&packet->head.type, "CL", sizeof(packet->head.type)))
	 {
		 puts("Conexão cliente recebida");
     packet->adress = inet_ntoa(dadosCl.sin_addr);
     packet->port   = ntohs(dadosCl.sin_port);

		 for(i = 0; i < MAXCLIENT; i++)		// verifica se o id ja esta na lista de clientes
		 {
			 if(clientes[i].head.id == packet->head.id)
			 {
				 localizado = true;
				 close(sockCon);
				 printf("ID %d cliente duplicado, deconectado ultimna solicitação\n", clientes[i].head.id);
				 break;
			 }
		 } // for localizar cliente

		 if(!localizado)
		 {
			 for(i = 0; i < MAXCLIENT; i++)
			 {
				 if(clientes[i].head.id == 0)
				 {
					 clientes[i]        = *packet;
					 free(packet);
					 clientes[i].sock   = sockCon;
           clientes[i].adress = packet->adress;
           clientes[i].port   = packet->port;
					 FD_SET(clientes[i].sock, activeFdSet);
           printf("Cliente conectado %s:%d ID - %d\n",
                 clientes[i].adress, clientes[i].port, clientes[i].head.id);
					 break;
				 }
			 }
		 }
	 }
	 // se o pacote é do tipo admin
	 if(!memcmp(&packet->head.type, "AD", sizeof(packet->head.type)))
	 {
		 puts("Conexão admin recebida");
     packet->adress  = inet_ntoa(dadosCl.sin_addr);
     packet->port   = ntohs(dadosCl.sin_port);

		 for(i = 0; i < MAXCLIENT; i++)
		 {
			 if(admins[i].head.id == packet->head.id)
			 {
				 localizado = true;
				 close(sockCon);
				 free(packet);
				 printf("ID %d admin duplicado, deconectado ultimna solicitação\n", admins[i].head.id);
				 break;
			 }
		 }

		 if(!localizado)
		 {
			 for(i = 0; i < MAXCLIENT; i++)
			 {
				 if(admins[i].head.id == 0)
				 {
					 admins[i]        = *packet;
					 free(packet);
					 admins[i].sock   = sockCon;
           admins[i].adress = packet->adress;
           admins[i].port   = packet->port;
					 FD_SET(admins[i].sock, activeFdSet);
           printf("Admin conectado %s:%d ID - %d\n",
                 admins[i].adress, admins[i].port, admins[i].head.id);
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
