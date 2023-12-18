#include <stdio.h>
#include "include/network.h"

int main(int *argv, char **argc)
{
    printf("Starting...\n");
    if (setupsocket(argc))
    {
        return 1;
    }
    printf("Socket setup was a success\n");
    if (bindport())
    {
        return 1;
    }
    senddata("Please help");
    printf("Exiting...\n");
}