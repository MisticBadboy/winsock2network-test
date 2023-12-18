#include "..\include\network.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "AdvApi32.lib")

WSADATA wsadata;

#define DEFAULT_PORT "27015"

struct addrinfo *result = NULL, *ptr = NULL, hints;

SOCKET ListenSocket = INVALID_SOCKET, ClientSocket, ConnectSocket;

int setupsocket(char **argv)
{
    int Result = WSAStartup(MAKEWORD(2, 2), &wsadata);
#if defined(SERVER)
    if (Result != 0)
    {
        printf("WSAStartup failed: %d\n", Result);
        return 1;
    }
#endif
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
#if defined(SERVER)
    hints.ai_flags = AI_PASSIVE;
#endif
#if defined(CLIENT)
    Result = getaddrinfo(argv[1], DEFAULT_PORT, &hints, &result);
#elif defined(SERVER)
    Result = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
#endif
    if (Result != 0)
    {
        printf("getaddrinfo failed : %d\n", Result);
        WSACleanup();
        return 1;
    }
#if defined(SERVER)
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET)
    {
        printf("Error at socket(): %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }
#else
    ptr = result;
    ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
    if (ConnectSocket == INVALID_SOCKET)
    {
        printf("Error at socket(): %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }
#endif
    return 0;
}

int bindport()
{
#if defined(SERVER)
    int Result = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (Result == SOCKET_ERROR)
    {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }
    freeaddrinfo(result);
#else
    int Result = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
    if (Result == SOCKET_ERROR)
    {
        closesocket(ConnectSocket);
        ConnectSocket = INVALID_SOCKET;
    }

    // Should really try the next address returned by getaddrinfo
    // if the connect call failed
    // But for this simple example we just free the resources
    // returned by getaddrinfo and print an error message

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET)
    {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return 1;
    }
#endif
    return 0;
}

#define DEFAULT_BUFLEN 512

int portlisten()
{
    printf("Listening...\n");
    char recvbuf[DEFAULT_BUFLEN];
    int Result, SendResult;
    int recvbuflen = DEFAULT_BUFLEN;
    if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        printf("Listen failed with error: %ld\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }
    ClientSocket = accept(ListenSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET)
    {
        printf("accept failed: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }
    printf("Connection Established\n");

    closesocket(ListenSocket);

    while (1)
    {
        Result = recv(ClientSocket, recvbuf, recvbuflen, 0);
        if (Result > 0)
        {
            printf("Bytes received: %d\n", Result);
            printf("%s\n", recvbuf);
            SendResult = send(ClientSocket, recvbuf, Result, 0);
            if (SendResult == SOCKET_ERROR)
            {
                printf("send failed: %d\n", WSAGetLastError());
                closesocket(ClientSocket);
                WSACleanup();
                return 1;
            }
            printf("Bytes sent: %d\n", SendResult);
        }
        else if (Result == 0)
        {
            printf("Connection closing...\n");
            break;
        }
        else
        {
            printf("recv failed: %d\n", WSAGetLastError());
            closesocket(ClientSocket);
            WSACleanup();
            return 1;
        }
    }
}

int shutdownsockets()
{
#if defined(SERVER)
    int Result = shutdown(ClientSocket, SD_SEND);
    if (Result == SOCKET_ERROR)
    {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ClientSocket);
        WSACleanup();
        return 1;
    }

    closesocket(ClientSocket);
    WSACleanup();
#endif
    return 0;
}

int senddata(const char *sendbuf)
{
    int recvbuflen = DEFAULT_BUFLEN;
    char recvbuf[DEFAULT_BUFLEN];

    int Result;
    Result = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0);
    if (Result == SOCKET_ERROR)
    {
        printf("send failed: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    printf("Bytes Sent: %ld\n", Result);

    Result = shutdown(ConnectSocket, SD_SEND);
    if (Result == SOCKET_ERROR)
    {
        printf("shutdown failed: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    do
    {
        Result = recv(ConnectSocket, recvbuf, recvbuflen, 0);
        if (Result > 0)
            printf("Bytes received: %d\n", Result);
        else if (Result == 0)
            printf("Connection closed\n");
        else
            printf("recv failed: %d\n", WSAGetLastError());
    } while (Result > 0);

    Result = shutdown(ConnectSocket, SD_SEND);
    if (Result == SOCKET_ERROR)
    {
        printf("shutdown failed: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }
    closesocket(ConnectSocket);
    WSACleanup();

    return 0;
}