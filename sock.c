#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>	    // for inet_ntoa
#include <unistd.h>	    // for close()


char buff[1000];
char *shell = "/bin/sh";

char msgsock[] = {0x04, 0x01, 0x00, 0x50, 0xC0, 0xA8, 0x01, 0x01, 0x45, 0x45, 0x00};

int main(void){
    
    // file descriptor que identifica o socket local
    int fSock = socket(AF_INET, SOCK_STREAM, 0);
    
    // escrutura que ira receber os parametros do servidor
    struct sockaddr_in dadosSv;
    dadosSv.sin_family = AF_INET;
    dadosSv.sin_port = htons(5000);
    dadosSv.sin_addr.s_addr = inet_addr("127.0.0.1"); // INADDR_ANY = localhost
    memset(&dadosSv.sin_zero, 0, sizeof(dadosSv.sin_zero));
    

    socklen_t tDadosSv= sizeof(dadosSv);

    if ((connect(fSock, (struct sockaddr *)&dadosSv, tDadosSv)) < 0)
	puts("erro ao conectar");
    
    send(fSock, msgsock, strlen(msgsock), 0);
    
    recv(fSock, buff, sizeof(buff), 0);
    printf(buff);
    return 0;
}

