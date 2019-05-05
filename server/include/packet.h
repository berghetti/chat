#ifndef _PACKET
#define _PACKET

// estruta que o servidor deve receber
struct header{
  uint32_t id;	 // id que identifica o cliente especifico
  uint32_t len;	 // tamanho do conteudo a ser recebido
  uint16_t type; // identifica o cliente se é 'CL' ou 'AD'
};

// estrutura da mensagem que o servidor armazena
typedef struct {
  struct header head; // cabeçaho do pacote
	uint8_t *data;			// dados da mensagem excluido o cabeçaho
  char *adress;       // endereço do cliente
  uint16_t port;      // porta de conexão do cliente
	int sock;				    // socket associado ao cliente
} PACKET;


PACKET * create_packet();

PACKET * resize_array(PACKET *old_packet, size_t tot_client);

void free_packet(PACKET *packet);

#endif // _PACKET
