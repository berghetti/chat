// main.c

#include <locale.h>     // for setlocale()
#include <unistd.h>			// for close, select
#include <sys/time.h>   // for select()
#include <sys/types.h>  // for select()

#include <server.h>

// armazana clientes e administradores conectados
PACKET *clientes;
PACKET *admins;

// zera o ID da conexão
void clearCon(int sock);

int main(void)
{
	setlocale(LC_ALL, "");

	// cria vetor com tamanho MAXCLIENT
	// acessa cada espaço com clientes[i].campo
	clientes = (PACKET *) malloc(sizeof(PACKET) * MAXCLIENT);
	admins = (PACKET *) malloc(sizeof(PACKET) * MAXCLIENT);

	int i;
	int status;
	int maxFd;						/* valor do maior file descriptor */
	fd_set activeFdSet;   /* estrutura que recebe descritores ativos */
	fd_set readFdSet;     /* estrutura que é atualizada a cada iteração */

	if((fSockSv = startSocket()) < 0)
		erro("socket");

	FD_ZERO(&activeFdSet); 				 /* zera fd_set */
	FD_SET(fSockSv, &activeFdSet); // inclui o socket do servidor

	maxFd = fSockSv;

	puts("Escutando...");

	while(true)
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
				{
					validar(&activeFdSet, &maxFd, admins, clientes);
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
			clientes[j].id = 0;
			printf("Conexão cliente %s:%d desconectou!\n",
						clientes[j].adress, clientes[j].port);
			break;
		}
		else if (admins[j].sock == sock)
		{
			admins[j].id = 0;
			printf("Conexão admin %s:%d desconectou!\n",
							admins[j].adress, admins[j].port);
			break;
		}
	}
}
