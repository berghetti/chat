// main.c

#include <locale.h>     // for setlocale()
#include <unistd.h>			// for close()

#include <server.h>


int main(void)
{
	setlocale(LC_ALL, "");

	// cria vetor com tamanho MAXCLIENT
	// acessa cada espaço com clientes[i].campo
	PACKET *clientes = (PACKET *) malloc(sizeof(PACKET) * MAXCLIENT);
	PACKET *admins = (PACKET *) malloc(sizeof(PACKET) * MAXCLIENT);


	int i, j;
	int status;
	int maxFd;						/* valor do maior file descriptor */
	fd_set activeFdSet;   /* estrutura que recebe descritores ativos */
	fd_set readFdSet;     /* estrutura que é atualizada a cada iteração */

	if((fSockSv = startSocket()) < 0)
		erro("socket");

	FD_ZERO(&activeFdSet); /* zera fd_set */
	FD_SET(fSockSv, &activeFdSet);

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
							if (status == WAITPAIR){
								continue;
								puts("aqui2");
							}
							else if (status == FAILSEND)
							{
								puts("Não localizado par com mesmo ID");
								FD_CLR(i, &activeFdSet);
								close(i);
								for(j = 0; j < MAXCLIENT; j++)
								{
									if(clientes[j].sock == i){
										clientes[j].id = 0;
										printf("Conexão cliente %s:%d desconectou!\n",
										 			clientes[j].adress, clientes[j].port);
										break;
									}
									else if (admins[j].sock == i)
									{
										admins[j].id = 0;
										printf("Conexão admin %s:%d desconectou!\n",
										 				admins[j].adress, admins[j].port);
										break;
									}
								}	// for j
							}
						}
					}
				} // for i

	}

	return 0;
}
