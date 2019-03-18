#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdbool.h>
#include <arpa/inet.h>	    // for inet_ntoa
#include <unistd.h>	    // for close()


const char *BANNER = "BANNER SERVIDOR\n";
char buff[1000];


int main(void){
    
    // file descriptor que identifica o socket local
    int fSockSv = socket(AF_INET, SOCK_STREAM, 0);
    
    // escrutura que ira receber os parametros locais
    struct sockaddr_in dadosSv;
    dadosSv.sin_family = AF_INET;
    dadosSv.sin_port = htons(5000);
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

    // habilita o socket a receber conexões
    listen(fSockSv, 2);

    puts("escutando... ");
    
    while(true){
	
	int tamMsg = 0;
	int fSockCl = 0;
	struct sockaddr_in dadosCl;
    	socklen_t tDadosCl = sizeof(dadosCl);
	
	// accpet aguardando por conexões
	if ((fSockCl = accept(fSockSv, 
		      (struct sockaddr *)&dadosCl, &tDadosCl)) != -1){
	    if ( fSockCl > 0){
		printf("conectado: %s\n", inet_ntoa(dadosCl.sin_addr));
	    }
	}
	

	while((tamMsg = recv(fSockCl, buff, 1000, 0)) > 0){
	   
	    buff[tamMsg] = '\0';
	    printf("tamMsg: %d\n", tamMsg);
	    printf("msg recebida: %s\n", buff);    
	    
	    send(fSockCl, stdin, strlen(stdin));
	}

	puts("recv ok");

	send(fSockCl, BANNER, strlen(BANNER), 0);
	close(fSockCl);
	close(fSockSv);
    }

    return 0;
}

