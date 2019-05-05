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

int clean_disconected(size_t sock)
{
	PACKET *TempPacketForDelete = create_packet();
	PACKET *testeRealloc = NULL;
	int i;

	for (i = 0; i < tot_client; i++){
		printf("\nclientes[%d].type - %d\n", i, clientes[i].head.type);
		printf("clientes[%d].id - %d\n", i, clientes[i].head.id);
		printf("clientes[%d].sock - %d\n", i, clientes[i].sock);
		printf("clientes[%d].port - %d\n", i, clientes[i].port);
	}

	for(i = 0; i < tot_client; i++)
	{
		printf("quantos vezes necessario procurar  - %d\n", i);
		if (clientes[i].sock == sock)
		{	/* troca o elemento encontrado pelo utimo da lista,
			 pois realloc ao diminuir retira a partir dos ultimos */

			printf("\nSock localizado - %ld\n", sock);
			printf("Clientes[%d].sock -  %d\n",i,  clientes[i].sock);

			printf("\ncliente %s:%d - ID %d desconectou\n",
							clientes[i].adress, clientes[i].port, clientes[i].head.id);

			// inveter posiçẽso do ultimo cliente, com cliente que desconectou
			*TempPacketForDelete 		= clientes[i];
			clientes[i] 						= clientes[tot_client - 1];
			clientes[tot_client -1] = *TempPacketForDelete;
			free(TempPacketForDelete);

			tot_client--;
			printf("Total de clientes - %ld\n", tot_client);
			if(tot_client == 0){
				free(clientes);
				free(admins);
				admins = NULL;
				clientes = NULL;
				exit(0);
				return 0;
			}

			puts("lipando...");
			break;
		}
	}
	printf("Total de clientes - realloc- %ld\n", tot_client);
	testeRealloc = resize_array(clientes, tot_client);
	if(testeRealloc != NULL){
		clientes = testeRealloc;
		testeRealloc = NULL;
		puts("Realocado com sucesso!") ;
	}
	else{
		puts("Falha ao realocar array");
		free(testeRealloc);
		return -1;
	}


	for (i = 0; i < tot_client; i++){
		printf("\nclientes[%d].type - %d\n", i, clientes[i].head.type);
		printf("clientes[%d].id - %d\n", i, clientes[i].head.id);
		printf("clientes[%d].sock - %d\n", i, clientes[i].sock);
		printf("clientes[%d].port - %d\n", i, clientes[i].port);
	}
	return 0;
}

/*
 * faz o encaminhamento das mensagens entre os cliente conectados
 */
int forwardMsg(int sock_in)
{
	uint8_t temp_buff[MAXBUFF] = {0};
	PACKET *packet;
	uint16_t i;
  bool localizado;

	if (!receivAll(sock_in, temp_buff, MAXBUFF)){
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

	if(!deserialize(packet, temp_buff)){
		close(sock_in);
		free_packet(packet);
		erro("falha ao deserializar dados, falha ao alocar espaço para data");
		return ERRO_MEMORY;
	}


	localizado = false;
	if(!memcmp(&packet->head.type, "CL", sizeof(packet->head.type)))
	{
		for(i = 0; i < tot_admin; i++)
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
		for(i = 0; i < tot_client; i++)
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


	int validar()
 {
	 struct sockaddr_in dadosCl; 			 // recebe os dados do cliente, por accept()
	 PACKET *temp_packet;							 // estrutura temporaria até a validação do pacote
	 uint8_t temp_buff[MAXBUFF] = {0}; // buff temporario até a inclusão dos dados em PACKET->data
	 socklen_t tDadosCl = sizeof(dadosCl);

	 int new_con = 0;		 		 // file descriptor socket cliente
	 uint16_t i;						 // contador
	 bool localizado;     // utilizada para verificar se o ID da conexão ja existe


	new_con = accept(fSockSv,(SA *) &dadosCl, &tDadosCl);
	if (new_con < 0){
		erro("accpet");
		return 0;
	}

	if (!receivAll(new_con, temp_buff, MAXBUFF)){
		close(new_con);
		erro("erro ao receber dados, possivel desconexão do cliente");
		return 0;
	}

	temp_packet = create_packet();
	if(temp_packet == NULL){
		close(new_con);
		free_packet(temp_packet);
		erro("falha ao alocar memoria para packet temporario");
		return 0;
	}

	// preenche temp_packet com dados do temp_buff, TYPE, ID, LEN E DATA
	if(!deserialize(temp_packet, temp_buff)){
		close(new_con);
		free_packet(temp_packet);
		erro("falha ao deserializar dados, falha ao alocar espaço para data");
		return 0;
	}

   // identifica conexão
	 // caso o packet recebido seja do tipo cliente
	 localizado = false;
	 if(!memcmp(&temp_packet->head.type, "CL", sizeof(temp_packet->head.type)))
	 {
		 puts("Conexão cliente recebida");
		 for(i = 0; i < tot_client; i++)		// verifica se o id ja esta na lista de clientes
		 {
			 if(clientes[i].head.id == temp_packet->head.id)
			 {
				 localizado = true;
				 close(new_con);
				 free_packet(temp_packet);
				 printf("ID %d cliente duplicado, deconectado ultimna solicitação\n",
				  			clientes[i].head.id);
				 return 0;
			 }
		 } // for localizar cliente

		 if(!localizado)
		 {
			 PACKET *temp_clientes = NULL;
			 tot_client++;
			 temp_clientes = realloc(clientes, tot_client * sizeof(PACKET));
			 if(temp_clientes == NULL){
				 close(new_con);
				 free_packet(temp_packet);
				 erro("falho ao realocar array clientes");
				 return 0;
			 }
			 else
			 {
				 clientes = temp_clientes;
				 temp_clientes = NULL;
			 }

			 printf("total cliente - %ld\n", tot_client);
			 clientes[tot_client - 1] = *temp_packet;
			 free_packet(temp_packet);

			 clientes[tot_client - 1].adress = inet_ntoa(dadosCl.sin_addr);
			 clientes[tot_client - 1].port   = ntohs(dadosCl.sin_port);
			 clientes[tot_client - 1].sock 	 = new_con;

       printf("Cliente conectado %s:%d ID - %d\n",
             clientes[tot_client - 1].adress, clientes[tot_client - 1].port, clientes[tot_client - 1].head.id);
		 }
	 }
	 // se o pacote é do tipo admin
	 else if(!memcmp(&temp_packet->head.type, "AD", sizeof(temp_packet->head.type)))
	 {
		 puts("Conexão admin recebida");
		 for(i = 0; i < tot_admin; i++)
		 {
			 if(admins[i].head.id == temp_packet->head.id)
			 {
				 localizado = true;
				 close(new_con);
				 free_packet(temp_packet);
				 printf("ID %d admin duplicado, deconectado ultima solicitação\n",
				  			admins[i].head.id);
				 return 0;
			 }
		 }

		 if(!localizado)
		 {
			 PACKET *temp_admins;
			 tot_admin++;
			 temp_admins = resize_array(admins, tot_admin);
			 if(temp_admins == NULL){
				 close(new_con);
				 free_packet(temp_packet);
				 erro("falho ao realocar array admins");
				 return 0;
			 }
			 else
			 		admins = temp_admins;

			 admins[tot_admin - 1] = *temp_packet;	// copia id, tipo, len e data
			 free_packet(temp_packet);

			 admins[tot_admin -1].adress = inet_ntoa(dadosCl.sin_addr);
			 admins[tot_admin -1].port   = ntohs(dadosCl.sin_port);
			 admins[tot_admin -1].sock 	 = new_con;

       printf("Admin conectado %s:%d ID - %d\n",
             admins[tot_admin - 1].adress, admins[tot_admin - 1].port, admins[tot_admin - 1].head.id);
		 }
	 }
	 else
	 { // desconecta caso a conexão não possa ser identificada
 	 		close(new_con);
			free_packet(temp_packet);
			return 0;
	 }
	 return new_con;
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
