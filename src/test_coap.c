#include "config.h"
#include <logging/log.h>
#define LOG_LEVEL APP_LOG_LEVEL
LOG_MODULE_REGISTER(coap_test);

#include <zephyr.h>
#include <stdio.h>
#include <net/socket.h>
#include <net/coap.h>
#include "test_coap.h"


#define COAP_HOST "172.16.15.14"
#define COAP_PORT 5683
#define COAP_PAYLOAD "Hello there, I'm sent via CoAP and I'm a really long CoAP message long long and way beyond 80 characters that will break"
#define COAP_PATH "coap/zephyr20/broken"

char payload[100];

void testCoAP()
{
#define CBUF_LEN 200
    char cbuffer[CBUF_LEN];
    int err;
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0)
    {
        LOG_ERR("Error opening socket: %d", sock);
        return;
    }

    LOG_INF("Sending CoAP packet");
    static struct sockaddr_in remote_addr = {
        sin_family : AF_INET,
        sin_port : htons(COAP_PORT),
    };
    net_addr_pton(AF_INET, COAP_HOST, &remote_addr.sin_addr);
    err = connect(sock, (struct sockaddr *)&remote_addr, sizeof(remote_addr));
    if (err < 0)
    {
        LOG_ERR("Unable to connect to backend: %d", err);
        close(sock);
        return;
    }

    struct coap_packet p;
    coap_packet_init(&p, payload, sizeof(payload), 1, COAP_TYPE_CON, 8, coap_next_token(), COAP_METHOD_POST, coap_next_id());
    coap_packet_append_option(&p, COAP_OPTION_URI_PATH, COAP_PATH, strlen(COAP_PATH));
    coap_packet_append_payload_marker(&p);
    coap_packet_append_payload(&p, (u8_t *)COAP_PAYLOAD, strlen(COAP_PAYLOAD));

    err = send(sock, p.data, p.offset, 0);
    if (err < 0)
    {
        LOG_ERR("Error sending CoAP packet to backend: %d", err);
        close(sock);
        return;
    }
    LOG_INF("CoAP packet sent (%d bytes). Waiting for reply", p.offset);
    // Wait for ack from backend
    err = recv(sock, cbuffer, CBUF_LEN, 0);
    if (err < 0)
    {
        LOG_ERR("Error calling recv(): %d", err);
        close(sock);
        return;
    }

    err = coap_packet_parse(&p, cbuffer, err, NULL, 0);
    if (err < 0)
    {
        LOG_ERR("Error parsing CoAP response: %d", err);
        close(sock);
        return;
    }
    LOG_INF("Response: %02x", coap_header_get_code(&p));
    close(sock);
}
