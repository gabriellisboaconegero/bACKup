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

// Struct responsavel por comunicação na rede, recebe e manda pacotes
// Por agora só faz a função de receiver, mas a ideia é que mude o nome
// e vire um connection_t.
struct connection_t {
    std::vector<char> packet;
    int max_size;
    int socket;
    int err;
    struct sockaddr_ll addr;

    connection_t (int, int);
    // Recebe o pacote
    bool recv();

    // Retorna uma string do erro se tiver
    std::string erro2string();

    // Valida o pacote com MI e loopback.
    // Verifica se o pacote é valido de acordo com protocolo
    // Ou seja:
    //  MI é 0b01111110 (0x7e)
    //  Se não é pacote de loopback
    bool valida_packet();
};
int cria_raw_socket(char *interface);
void cliente(char *interface);
