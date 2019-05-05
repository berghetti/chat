// main.c

#include <locale.h>     // setlocale
#include <unistd.h>			// close, select
#include <sys/select.h>	// select
#include <string.h>			// memset

#include <server.h>			// in ./include

// armazana clientes e administradores conectados
PACKET *clientes = NULL;
PACKET *admins = NULL;

size_t tot_client = 0;	// total de clientes no servidor
size_t tot_admin = 0;	// total de admins no servidor
int fSockSv;


int main(void)
{
	setlocale(LC_ALL, "");

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
					if ( (fd = validar()) > 0){
						FD_SET(fd, &active_set);
						maxFd = (maxFd > fd) ? maxFd : fd;
					}
					else
						continue; // conexão invalida, pula para proxima iteração
				}
				else
				{
					puts("Recebendo dados...");
					status = forwardMsg(i);
					switch (status)
					{
					case PEER_NOT_FOUND:
						puts("Não localizado par com mesmo ID");
						continue;

					case ERRO_MEMORY:
						continue;

					case FAILSEND:
					case DISCONECTED:
						FD_CLR(i, &active_set);
						clean_disconected(i);
						close(i);
						// clearCon(i);
						continue;
					} // switch
				}
			} //if FD_ISSET
		} // for i
	} // while true
	// free_packet(clientes);
	// free_packet(admins);
	return 0;
}
