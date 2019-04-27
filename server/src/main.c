// main.c

#include <locale.h>     // setlocale
#include <unistd.h>			// close, select
#include <sys/select.h>	// select
#include <string.h>			// memset

#include <server.h>			// in ./include

// armazana clientes e administradores conectados
PACKET *clientes;
PACKET *admins;

int fSockSv;
// zera os dados da conexão
void clearCon(int sock);

int main(void)
{
	setlocale(LC_ALL, "");

	// cria vetor com tamanho MAXCLIENT
	// acessa cada espaço com clientes[i].campo
	clientes = (PACKET *) malloc(sizeof(PACKET) * MAXCLIENT);
	admins = (PACKET *) malloc(sizeof(PACKET) * MAXCLIENT);

	memset(clientes, 0, sizeof(PACKET));
	memset(admins, 0, sizeof(PACKET));

	int i;
	int status;
	int fd;

	int maxFd;						/* valor do maior file descriptor */
	fd_set activeFdSet;   /* estrutura que recebe descritores ativos */
	fd_set readFdSet;     /* estrutura que é atualizada a cada iteração */

	if((fSockSv = startSocket()) < 0)
		erro("socket");

	FD_ZERO(&activeFdSet); 				 /* zera fd_set */
	FD_SET(fSockSv, &activeFdSet); // inclui o socket do servidor

	maxFd = fSockSv;

	puts("Escutando...");

	while(1)
	{
		readFdSet = activeFdSet;

		if(select(maxFd + 1, &readFdSet, NULL, NULL, NULL) < 0)
				erro("select");

		for(i = 0; i <= maxFd; i++)
		{
			if(FD_ISSET(i, &readFdSet))
			{
				/* caso seja uma nova conexão
				   verifica se é uma conexão validar
				   e aloca no vetor adequado, clientes ou admins */
				if(i == fSockSv)
				{	// retorna o file descriptor da conexão
					if( (fd = validar(&activeFdSet, admins, clientes)) > 0)
 						maxFd = (maxFd > fd) ? maxFd : fd;
				}
				else
				{
					puts("Recebendo dados...");
					status = forwardMsg(i, admins, clientes);
					switch (status)
					{
					case WAITPAIR:
						puts("Não localizado par com mesmo ID");
						continue;

					case FAILSEND:
					case DISCONECTED:
						FD_CLR(i, &activeFdSet);
						clearCon(i);
						close(i);
						break;
					} // switch
				}
			} //if FD_ISSET
		} // for i
	} // while true

	return 0;
}

void clearCon(int sock)
{
	int j;
	for(j = 0; j < MAXCLIENT; j++)
	{
		if(clientes[j].sock == sock){
			printf("Conexão cliente desconectou - %s:%d ID - %d!\n",
						clientes[j].adress, clientes[j].port, clientes[j].head.id);
			memset(&clientes[j], 0, sizeof(PACKET));
			break;
		}
		else if (admins[j].sock == sock)
		{
			printf("Conexão admin desconectou - %s:%d ID - %d\n",
							admins[j].adress, admins[j].port, admins[j].head.id);
			memset(&admins[j], 0, sizeof(PACKET));
			break;
		}
	}
}
