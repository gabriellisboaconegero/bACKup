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

// =========== Constantes ===========
#define MAX_MSG_LEN 100

// Function return codes
#define MSG_TO_BIG -1
#define SEND_ERR -2
#define OK 1

// Códigos do protocolo
#define PKT_ACK             0b00000
#define PKT_NACK            0b00001
#define PKT_OK              0b00010
#define PKT_BACKUP          0b00100
#define PKT_RESTAURA        0b00101
#define PKT_VERIFICA        0b00110
#define PKT_OK_CKSUM        0b01101
#define PKT_OK_TAM          0b01110
#define PKT_TAM             0b01111
#define PKT_DADOS           0b10000
#define PKT_FIM_TX_DADOS    0b10001
#define PKT_ERRO            0b11111

// Macros de tamanho de cada campo do protocolo em bits
#define MI_SIZE             8
#define TAM_SIZE            6
#define SEQ_SIZE            5
#define TIPO_SIZE           5
#define CRC_SIZE            8
// Tamanho minimo e maximo de um packet e dados (em bytes)
#define PACKET_MIN_SIZE         ((MI_SIZE + TAM_SIZE + SEQ_SIZE + TIPO_SIZE + CRC_SIZE)/8)
#define PACKET_MAX_DADOS_SIZE   ((1<<TAM_SIZE)-1)
#define PACKET_MAX_SIZE         (PACKET_MIN_SIZE + PACKET_MAX_DADOS_SIZE)
// =========== Constantes ===========

// =========== Structs ===========
struct packet_t {
    uint8_t tipo;
    uint8_t tam;
    uint8_t seq;
    std::vector<uint8_t> dados;

    // Desserializa os dados de um buffer em um packet.
    // Retorna falso se o packet é inválido.
    // Retorna true, c.c.
    bool deserialize(std::vector<uint8_t> &buf);
    // Serializa os dados de um packet em um buffer.
    std::vector<uint8_t> serialize();
};

// Struct responsavel por comunicação na rede, recebe e manda pacotes.
struct connection_t {
    int socket;
    uint8_t seq;
    struct sockaddr_ll addr;

    // Cria raw socket de conexão e inicializa struct.
    // Retorna false se não foi possivel criar socket.
    // Retorn true c.c.
    bool connect(char *);

    // Recebe o pacote, fica buscando até achar menssagem do protocolo
    // Retorna false em caso de erro, true c.c.
    bool recv_packet(struct packet_t *);

    // Envia um packet.
    // Retorna MSG_TO_BIG caso a menssagem tenha mais de 63 bytes
    // Retorna SEND_ERR em caso de erro ao fazer send
    // Retorna OK c.c
    int send_packet(uint8_t, std::string&);
};
// =========== Structs ===========
