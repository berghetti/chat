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

	memset(clientes, 0, sizeof(PACKET) * MAXCLIENT);
	memset(admins, 0, sizeof(PACKET) * MAXCLIENT);

	int i;
	int status;
	int fd;

	int maxFd;						/* valor do maior file descriptor */
	fd_set active_set;   /* estrutura que recebe descritores ativos */
	fd_set read_set;     /* estrutura que é atualizada a cada iteração */

	if((fSockSv = startSocket()) < 0)
		erro("socket");

	FD_ZERO(&active_set); 				 /* zera fd_set */
	FD_SET(fSockSv, &active_set); // inclui o socket do servidor

	maxFd = fSockSv;

	puts("Escutando...");

	while(1)
	{
		read_set = active_set;

		if(select(maxFd + 1, &read_set, NULL, NULL, NULL) < 0)
				erro("select");

		for(i = 0; i <= maxFd; i++)
		{
			if(FD_ISSET(i, &read_set))
			{
				/* caso seja uma nova conexão
				   verifica se é uma conexão validar
				   e aloca no vetor adequado, clientes ou admins */
				if(i == fSockSv)
				{	// retorna o file descriptor da conexão
					if ( (fd = validar(admins, clientes)) > 0)
					{
						FD_SET(fd, &active_set);
						maxFd = (maxFd > fd) ? maxFd : fd;
					}
					else
						continue; // conexão invalida, pula para proxima iteração
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
						FD_CLR(i, &active_set);
						clearCon(i);
						close(i);
						continue;
					} // switch
				}
			} //if FD_ISSET
		} // for i
	} // while true
	free(clientes);
	free(admins);
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
