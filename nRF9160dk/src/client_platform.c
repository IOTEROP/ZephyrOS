/**********************************************
*
* Copyright (c) 2016-2021 IoTerop.
* All rights reserved.
*
**********************************************/

/**********************************************
 *
 * This file implements the IOWA system
 * abstraction functions for Linux and Windows.
 *
 * This is tailored for the LwM2M Client on nrf9160DK
 * TODO: TCP support to add
 *
 **********************************************/

// IOWA header
#include "iowa_platform.h"

#include <zephyr.h>
#include <stdio.h>

#include <modem/lte_lc.h>
#include <net/socket.h>
#include <errno.h>

#include <modem/modem_key_mgmt.h>
#include <net/tls_credentials.h>

typedef struct
{
    // a mutex for iowa_system_mutex_* functions
    struct k_mutex mutex;

    // a socket to interrupt the select()
    int sysSocket;
} sample_platform_data_t;

// This function creates a self-connected UDP socket which only purpose
// is to interrupt the select
static int prv_createSysSocket()
{
    int sysSock;
    struct sockaddr_in sysAddr;

    sysSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sysSock  < 0)
    {
       printk("prv_createSysSocket: Failed to create TCP/UDP socket: %d\n", errno);
       return -1;
    }

    memset((char *)& sysAddr, 0, sizeof(sysAddr));

    sysAddr.sin_family = AF_INET;
    sysAddr.sin_port = htons(60000);
    sysAddr.sin_addr.s_addr =  htonl((127 << 24) | (0 << 16) | (0 << 8) | 1);
    
    //bind socket to port
    if (bind(sysSock, (struct sockaddr *) &sysAddr, sizeof(sysAddr)) == -1)
    {
        printk("Bind error\n");
        goto error;
    }

    return sysSock;

error:
    close(sysSock);

    return -1;
}

void free_platform_data(void *userData)
{
    sample_platform_data_t *dataP;

    dataP = (sample_platform_data_t *)userData;

    if (dataP->sysSocket != -1)
    {
        close(dataP->sysSocket);
    }
    k_free(dataP);
}

// This function initializes data used in the system abstraction functions.
// The allocated stucture will be past as userData to the other functions in this file.
void * get_platform_data()
{
    sample_platform_data_t *dataP;
    dataP = (sample_platform_data_t *)k_malloc(sizeof(sample_platform_data_t));
    if (dataP == NULL)
    {
        return NULL;
    }

    if (k_mutex_init(&(dataP->mutex)) != 0)
    {
        goto error;
    }

    dataP->sysSocket = prv_createSysSocket();
    if (dataP->sysSocket == -1)
    {
        goto error;
    }

    return (void *)dataP;

error:
    free_platform_data(dataP);
    return NULL;
}

// We bind this function directly to malloc().
void * iowa_system_malloc(size_t size)
{
    return k_malloc(size);
}

// We bind this function directly to free().
void iowa_system_free(void *pointer)
{
    k_free(pointer);
}

// We return the number of seconds since Epoch.
int32_t iowa_system_gettime(void)
{
    return (int32_t) (k_uptime_get() / 1000);
}

// We fake a reboot by exiting the application.
void iowa_system_reboot(void *userData)
{
    (void)userData;

    fprintf(stdout, "\n\tFaking a reboot.\r\n\n");
    exit(0);
}

// Traces are output on stderr.
void iowa_system_trace(const char *format,
                       va_list varArgs)
{
    vprintf(format, varArgs);    
}

// For POSIX platforms, we use BSD sockets.
// The socket number (incremented by 1, 0 being a valid socket number) is cast as a void *.
static void * prv_sockToPointer(int sock)
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

#if defined(CONFIG_MBEDTLS)
/**@brief Add socket credentials according to security tag */
static int socket_sectag_set(int fd, int sec_tag)
{
    int err;
    int verify;
    sec_tag_t sec_tag_list[] = {sec_tag};

    enum
    {
        NONE = 0,
        OPTIONAL = 1,
        REQUIRED = 2,
    };

    verify = NONE;

    err = setsockopt(fd, SOL_TLS, TLS_PEER_VERIFY, &verify, sizeof(verify));
    if (err)
    {
        printk("Failed to setup peer verification, errno %d\n", errno);
        return -errno;
    }

    printk("Setting up TLS credentials, tag %d\n", sec_tag);
    err = setsockopt(fd, SOL_TLS, TLS_SEC_TAG_LIST, sec_tag_list,
                     sizeof(sec_tag_t) * ARRAY_SIZE(sec_tag_list));
    if (err)
    {
        printk("Failed to setup socket security tag, errno %d\n", errno);
        return -errno;
    }

    return 0;
}

#endif //(CONFIG_MBEDTLS)


// We consider only UDP connections.
// We open an UDP socket binded to the the remote address.
void * iowa_system_connection_open(iowa_connection_type_t type,
                                   char *hostname,
                                   char *port,
                                   void *userData)
{
#ifdef _WIN32
    struct WSAData wd;
#endif
    struct addrinfo hints;
    struct addrinfo *servinfo = NULL;
    struct addrinfo *p;
    int s;
#if defined(CONFIG_MBEDTLS)
    int err;
#endif

    (void)userData;

    // let's consider only UDP connection in this sample
    if (type != IOWA_CONN_DATAGRAM)
    {
        return NULL;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    if (0 != getaddrinfo(hostname, port, &hints, &servinfo)
        || servinfo == NULL)
    {
        return NULL;
    }

    // we test the various addresses
    s = -1;
    for (p = servinfo; p != NULL && s == -1; p = p->ai_next)
    {
#if defined(CONFIG_MBEDTLS)
      p->ai_protocol = IPPROTO_DTLS_1_2;
#else
      p->ai_protocol = IPPROTO_UDP;
#endif
        s = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (s >= 0)
        {
#if defined(CONFIG_MBEDTLS)
          err = socket_sectag_set(s, CONFIG_IOWA_BOARD_TLS_TAG);
          if (err)
          {
            close(s);
            s = -1;       
          }
          else 
#endif
            if (-1 == connect(s, p->ai_addr, p->ai_addrlen))
            {
                close(s);
                s = -1;
            }
        }
    }

    if (NULL != servinfo)
    {
        freeaddrinfo(servinfo);
    }

    if (s < 0)
    {
        // failure
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

    nbSent = send(sock, buffer, length, 0);

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

    numBytes = recv(sock, buffer, length, 0);

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
    struct timeval tv;
    fd_set readfds;
    size_t i;
    int result;
    sample_platform_data_t *dataP;
    int maxFd;
    int fd;

    dataP = (sample_platform_data_t *)userData;

    tv.tv_sec = timeout;
    tv.tv_usec = 0;

    FD_ZERO(&readfds);

    // we add an dummy socket to be able to interrupt the select()
    FD_SET(dataP->sysSocket, &readfds);
    maxFd = dataP->sysSocket;

    // Then the sockets requested by IOWA
    for (i = 0; i < connCount; i++)
    {
        fd = prv_pointerToSock(connArray[i]);
        FD_SET(fd, &readfds);
        if (fd > maxFd)
        {
            maxFd = fd;
        }
    }

    result = select(maxFd + 1, &readfds, NULL, NULL, &tv);

    if (result > 0)
    {
        for (i = 0; i < connCount; i++)
        {
            if (!FD_ISSET(prv_pointerToSock(connArray[i]), &readfds))
            {
                connArray[i] = NULL;
            }
        }
        if (FD_ISSET(dataP->sysSocket, &readfds))
        {
            uint8_t buffer[1];
            struct sockaddr peerAddr;
            socklen_t peerAddrLength;

            result--;
            (void)recvfrom(dataP->sysSocket, buffer, 1, 0, &peerAddr, &peerAddrLength);
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

    close(sock);
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
        int sysSock;
        struct sockaddr_in sysAddr, realAddr;

        sysSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (sysSock == -1)
        {
            printk("-Failed to create socket: %d\n", errno);
            return;
        }

        memset((char *)& sysAddr, 0, sizeof(sysAddr));

        sysAddr.sin_family = AF_INET;
        sysAddr.sin_port = htons(60000);
        sysAddr.sin_addr.s_addr = htonl((127 << 24) | (0 << 16) | (0 << 8) | 1);

        //bind socket to port
        if (connect(sysSock, (struct sockaddr *) &sysAddr, sizeof(sysAddr)) == -1)
        {
            printk("- Failed to connect socket: %d\n", errno);
            return;
        }

        buffer[0] = 'n';
        send(sysSock, buffer, 1, 0);

        (void)close(sysSock);
    }
}

void iowa_system_mutex_lock(void *userData)
{
    sample_platform_data_t *dataP;

    dataP = (sample_platform_data_t *)userData;

    k_mutex_lock(&(dataP->mutex), K_NO_WAIT);

}

void iowa_system_mutex_unlock(void *userData)
{
    sample_platform_data_t *dataP;

    dataP = (sample_platform_data_t *)userData;

    k_mutex_unlock(&(dataP->mutex));
}

// This is not a proper way to generate a random vector, it only serves as an example here
int iowa_system_random_vector_generator(uint8_t *randomBuffer,
                                        size_t size,
                                        void *userData)
{
    size_t i;

    (void)userData;

    for (i = 0; i < size; i++)
    {
        randomBuffer[i] = rand() % 256;
    }

    return 0;
}
