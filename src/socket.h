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
#include <algorithm>

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
#define RECV_TIMEOUT    -4
#define DONT_ACCEPT     -5
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
// Outros tipos de pacotes, apenas para controle. Não são enviados
// na rede.
#define PKT_ERRO            0b11111
#define PKT_TIMEOUT         0b11110
#define PKT_UNKNOW          0b11101

#define NO_FILE_ERRO        1

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
    struct packet_t last_pkt_send;
    struct packet_t last_pkt_recv;
    struct sockaddr_ll addr;

    // Cria raw socket de conexão e inicializa struct.
    // Retorna false se não foi possivel criar socket.
    // Retorn true c.c.
    bool connect(const char *);

    // Espera por um pacote dado um certo intervalo. Se intervalo for 0
    // então não existe timeout.
    // Retorna RECV_ERR (-1) se houver algum erro durante execução.
    // Retorna PKT_TIMEOUT se tiver ocorrido o timeout
    // Retorn o tipo do pacote c.c.
    int recv_packet(int interval, struct packet_t *pkt);

    // Envia um packet. Se pacote for enviado com sucesso copia
    // para last_pkt.
    // Retorna MSG_TO_BIG caso a menssagem tenha mais de 63 bytes
    // Retorna SEND_ERR em caso de erro ao fazer send
    // Retorna OK c.c
    struct packet_t make_packet(int tipo, std::vector<uint8_t> &umsg);

    int send_packet(struct packet_t *pkt);

    void send_erro(uint8_t erro_id);

    void send_ack();

    void send_nack();

    void save_last_recv(struct packet_t *pkt);

    void save_last_send(struct packet_t *pkt);

    void update_seq();
    // Envia um pacote e espera por uma resposta. O pacote recebido é processado
    // pela função process_buf, onde ela decide quais ações tomar com o buffer
    // que foi recebido. É garantido que o buffer vai ter o marcador de inicio
    // do protocolo.
    // Faz retransmissão de dados se tiver timeout, configurar PACKET_TIMEOUT e
    // PACKET_RESTRANSMISSION.
    // Retorna MSG_TO_BIG se menssagem for grande demais.
    // Retorna SEND_ERR em caso de erro ao fazer send.
    // Retorna RECV_TIMEOUT em caso de não recebimento de resposta.
    // Retorna valor da função de parametro c.c.
    // int send_await_packet(uint8_t,
    //                       std::vector<uint8_t> &,
    //                       struct packet_t *,
    //                       std::function<int(struct packet_t *, std::vector<uint8_t> &)>);
};
// =========== Structs ===========
