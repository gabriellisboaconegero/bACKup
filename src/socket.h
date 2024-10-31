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

const int MAX_MSG_LEN = 100;

struct packet_t {
    int tipo;
    int tam;
    int seq;
    std::vector<char> dados;

};

// Struct responsavel por comunicação na rede, recebe e manda pacotes
// Por agora só faz a função de receiver, mas a ideia é que mude o nome
// e vire um connection_t.
struct connection_t {
    int max_size;
    int socket;
    struct packet_t packet;
    struct sockaddr_ll addr;

    bool connect(char *, int);

    // Recebe o pacote
    std::pair<struct packet_t, bool> recv_packet();

    // Envia um packet
    bool send_packet(int, std::string&);

    // Retorna tipo e dados do pacote recebido
    std::pair<int, std::vector<char>> get_packet();

};
int cria_raw_socket(char *interface);
void cliente(char *interface);
