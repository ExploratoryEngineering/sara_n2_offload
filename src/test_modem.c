#include "config.h"
#include <logging/log.h>
#define LOG_LEVEL APP_LOG_LEVEL
LOG_MODULE_REGISTER(modem_test);

#include <zephyr.h>
#include <stdio.h>


#include "comms.h"
#include "at_commands.h"

#include "test_modem.h"

static int received = 0;
static void recv_cb(int sockfd, size_t len)
{
    LOG_INF("Got %d bytes on socket %d", len, sockfd);
    received = len;
}


void testModem()
{
    int sockfd = -1;

    receive_callback(recv_cb);

    modem_write("AT+NSOCR=\"DGRAM\",17,6001,1\r");
    if (atnsocr_decode(&sockfd) != AT_OK)
    {
        LOG_ERR("Unable to decode nsocr");
        return;
    }
    modem_write("ATI\r");
    atnsocl_decode();

    modem_write("AT+NSOST=0,\"172.16.15.14\",1234,6,\"AABBAAAABBAA\"\r");
    int fd = -1;
    size_t size = 0;
    if (atnsost_decode(&fd, &size) != AT_OK)
    {
        LOG_ERR("NSOS sent error nsost");
        return;
    }
    LOG_INF("Message sent (fd=%d,len=%d), waiting for response", fd, size);

    while (received == 0)
    {
        k_sleep(1000);
    }
    char ip[32];
    char data[34];
    int port;
    memset(data, 0, sizeof(data));
    size_t received;
    size_t remaining;
    modem_write("AT+NSORF=0,32\r");
    if (atnsorf_decode(&sockfd, (char *)&ip, &port, (uint8_t *)&data, &received, &remaining) != AT_OK)
    {
        LOG_ERR("Unable to decode nsorf");
    }

    modem_write("AT+NSOCL=0\r");
    if (atnsocl_decode() != AT_OK)
    {
        LOG_ERR("Unable to decode nsocl");
    }
}
