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

// funções auxiliares

/* receivAll e sendAll são funções auxiliares utulizadas
   na função forwardMsg, garante que os dados sejam
   todos enviados/recebidos */
bool receivAll(int sockIn, uint8_t *buff, size_t buffsize)
{
	ssize_t bytes_recv;
	uint32_t len;
	uint8_t *p_buff = buff;	// ponteiro utilzado para percorrer 'buff'

	do
	{
		bytes_recv = recv(sockIn, p_buff, buffsize, 0);
		if(bytes_recv <= 0) // cliente desconectou ou recv retornou erro
			return false;

		len				 = *(buff + 3);
		bytes_recv += bytes_recv;
		p_buff 		 += bytes_recv;
	}while(bytes_recv < len);
	return true;
}

bool sendAll(int sockOut, uint8_t *buff, size_t len)
{
	uint8_t *p_buff = buff;
	ssize_t bytes_send;

	while (len > 0)
	{
		bytes_send = send(sockOut, p_buff, len, 0);
		if(bytes_send < 0)	// erro ao enviar
			return false;
		p_buff += bytes_send;
		len    -= bytes_send;
	}
	return true;
}

/*cabeçalho
 2 bytes - tipos
 4 bytes - id
 4 bytes - len */
bool deserialize(PACKET *packet, uint8_t *buff)
{
	uint8_t padding;

	memcpy(&packet->head.type, buff, sizeof(packet->head.type));

	padding = sizeof(packet->head.type);
	memcpy(&packet->head.id, buff + padding, sizeof(packet->head.id));

	padding += sizeof(packet->head.id);
	memcpy(&packet->head.len, buff + padding, sizeof(packet->head.len));

	// converte ordem dos bytes para formato do host
	packet->head.type = ntohs(packet->head.type);
	packet->head.id 	= ntohl(packet->head.id);
	packet->head.len 	= ntohl(packet->head.len);

	padding += sizeof(packet->head.len);
	packet->data = (uint8_t *) malloc(packet->head.len);
	if(packet->data == NULL)
		return false;
	memcpy(packet->data, buff + padding, packet->head.len);

	return true;
}

// identifica os bytes contidos em header para confirmar tipo esperado
bool identifyConnection(void *tipoRecebido , void *tipoDesejado)
{
	return (memcmp(tipoRecebido, tipoDesejado, 2) == 0);
}

bool checkDuplicity(PACKET *actives, PACKET *newClient, size_t len)
{
	int count;
	for(count = 0; count < len; count++){
		if(actives[count].head.id == newClient->head.id)
			return true;
	}
	return false;
}

void addDataNewCon(SA_I *newDados, PACKET *actives, size_t len)
{
	char *ativo;

	if(identifyConnection(&actives[len -1].head.type, "CL"))
		ativo = "Cliente";
	else
		ativo = "Admin";

	actives[len - 1].adress = inet_ntoa(newDados->sin_addr);
	actives[len - 1].port   = ntohs(newDados->sin_port);

	printf("%s conectado %s:%d ID - %d\n",
					ativo,
					actives[len - 1].adress,
					actives[len - 1].port,
					actives[len - 1].head.id);
}

// sobreescreve 'a' com 'b'
void overWriteWithLast(PACKET *a, PACKET *b)
{
	*a = *b;
}

// funções utilizadas na main

int clearData(int sock)
{
	int i;
	int len;

	for (i = 0; i < totalClientes; i++){
		printf("\nclientes[%d].type - %d\n", i, clientes[i].head.type);
		printf("clientes[%d].id - %d\n", i, clientes[i].head.id);
		printf("clientes[%d].sock - %d\n", i, clientes[i].sock);
		printf("clientes[%d].port - %d\n", i, clientes[i].port);
	}
	puts("\n");
	for (i = 0; i < totalAdmins; i++){
		printf("\nadmins[%d].type - %d\n", i, admins[i].head.type);
		printf("admins[%d].id - %d\n", i, admins[i].head.id);
		printf("admins[%d].sock - %d\n", i, admins[i].sock);
		printf("admins[%d].port - %d\n", i, admins[i].port);
	}

	len = (totalClientes > totalAdmins) ? totalClientes : totalAdmins;
	for(i = 0; i < len; i++)
	{
		if(i < totalClientes){
			if (clientes[i].sock == sock)
			{
				printf("\nCliente %s:%d - ID %d desconectou\n",
								clientes[i].adress,
								clientes[i].port,
								clientes[i].head.id);

				totalClientes--;
				overWriteWithLast(&clientes[i], &clientes[totalClientes]);
				if(!resize_array(&clientes, totalClientes)){
					puts("Falha ao realocar array");
					return 0;
				}
				puts("Realocado com sucesso!") ;
				break;
			}
		}
		if(i < totalAdmins)
		{
			if (admins[i].sock == sock)
			{
				printf("\nAdmin %s:%d - ID %d desconectou\n",
								admins[i].adress,
								admins[i].port,
								admins[i].head.id);

				totalAdmins--;
				overWriteWithLast(&admins[i], &admins[totalAdmins]);
				if(!resize_array(&admins, totalAdmins)){
					puts("Falha ao realocar array");
					return 0;
				}
				puts("Realocado com sucesso!") ;
				break;
			}
		}
	}


	for (i = 0; i < totalClientes; i++){
		printf("\nclientes[%d].type - %d\n", i, clientes[i].head.type);
		printf("clientes[%d].id - %d\n", i, clientes[i].head.id);
		printf("clientes[%d].sock - %d\n", i, clientes[i].sock);
		printf("clientes[%d].port - %d\n", i, clientes[i].port);
	}
	puts("\n");
	for (i = 0; i < totalAdmins; i++){
		printf("\nadmins[%d].type - %d\n", i, admins[i].head.type);
		printf("admins[%d].id - %d\n", i, admins[i].head.id);
		printf("admins[%d].sock - %d\n", i, admins[i].sock);
		printf("admins[%d].port - %d\n", i, admins[i].port);
	}
	return 0;
}

/*
 * faz o encaminhamento das mensagens entre os cliente conectados
 */
int forwardMsg(int sock_in)
{
	uint8_t tempBuffer[MAXBUFF] = {0};
	PACKET *packet;
	uint16_t i;
  bool localizado;

	if (!receivAll(sock_in, tempBuffer, MAXBUFF)){
		close(sock_in);
		puts("DISCONECTED");
		return DISCONECTED;
	}

	packet = create_packet();
	if(packet == NULL){
		close(sock_in);
		free_packet(packet);
		erro("falha ao alocar packet temporario");
		return ERRO_MEMORY;
	}

	if(!deserialize(packet, tempBuffer)){
		close(sock_in);
		free_packet(packet);
		erro("falha ao deserializar dados, falha ao alocar espaço para data");
		return ERRO_MEMORY;
	}


	localizado = false;
	if(!memcmp(&packet->head.type, "CL", sizeof(packet->head.type)))
	{
		for(i = 0; i < totalAdmins; i++)
		{
			if(admins[i].head.id == packet->head.id)
      {
        localizado = true;
			  if(!sendAll(admins[i].sock, packet->data, packet->head.len + 1))
			  {
				  //clean_disconected(admins[i].sock);
				  return FAILSEND;
        }
      }
		} // não foi encontrado par com mesmo ID
    if (!localizado)
		{
			free_packet(packet);
			return PEER_NOT_FOUND;
		}
	}
	else if(!memcmp(&packet->head.type, "AD", sizeof(packet->head.type)))
	{
		for(i = 0; i < totalClientes; i++)
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
		if (!localizado)
		{
			free_packet(packet);
			packet = NULL;
			return PEER_NOT_FOUND;
		}
	}
	else
	{
		close(sock_in);
		free_packet(packet);
		return INVALID;
	}

	free_packet(packet);
	packet = NULL;
	return 0;
}

/* funcao testa nova conexão se o pacote recebidos
   é de algum tipo esperado, caso não seja returna 0 */
int validar()
{
	uint8_t tempBuffer[MAXBUFF] = {0};
	PACKET *tempPacket;
	SA_I dadosNewCon;
	socklen_t lenDadosCl = sizeof(dadosNewCon);
	int newCon;


	newCon = accept(fSockSv,(SA *) &dadosNewCon, &lenDadosCl);
	if (newCon < 0){
		erro("accpet");
		return 0;
	}

	if (!receivAll(newCon, tempBuffer, MAXBUFF)){
		close(newCon);
		erro("Erro ao receber dados, possivel desconexão do cliente");
		return 0;
	}

	tempPacket = create_packet();
	if(tempPacket == NULL){
		close(newCon);
		free_packet(tempPacket);
		erro("falha ao alocar memoria para packet temporario");
		return 0;
	}

	// preenche tempPacket com dados do tempBuffer, TYPE, ID, LEN E DATA
	if(!deserialize(tempPacket, tempBuffer)){
		close(newCon);
		free_packet(tempPacket);
		erro("falha ao deserializar dados, falha ao alocar espaço para data");
		return 0;
	}

   // identifica conexão
	 // caso o packet recebido seja do tipo cliente
	if(identifyConnection(&tempPacket->head.type, "CL"))
	{
		puts("Analisando conexão cliente...");

		if(checkDuplicity(clientes, tempPacket, totalClientes))
		{
			close(newCon);
			free_packet(tempPacket);
			puts("Cliente duplicado, deconectado ultimna solicitação");
			return 0;
		}
		else
		{
			totalClientes++;
			if(!resize_array(&clientes, totalClientes))
			{
				close(newCon);
				free_packet(tempPacket);
				erro("Falha ao realocar array clientes");
				return 0;
			}

			clientes[totalClientes - 1] = *tempPacket;
			free_packet(tempPacket);

			clientes[totalClientes - 1].sock = newCon;
			addDataNewCon(&dadosNewCon, clientes, totalClientes);

		}
	}
	 // se o pacote é do tipo admin
	else
	if(identifyConnection(&tempPacket->head.type, "AD"))
	{
		puts("Analisando conexão admin...");

		if(checkDuplicity(admins, tempPacket, totalAdmins))
		{
			close(newCon);
			free_packet(tempPacket);
			puts("Cliente duplicado, deconectado ultimna solicitação");
			return 0;
		}
		else
		{
			totalAdmins++;
			if(!resize_array(&admins, totalAdmins))
			{
				close(newCon);
				free_packet(tempPacket);
				erro("falho ao realocar array admins");
				return 0;
			}

			admins[totalAdmins - 1] = *tempPacket;	// copia id, tipo, len e data
			free_packet(tempPacket);

			admins[totalAdmins -1].sock = newCon;
			addDataNewCon(&dadosNewCon, admins, totalAdmins);

		}
	}
	else
	{ // desconecta caso a conexão não possa ser identificada
 		close(newCon);
		free_packet(tempPacket);
		return 0;
	}
	return newCon;
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
