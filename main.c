#include <stdio.h>
#include "include/network.h"

int main(int *argv, char **argc)
{
    printf("Starting...\n");
    while (1)
    {
        if (setupsocket(NULL))
        {
            return 1;
        }
        printf("Socket setup was a success\n");
        if (bindport())
        {
            return 1;
        }
        printf("Port has been binded to\n");
        if (portlisten())
        {
            printf("Port failed\n");
        }
        shutdownsockets();
    }
    printf("Exiting...\n");
}