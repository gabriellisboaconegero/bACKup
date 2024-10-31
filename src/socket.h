#pragma once
// Coisas de socket
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <sys/types.h>
// Coisas de socket

#include <stdlib.h>
#include <stdio.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <sysexits.h>

using uchar = unsigned char;

// Constantes
#define MAX_MSG_LEN 100
#define MSG_TO_BIG -1
#define SEND_ERR -2
#define OK 1

struct packet_t {
    int tipo;
    int tam;
    int seq;
    std::vector<uchar> dados;

    // Desserializa os dados de um buffer em um packet.
    // Retorna falso se o packet é inválido.
    // Retorna true, c.c.
    bool deserialize(std::vector<uchar> &buf);
    // Serializa os dados de um packet em um buffer.
    std::vector<uchar> serialize();
};

// Struct responsavel por comunicação na rede, recebe e manda pacotes.
struct connection_t {
    int max_size;
    int socket;
    struct sockaddr_ll addr;

    // Cria raw socket de conexão e inicializa struct.
    // Retorna false se não foi possivel criar socket.
    // Retorn true c.c.
    bool connect(char *, int);

    // Recebe o pacote, fica buscando até achar menssagem do protocolo
    // Retorna false em caso de erro, true c.c.
    bool recv_packet(struct packet_t *);

    // Envia um packet.
    // Retorna MSG_TO_BIG caso a menssagem tenha mais de 63 bytes
    // Retorna SEND_ERR em caso de erro ao fazer send
    // Retorna OK c.c
    int send_packet(uchar, std::string&);
};
