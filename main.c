#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/types.h>      // for socket()
#include <sys/socket.h>     // for socket()
#include <netinet/in.h>     // for socket()
#include <string.h>         // for strlen()
#include <arpa/inet.h>      // for inet_ntoa()
#include <unistd.h>         // for close()
#include <sys/time.h>       // for select()
#include <sys/types.h>      // for select()

#define PORT    5000
#define MAXBUFF 1000

void erro(char *msg){
    perror(msg);
    exit(EXIT_FAILURE);
}
    
int startSocket(void){
    /* file descriptor socket local */
    int fSockSv;
    if ((fSockSv = socket(AF_INET, SOCK_STREAM, 0)) < 1){
        erro("socket");
    }
    
    /* escrutura que ira receber os parametros locais */
    struct sockaddr_in dadosSv;
    dadosSv.sin_family = AF_INET;
    dadosSv.sin_port = htons(PORT);
    dadosSv.sin_addr.s_addr = INADDR_ANY; // INADDR_ANY = localhost
    memset(&dadosSv.sin_zero, 0, sizeof(dadosSv.sin_zero));
    
    /* permite que o socket seja vinculado a um endereço ja em uso,
    bom para casos que é necessario reiniciar o socket */
    int reuse = 1;
    if ((setsockopt(fSockSv, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse))) < 0){
        erro("setsockopt");
    }

    socklen_t tDadosSv = sizeof(dadosSv);
    
    /* da socket ao 'fSockSv' o endereço e porta de
     comunição definida na struct 'dadosSv' */
    if ((bind(fSockSv, (struct sockaddr *)&dadosSv, tDadosSv)) < 0){
        erro("bind");
    }

    /* habilita o socket a receber conexões */
    if ((listen(fSockSv, 5)) < 0){
        erro("listen");
    }

    return fSockSv;
}


int main(void){
    char recvbuff[MAXBUFF] = {0};
    char sendbuff[MAXBUFF] = {0};
    int fSockSv;
    fd_set activeFdSet;                /* estrutura fd_set recebe descriptors ativos*/
    fd_set readFdSet;                  /* estrutura fd_set recebe descriptors para monitorar */

    if((fSockSv = startSocket()) < 0){
        erro("socket"); 
    }

    /* inicializa fd_set */
    FD_ZERO(&activeFdSet);
    FD_SET(fileno(stdin), &activeFdSet);

    puts("Escutando...");

    while(true){

        int tamMsg  = 0;            /* tamanho da msg enviada pelo cliente */
        int fSockCl = 0;            /* file descriptor socket cliente */
        int maxFd   = 0;            /* valor do maior file decriptor */
        struct sockaddr_in dadosCl; /* recebe os dados do cliente, por accept() */
        socklen_t tDadosCl = sizeof(dadosCl);

        /* accpet aguardando por conexões */
        if ((fSockCl = accept(fSockSv, 
                              (struct sockaddr *) &dadosCl,
                              &tDadosCl)) < 0){
            erro("accpet");
        }

        maxFd = (fSockCl > maxFd) ? fSockCl : maxFd;

        /* inseri socket cliente 'fSockCl' no fd_set */
        FD_SET(fSockCl, &activeFdSet); 

        printf("Conectado: %s:%d\n\n", 
                inet_ntoa(dadosCl.sin_addr),
                ntohs(dadosCl.sin_port));

        while(true){

            readFdSet = activeFdSet;

            /* aguardando algum file descriptor
             disponivel para leitura */
            if(select(maxFd + 1, &readFdSet, NULL, NULL, NULL) < 0){
                perror("select");
                exit(EXIT_FAILURE);
            }

            if(FD_ISSET(fSockCl, &readFdSet)){
                memset(recvbuff, 0, MAXBUFF);
                if ((tamMsg = recv(fSockCl, recvbuff, MAXBUFF, 0)) > 0){
                    recvbuff[tamMsg] = '\0'; /* inserir caracter NUL no fim da msg */
                    printf("%s", recvbuff);
                    fflush(stdout);         /* força descarga dos dados mesmo que não encontre '\n' */   
                }
                else {
                    close(fSockCl);
                    FD_CLR(fSockCl, &activeFdSet);
                    puts("\nCliente desconectou!");
                    break;
                }
            }            
            else if(FD_ISSET(fileno(stdin), &readFdSet)){
                memset(sendbuff, 0, MAXBUFF);
                read(fileno(stdin), sendbuff, MAXBUFF);
                if((send(fSockCl, sendbuff, strlen(sendbuff), 0)) < 0){
                    erro("send");
                }
            }
        }  /* while interno */
        puts("Aguardando novas conexoes...\n");
    } /* while externo */

    close(fSockSv);
    return 0;
}

