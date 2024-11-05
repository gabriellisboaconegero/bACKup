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
#include <sys/time.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <sysexits.h>
#include <functional>

// =========== Constantes ===========
// Marcador de Inicio
#define PACKET_MI 0x7e // 0b01111110

// Tamnho maximo de respo
#define MAX_MSG_LEN 100

// Polinômio gerador usando no crc8
#define GENERATOR_POLY 0x1D

// Configuração de retransmissão
#define PACKET_TIMEOUT_INTERVAL         250 // Tempo até retransmissão (em milisegundos)
#define PACKET_RETRASMISSION_ROUNDS     10    // Quantidade de retransmissões máximas

// Function return codes
#define MSG_TO_BIG      -1
#define SEND_ERR        -2
#define RECV_ERR        -3
#define TIMEOUT_ERR     -4
#define RECV_TIMEOUT    -5
#define OK               1

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
    bool connect(const char *);

    // Recebe um pacote esperando por determinado intervalo. Coloca o pacote lido em pkt
    // se ele for de acordo com o protocolo. Se intervalo for 0 fica esperando sem timeout.
    // Retorna RECV_TIMEOUT se der timeout
    // Retorna RECV_ERR se der erro
    // Retorna OK c.c
    int recv_packet(int interval, struct packet_t *pkt,
                          std::function<bool(struct packet_t *)> is_espected_packet);

    // Envia um packet.
    // Retorna MSG_TO_BIG caso a menssagem tenha mais de 63 bytes
    // Retorna SEND_ERR em caso de erro ao fazer send
    // Retorna OK c.c
    int send_packet(uint8_t, std::vector<uint8_t> &);

    // Envia um pacote e espera por uma resposta. O pacote de resposta deve ser aceito
    // pela função is_espected_packet, passada como parametro para a função.
    //      is_espected_packet: Retorna true se o packet é esperado
    //                          Retorna falso c.c.
    // Faz retransmissão de dados se tiver timeout, configurar PACKET_TIMEOUT e
    // PACKET_RESTRANSMISSION.
    // Retorna MSG_TO_BIG se menssagem for grande demais.
    // Retorna SEND_ERR em caso de erro ao fazer send.
    // Retorna TIMEOUT_ERR em caso de não recebimento de resposta.
    // Retorna OK c.c.
    int send_await_packet(uint8_t,
                          std::vector<uint8_t> &,
                          struct packet_t *,
                          std::function<bool(struct packet_t *)>);
};
// =========== Structs ===========
