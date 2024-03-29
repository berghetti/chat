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

/* função auxiliar que garante o rebecimento de
   todos os dados conforme especificado no campo 'len'
	 do cabeçalho da mensagem */
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

/* função auxiliar que garante o envio de todos
   os dados conforme especificado no campo 'len'
	 do cabeçaho da mensagem */
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

/* função auxiliar que deserializa os bytes
   em uma strutura do tipo 'PACKET' */
bool deserialize(PACKET *packet, uint8_t *buff)
{
	/*cabeçalho
	 2 bytes - tipos
	 4 bytes - id
	 4 bytes - len */
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
	if(packet->data == NULL){
		free(packet->data);
		return false;
	}

	memcpy(packet->data, buff + padding, packet->head.len);

	return true;
}

/* função auxiliar que retorno se o pacote é do tipo esperado */
bool identifyConnection(void *tipoRecebido , void *tipoDesejado)
{
	return (memcmp(tipoRecebido, tipoDesejado, 2) == 0);
}

/* função auxiliar examine array do tipo PACKET
   em busca de um ID especifico */
bool checkDuplicatedId(PACKET *array, uint32_t id, size_t len)
{
	int count;
	for(count = 0; count < len; count++){
		if(array[count].head.id == id)
			return true;
	}
	return false;
}

// funções utilizadas na main

/* localiza a qual user pertence o socket que
	 desconectou, copia o ultimo user do array para a posição do
	 user que desconectou e redimensiona o array com reallc */
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
				clientes[i] = clientes[totalClientes]; // copia ultima posição para posição que desconectou
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
				admins[i] = admins[totalAdmins];
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

/* faz o encaminhamento das mensagens entre admin x clientes */
int forwardMsg(int sock_in)
{
	uint8_t tempBuffer[MAXBUFF] = {0};
	PACKET *tempPacket = NULL;
	bool localizado = false;
	uint16_t i;


	if (!receivAll(sock_in, tempBuffer, MAXBUFF)){
		close(sock_in);
		puts("DISCONECTED");
		return DISCONECTED;
	}

	tempPacket = create_packet();
	if(tempPacket == NULL){
		close(sock_in);
		free_packet(tempPacket);
		return ERRO_MEMORY;
	}

	if(!deserialize(tempPacket, tempBuffer)){
		close(sock_in);
		free_packet(tempPacket);
		erro("falha ao deserializar dados, falha ao alocar espaço para data");
		return ERRO_MEMORY;
	}

	if(identifyConnection(&tempPacket->head.type, "CL"))
	{
		for(i = 0; i < totalAdmins; i++)
		{
			if(admins[i].head.id == tempPacket->head.id)
      {
        localizado = true;
			  if(!sendAll(admins[i].sock, tempPacket->data, tempPacket->head.len))
				  return FAILSEND;

				break;
      }
		}

		free_packet(tempPacket);
		tempPacket = NULL;

		if(localizado)
			return 0;
    else // não foi encontrado par com mesmo ID
			return  PEER_NOT_FOUND;

	}
	else
	if(identifyConnection(&tempPacket->head.type, "AD"))
	{
		for(i = 0; i < totalClientes; i++)
		{
			if(clientes[i].head.id == tempPacket->head.id)
      {
        localizado = true;
        if(!sendAll(clientes[i].sock, tempPacket->data, tempPacket->head.len))
  				return FAILSEND;
      }
		}

		free_packet(tempPacket);
		tempPacket = NULL;

		if(localizado)
			return 0;
    else // não foi encontrado par com mesmo ID
			return  PEER_NOT_FOUND;

	}
	else
	{
		close(sock_in);
		free_packet(tempPacket);
		tempPacket = NULL;
		return INVALID;
	}
}

/* funcao testa nova conexão se o pacote recebido (cliente ou admin)
   é de algum tipo esperado, caso não seja returna 0 */
int validar(void)
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

		if(checkDuplicatedId(clientes, tempPacket->head.id, totalClientes))
		{
			close(newCon);
			free_packet(tempPacket);
			tempPacket = NULL;
			puts("Cliente duplicado, deconectado ultimna solicitação");
			return 0;
		}
		else
		{
			totalClientes++;
			if(!resize_array(&clientes, totalClientes)){
				close(newCon);
				free_packet(tempPacket);
				tempPacket = NULL;
				erro("Falha ao realocar array clientes");
				return 0;
			}

			clientes[totalClientes - 1] = *tempPacket;
			free_packet(tempPacket);
			tempPacket = NULL;

			clientes[totalClientes - 1].sock   = newCon;
			clientes[totalClientes - 1].adress = inet_ntoa(dadosNewCon.sin_addr);
			clientes[totalClientes - 1].port   = ntohs(dadosNewCon.sin_port);

			printf("Cliente conectado %s:%d ID - %d\n",
							clientes[totalClientes - 1].adress,
							clientes[totalClientes - 1].port,
							clientes[totalClientes - 1].head.id);
		}
	}
	 // se o pacote é do tipo admin
	else
	if(identifyConnection(&tempPacket->head.type, "AD"))
	{
		puts("Analisando conexão admin...");

		if(checkDuplicatedId(admins, tempPacket->head.id, totalAdmins))
		{
			close(newCon);
			free_packet(tempPacket);
			tempPacket = NULL;
			puts("Cliente duplicado, deconectado ultimna solicitação");
			return 0;
		}
		else
		{
			totalAdmins++;
			if(!resize_array(&admins, totalAdmins)){
				close(newCon);
				free_packet(tempPacket);
				tempPacket = NULL;
				erro("falho ao realocar array admins");
				return 0;
			}

			admins[totalAdmins - 1] = *tempPacket;	// copia id, tipo, len e data
			free_packet(tempPacket);
			tempPacket = NULL;

			admins[totalAdmins -1].sock    = newCon;
			admins[totalAdmins - 1].adress = inet_ntoa(dadosNewCon.sin_addr);
			admins[totalAdmins - 1].port   = ntohs(dadosNewCon.sin_port);

			printf("Admin conectado %s:%d ID - %d\n",
							admins[totalAdmins - 1].adress,
							admins[totalAdmins - 1].port,
							admins[totalAdmins - 1].head.id);
		}
	}
	else
	{ // desconecta caso a conexão não possa ser identificada
 		close(newCon);
		free_packet(tempPacket);
		tempPacket = NULL;
		return 0;
	}
	return newCon;
}

/* inicializa o socket */
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
		exit(EXIT_FAILURE);
	}

	/* habilita o socket a receber conexões */
	if ((listen(fSockSv, 128) < 0))
	{
		erro("listen");
		exit(EXIT_FAILURE);
	}

	return fSockSv;
}

void erro(char *msg)
{
	perror(msg);
	//exit(EXIT_FAILURE);
}
