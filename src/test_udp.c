#include "config.h"
#include <zephyr.h>
#include <stdio.h>
#include <net/socket.h>
#include "test_udp.h"

#define MDM_MAX_SOCKETS 7

void testUDPCounter()
{
    printf("Counting bytes\n");
    int err;

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0)
    {
        printf("Error opening socket: %d\n", sock);
        return;
    }

    static struct sockaddr_in remote_addr = {
        sin_family : AF_INET,
    };
    remote_addr.sin_port = htons(1234);

    net_addr_pton(AF_INET, "172.16.15.14", &remote_addr.sin_addr);

    err = connect(sock, (struct sockaddr *)&remote_addr, sizeof(remote_addr));
    if (err < 0)
    {
        printf("Unable to connect to backend: %d\n", err);
        close(sock);
        return;
    }

    char buffer[255];
    char dump[512];
    memset(dump, 0, sizeof(dump));
    memset(buffer, 0, sizeof(buffer));
    for (int i = 0; i < sizeof(buffer); i++) {
        buffer[i] = '.';
        int len = 21 + strlen(buffer);
        sprintf(dump, "%03d bytes on the wall%s", len, buffer);

        printf("Sending %d bytes\n", len);
        err = send(sock, dump, len, 0);
        if (err < len)
        {
            printf("Error sending (%d bytes sent): %d\n", len, err);
            close(sock);
            return;
        }

        k_sleep(2000);
    }
    close(sock);
}


static char udp_message[64];
static int sockets[MDM_MAX_SOCKETS];

void close_sockets() {
    for (int i = 0; i < MDM_MAX_SOCKETS; i++) {
        close(sockets[i]);
    }
}

void testUDP()
{
    int err = 0;
    printf("Creating sockets\n");
    for (int i = 0; i < MDM_MAX_SOCKETS; i++) {
        sockets[i] = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (sockets[i] < 0)
        {
            printf("Error opening socket: %d\n", err);
            close_sockets();
            return;
        }
    }

    printf("Connecting sockets\n");
    for (int i = 0; i < MDM_MAX_SOCKETS; i++) {
        static struct sockaddr_in remote_addr = {
            sin_family : AF_INET,
        };
        remote_addr.sin_port = htons(1234);

        net_addr_pton(AF_INET, "172.16.15.14", &remote_addr.sin_addr);

        err = connect(sockets[i], (struct sockaddr *)&remote_addr, sizeof(remote_addr));
        if (err < 0)
        {
            printf("Unable to connect to backend: %d\n", err);
            close_sockets();
            return;
        }
    }

    printf("Sending messages\n");
    for (int i = 0; i < MDM_MAX_SOCKETS; i++) {
        sprintf(udp_message, "Hello this is socket %d", i);
        err = send(sockets[i], udp_message, strlen(udp_message), 0);
        if (err < strlen(udp_message))
        {
            printf("Error sending: %d\n", err);
            close_sockets();
            return;
        }
    }

    memset(udp_message, 0, sizeof(udp_message));

    printf("Polling\n");

    // Poll all sockets
    bool data = false;
    while (!data) {
        struct pollfd polls[MDM_MAX_SOCKETS];
        for (int i = 0; i < MDM_MAX_SOCKETS; i++) {
            polls[i].fd = sockets[i];
            polls[i].events = POLLIN|POLLOUT;
        }
        err = poll(polls, MDM_MAX_SOCKETS, 1000);
        if (err < 0) {
            printf("Error polling: %d\n", err);
            close_sockets();
            return;
        }
        for (int i = 0; i < MDM_MAX_SOCKETS; i++) {
            if ((polls[i].revents & POLLIN) == POLLIN) {
                printf("Got data on socket fd=%d\n", sockets[i]);
                memset(udp_message, 0, sizeof(udp_message));
                err = recv(sockets[i], udp_message, sizeof(udp_message), 0);
                if (err < 0) {
                    printf("Error receiving on socket %d: %d\n", i, err);
                    close_sockets();
                    return;
                }
                printf("Received %d bytes (%s) on socket %d\n", err, udp_message, sockets[i]);
                data = true;
            }
        }
    }
    close_sockets();

    printf("UDP test complete\n");
}