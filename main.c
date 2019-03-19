#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>      // for socket()
#include <sys/socket.h>     // for socket()
#include <netinet/in.h>     // for socket()
#include <string.h>         // for strlen()
#include <arpa/inet.h>      // for inet_ntoa()
#include <unistd.h>         // for close()
#include <sys/time.h>       // for select()
#include <sys/types.h>      // for select()

#define PORT 5000

int main(void){
    char recvbuff[1000];
    char sendbuff[1000];    
    /* file descriptor socket local */
    int fSockSv = socket(AF_INET, SOCK_STREAM, 0);
    
    /* escrutura que ira receber os parametros locais */
    struct sockaddr_in dadosSv;
    dadosSv.sin_family = AF_INET;
    dadosSv.sin_port = htons(PORT);
    dadosSv.sin_addr.s_addr = INADDR_ANY; // INADDR_ANY = localhost
    memset(&dadosSv.sin_zero, 0, sizeof(dadosSv.sin_zero));
    
    /* permite que o socket seja vinculado a um endereço ja em uso,
    bom para casos que é necessario reiniciar o socket */
    int reuse = 1;
    setsockopt(fSockSv, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    socklen_t tDadosSv= sizeof(dadosSv);
    
    /* da socket ao 'fSockSv' o endereço e porta de
     comunição definida na struct 'dadosSv' */
    bind(fSockSv, (struct sockaddr *)&dadosSv, tDadosSv);

    /* habilita o socket a receber conexões */
    listen(fSockSv, 2);

    puts("escutando... ");

    while(true){
    
    int tamMsg = 0;                  /* recebe tamanho da msg enviado pelo cliente */
    int fSockCl = 0;                 /* file descriptor socket cliente */
    struct sockaddr_in dadosCl;      /* estrutura que recebe os dados do cliente, accept() */
    socklen_t tDadosCl = sizeof(dadosCl);

    /* accpet aguardando por conexões */
    if ((fSockCl = accept(fSockSv, 
              (struct sockaddr *)&dadosCl, &tDadosCl)) != -1){
        if ( fSockCl > 0){
        printf("conectado: %s:%d\n\n", 
                inet_ntoa(dadosCl.sin_addr),
                ntohs(dadosCl.sin_port));
        }
    }

        while(true){
            /* estrutura fd_set */
            fd_set rfds;
            
            /* zera a estrutura 'rfds' e monitora a
            entrada padrão e o socket cliente */
            FD_ZERO(&rfds);
            FD_SET(0, &rfds);
            FD_SET(fSockCl, &rfds);

           if(select(FD_SETSIZE, &rfds, NULL, NULL, NULL) < 0){
                puts("erro select()");
                exit(EXIT_FAILURE);
            }
            if(FD_ISSET(fSockCl, &rfds)){
                if((tamMsg = read(fSockCl, recvbuff, sizeof(recvbuff))) <= 0){
                   if(close(fSockCl) == 0){
                        puts("cliente desconectou!");
                        break;
                    }
                }else{
                    recvbuff[tamMsg] = '\0';
                    printf(recvbuff);
                }
            }
            else if(FD_ISSET(0, &rfds)){
                fgets(sendbuff, 1000, stdin);
                printf("comando enviado: %s", sendbuff);
                write(fSockCl, sendbuff, strlen(sendbuff));
            }
        }  /* while interno*/

    } /* while externo*/

    close(fSockSv);

    return 0;
}

