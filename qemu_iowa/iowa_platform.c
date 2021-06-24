/**********************************************
*
* Copyright (c) 2016-2021 IoTerop.
* All rights reserved.
*
**********************************************/

/**********************************************
 *
 * This file implements the IOWA system
 * abstraction functions for Zephyr.
 *
 * This is tailored for the LwM2M Client.
 *
 **********************************************/

// IOWA header
#include "iowa_platform.h"

// Platform specific headers
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <zephyr/types.h>
#include <net/socket.h>
#include <net/net_ip.h>
#include <net/socket_select.h>
#include <syscalls/socket.h>
#include <random/rand32.h>
#include <power/reboot.h>

typedef struct
{
    // We need the iowa context for calling iowa_connection_closed()
    iowa_context_t contextP;

    // a socket to interrupt the select()
    int sysSocket;
} sample_platform_data_t;

// This function creates a self-connected UDP socket which only purpose is to
// interrupt the select
static int prv_createSysSocket()
{
    int sysSock;
    struct sockaddr_in sysAddr, realAddr;
    int addrLen;

    sysSock = zsock_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sysSock == -1)
    {
        fprintf(stderr, "Error in zsock_socket\r\n");
        return -1;
    }

    memset((char *)&sysAddr, 0, sizeof(sysAddr));

    sysAddr.sin_family = AF_INET;
    sysAddr.sin_port = 0;
    sysAddr.sin_addr.s_addr = 0;

    //bind socket to port
    if (zsock_bind(sysSock, (struct sockaddr *)&sysAddr, sizeof(sysAddr)) == -1)
    {
        fprintf(stderr, "Error in zsock_bind\r\n");
        goto error;
    }

    addrLen = sizeof(realAddr);
    if (zsock_getsockname(sysSock, (struct sockaddr *)&realAddr, (socklen_t *)&addrLen) == -1)
    {
        fprintf(stderr, "Error in zsock_getsockname\r\n");
        goto error;
    }

    if (zsock_connect(sysSock, (struct sockaddr *)&realAddr, addrLen) == -1)
    {
        fprintf(stderr, "Error in zsock_connect\r\n");
        goto error;
    }

    return sysSock;

error:
    zsock_close(sysSock);
    return -1;
}

// to set the iowa context once initialized.
void platform_data_set_iowa_context(void *userData,
                                    iowa_context_t contextP)
{
    sample_platform_data_t *dataP;

    dataP = (sample_platform_data_t *)userData;

    dataP->contextP = contextP;
}

void free_platform_data(void *userData)
{
    sample_platform_data_t *dataP;

    dataP = (sample_platform_data_t *)userData;

    if (dataP->sysSocket != -1)
    {
        zsock_close(dataP->sysSocket);
    }

    free(dataP);
}

// This function initializes data used in the system abstraction functions.
// The allocated stucture will be past as userData to the other functions in this file.
void *get_platform_data()
{
    sample_platform_data_t *dataP;

    dataP = (sample_platform_data_t *)malloc(sizeof(sample_platform_data_t));
    if (dataP == NULL)
    {
        fprintf(stderr, "Error in malloc\r\n");
        return NULL;
    }

    dataP->sysSocket = prv_createSysSocket();
    if (dataP->sysSocket == -1)
    {
        fprintf(stderr, "Error in prv_createSysSocket\r\n");
        goto error;
    }

    return (void *)dataP;

error:
    free_platform_data(dataP);
    return NULL;
}

// We bind this function directly to malloc().
void *iowa_system_malloc(size_t size)
{
    return malloc(size);
}

// We bind this function directly to free().
void iowa_system_free(void *pointer)
{
    free(pointer);
}

// We return the number of seconds since startup.
int32_t iowa_system_gettime(void)
{
    return (int32_t)(k_uptime_get() / 1000);
}

void iowa_system_reboot(void *userData)
{
    sys_reboot(SYS_REBOOT_COLD);
}

// Traces are output on stderr.
void iowa_system_trace(const char *format,
                       va_list varArgs)
{
    vfprintf(stderr, format, varArgs);
}

// For POSIX platforms, we use BSD sockets.
// The socket number (incremented by 1, 0 being a valid socket number) is cast as a void *.
static void *prv_sockToPointer(int sock)
{
    void *pointer;

    pointer = NULL;
    sock += 1;

    memcpy(&pointer, &sock, sizeof(int));

    return pointer;
}

static int prv_pointerToSock(void *pointer)
{
    int sock;

    memcpy(&sock, &pointer, sizeof(int));

    return sock - 1;
}

// We consider only UDP connections.
// We open an TCP socket binded to the the remote address.
void *iowa_system_connection_open(iowa_connection_type_t type,
                                  char *hostname,
                                  char *port,
                                  void *userData)
{
    struct zsock_addrinfo hints;
    struct zsock_addrinfo *servinfo = NULL;
    struct zsock_addrinfo *p;
    int s;

    (void)userData;

    // let's consider only UDP connection in this sample
    if (type != IOWA_CONN_DATAGRAM)
    {
        fprintf(stderr, "Only UDP is supported in this sample\r\n");
        return NULL;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    if (0 != zsock_getaddrinfo(hostname, port, &hints, &servinfo) || servinfo == NULL)
    {
        fprintf(stderr, "Error in zsock_getaddrinfo\r\n");
        return NULL;
    }

    // we test the various addresses
    s = -1;
    for (p = servinfo; p != NULL && s == -1; p = p->ai_next)
    {
        fprintf(stderr, "zsock_socket invoked: %d %d %d\r\n", p->ai_family, p->ai_socktype, p->ai_protocol);
        s = zsock_socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (s >= 0)
        {
            fprintf(stderr, "zsock_socket succeeded\r\n");
            if (-1 == zsock_connect(s, p->ai_addr, p->ai_addrlen))
            {
                fprintf(stderr, "zsock_connect failed with code %d\r\n", errno);
                zsock_close(s);
                s = -1;
            }
        }
        else
        {
            fprintf(stderr, "zsock_socket returned %d: %d\r\n", s, errno);
        }
    }

    if (NULL != servinfo)
    {
        zsock_freeaddrinfo(servinfo);
    }

    if (s < 0)
    {
        // failure
        fprintf(stderr, "Could not connect to server\r\n");
        return NULL;
    }

    return prv_sockToPointer(s);
}

// Since the socket is binded, we can use send() directly.
int iowa_system_connection_send(void *connP,
                                uint8_t *buffer,
                                size_t length,
                                void *userData)
{
    int nbSent;
    int sock;

    (void)userData;

    sock = prv_pointerToSock(connP);

    nbSent = zsock_send(sock, buffer, length, 0);

    return nbSent;
}

// Since the socket is binded, it receives datagrams only from the binded address.
int iowa_system_connection_recv(void *connP,
                                uint8_t *buffer,
                                size_t length,
                                void *userData)
{
    int numBytes;
    int sock;

    (void)userData;

    sock = prv_pointerToSock(connP);

    numBytes = zsock_recv(sock, buffer, length, 0);

    return numBytes;
}

// In this function, we use select on the sockets provided by IOWA
// and on the sample_platform_data_t::pipeArray to be able to
// interrupt the select() if required.
int iowa_system_connection_select(void **connArray,
                                  size_t connCount,
                                  int32_t timeout,
                                  void *userData)
{
    struct zsock_timeval tv;
    zsock_fd_set readfds;
    size_t i;
    int result;
    sample_platform_data_t *dataP;
    int maxFd;
    int fd;

    dataP = (sample_platform_data_t *)userData;

    tv.tv_sec = timeout;
    tv.tv_usec = 0;

    ZSOCK_FD_ZERO(&readfds);

    // we add an dummy socket to be able to interrupt the select()
    ZSOCK_FD_SET(dataP->sysSocket, &readfds);
    maxFd = dataP->sysSocket;

    // Then the sockets requested by IOWA
    for (i = 0; i < connCount; i++)
    {
        fd = prv_pointerToSock(connArray[i]);
        ZSOCK_FD_SET(fd, &readfds);
        if (fd > maxFd)
        {
            maxFd = fd;
        }
    }

    result = zsock_select(maxFd + 1, &readfds, NULL, NULL, &tv);

    if (result > 0)
    {
        result = 0;

        if (ZSOCK_FD_ISSET(dataP->sysSocket, &readfds))
        {
            uint8_t buffer[1];

            (void)zsock_recv(dataP->sysSocket, buffer, 1, 0);
        }

        for (i = 0; i < connCount; i++)
        {
            if (ZSOCK_FD_ISSET(prv_pointerToSock(connArray[i]), &readfds))
            {
                int res;
                char unused;

                res = zsock_recv(prv_pointerToSock(connArray[i]), &unused, 1, ZSOCK_MSG_PEEK);
                if (res <= 0)
                {
                    iowa_connection_closed(dataP->contextP, connArray[i]);
                    connArray[i] = NULL;
                }
                else
                {
                    result++;
                }
            }
            else
            {
                connArray[i] = NULL;
            }
        }
    }
    else if (result < 0)
    {
        if (errno == EINTR)
        {
            result = 0;
        }
    }

    return result;
}

void iowa_system_connection_close(void *connP,
                                  void *userData)
{
    int sock;

    (void)userData;

    sock = prv_pointerToSock(connP);

    zsock_close(sock);
}

// To make the call to select() in iowa_system_connection_select() stops,
// we write data to the sysSocket if it's empty.
void iowa_system_connection_interrupt_select(void *userData)
{
    sample_platform_data_t *dataP;
    uint8_t buffer[1];
    int len;

    dataP = (sample_platform_data_t *)userData;

    if (dataP->sysSocket != -1)
    {
        len = zsock_recv(dataP->sysSocket, buffer, 1, ZSOCK_MSG_PEEK | ZSOCK_MSG_DONTWAIT);
        if (len == -1)
        {
            buffer[0] = 'n';
            len = zsock_send(dataP->sysSocket, buffer, 1, 0);
        }
    }
}

int iowa_system_random_vector_generator(uint8_t *randomBuffer,
                                        size_t size,
                                        void *userData)
{
    sys_rand_get(randomBuffer, size);

    return 0;
}

// In these samples, we return hard coded values.
#define SERVER_URI "coaps://192.0.2.2:5684"
#define BOOTSTRAP_SERVER_URI "coaps://192.0.2.2:5784"
#define PSK_IDENTITY "IOWA"
#define PSK_KEY "123456"

iowa_status_t iowa_system_security_data(const uint8_t *peerIdentity,
                                        size_t peerIdentityLen,
                                        iowa_security_operation_t securityOp,
                                        iowa_security_data_t *securityDataP,
                                        void *userDataP)
{
    (void)userDataP;

    switch (securityOp)
    {
    case IOWA_SEC_READ:
        switch (securityDataP->securityMode)
        {
        case IOWA_SEC_PRE_SHARED_KEY:
            if ((peerIdentityLen == strlen(SERVER_URI) && memcmp(peerIdentity, SERVER_URI, peerIdentityLen) == 0) || (peerIdentityLen == strlen(BOOTSTRAP_SERVER_URI) && memcmp(peerIdentity, BOOTSTRAP_SERVER_URI, peerIdentityLen) == 0))
            {
                securityDataP->protocol.pskData.identityLen = strlen(PSK_IDENTITY);
                securityDataP->protocol.pskData.identity = (uint8_t *)PSK_IDENTITY;

                securityDataP->protocol.pskData.privateKeyLen = strlen(PSK_KEY);
                securityDataP->protocol.pskData.privateKey = (uint8_t *)PSK_KEY;

                return IOWA_COAP_NO_ERROR;
            }

            // The peer does not match known Server URIs
            return IOWA_COAP_404_NOT_FOUND;

        default:
            // Implementation for other modes is not done in this example.
            break;
        }
        break;

    case IOWA_SEC_FREE:
        // Nothing to do here
        return IOWA_COAP_NO_ERROR;

    default:
        // Implementation for other operations is not done in this example.
        break;
    }

    return IOWA_COAP_501_NOT_IMPLEMENTED;
}
