// main.c

#include <locale.h>     // setlocale
#include <unistd.h>			// close, select
#include <sys/select.h>	// select
#include <string.h>			// memset

#include <server.h>			// in ./include

// armazana clientes e administradores conectados
PACKET *clientes = NULL;
PACKET *admins = NULL;

uint32_t totalClientes = 0;	// total de clientes no servidor
uint32_t totalAdmins = 0;	// total de admins no servidor
int fSockSv;


int main(void)
{
	setlocale(LC_ALL, "");

	int i;
	int status;
	int fd;
	int maxFd;					 /* valor do maior file descriptor */
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
		printf("\n\n total de users ativos - %d\n\n", totalAdmins + totalClientes);
		if(select(maxFd + 1, &read_set, NULL, NULL, NULL) < 0)
				erro("select");

		for(i = 0; i <= maxFd; i++)
		{
			if(FD_ISSET(i, &read_set))
			{
				/* caso seja uma nova conexão,
				   verifica se é uma conexão validar
				   e aloca no vetor adequado, clientes ou admins */
				if(i == fSockSv)
				{	// retorna o file descriptor da conexão
					fd = validar();
					if (!fd)
						continue; // conexão invalida, pula para proxima iteração
					else{
						FD_SET(fd, &active_set);
						maxFd = (maxFd > fd) ? maxFd : fd;
					}
				}
				else
				{
					puts("Recebendo dados...");
					status = forwardMsg(i);
					switch (status)
					{
					case PEER_NOT_FOUND:
						puts("Não localizado par com mesmo ID");
						break;

					case FAILSEND:
					case DISCONECTED:
						FD_CLR(i, &active_set);
						clearData(i);
						break;

					case ERRO_MEMORY:
						erro("falha ao alocar/realocar memoria");
						break;

					case INVALID:
						exit(EXIT_FAILURE);

					default:
						break;
					}
				}
			}
		}
	}
	return 0;
}
