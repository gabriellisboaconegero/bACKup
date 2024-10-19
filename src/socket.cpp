#include <arpa/inet.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <stdlib.h>
#include <stdio.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>
#include <ifaddrs.h>
#include <sysexits.h>
#include <sys/socket.h>
#include <sys/types.h>
using namespace std;
const int MAX_MSG_LEN = 100;

// ler os seguintes manuais para entender melhor o que essa função faz
//      man packet
//      man socket
//      man setsockopt
int cria_raw_socket(char* nome_interface_rede) {
    // Cria arquivo para o socket sem qualquer protocolo
    int soquete = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (soquete == -1) {
        fprintf(stderr, "Erro ao criar socket: Verifique se você é root!\n");
        exit(-1);
    }
 
    int ifindex = if_nametoindex(nome_interface_rede);
 
    struct sockaddr_ll endereco = {0};
    endereco.sll_family = AF_PACKET;
    endereco.sll_protocol = htons(ETH_P_ALL);
    endereco.sll_ifindex = ifindex;
    // Inicializa socket
    if (bind(soquete, (struct sockaddr*) &endereco, sizeof(endereco)) == -1) {
        fprintf(stderr, "Erro ao fazer bind no socket\n");
        exit(-1);
    }
 
    struct packet_mreq mr = {0};
    mr.mr_ifindex = ifindex;
    mr.mr_type = PACKET_MR_PROMISC;
    // Não joga fora o que identifica como lixo: Modo promíscuo
    if (setsockopt(soquete, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr)) == -1) {
        fprintf(stderr, "Erro ao fazer setsockopt: "
            "Verifique se a interface de rede foi especificada corretamente.\n");
        exit(-1);
    }
 
    return soquete;
}

void cliente(char *interface) {
    int socket = cria_raw_socket(interface);
    if (socket == -1){
        cout << "[ERRO]: Algo de errado com socket" << endl;
        exit(1);
    }

    string msg;
    while(1) {
        cout << "Escreva uma menssagem (q para sair): ";
        cin >> msg;
        if (msg == "q")
            break;
        if (msg.size() <= 14){
            cout << "[WARNING]: Menssagem com menos de 14 caracteres" << endl;
        }
        if (send(socket, msg.c_str(), msg.size(), 0) < 0){
            cout << "[ERROR]: " << strerror(errno) << endl;
        }
    }
    cout << "Saindo do cliente" << endl;
}

void servidor(char *interface) {
    int socket = cria_raw_socket(interface);
    if (socket == -1){
        cout << "[ERRO]: Algo de errado com socket" << endl;
        exit(1);
    }

    char *buf = new char [MAX_MSG_LEN+1]();
    int msg_size;
    while(1) {
        if ((msg_size = recv(socket, buf, MAX_MSG_LEN, 0)) < 0){
            cout << "[ERROR]: " << strerror(errno) << endl;
        }else{
            if (buf[0] != 'a')
                continue;
            cout << "MENSSAGEM RECEBIDA (size: " << msg_size << "): ";
            for (int i = 0; i < msg_size; i++)
                cout << buf[i];
            cout << endl;
        }
    }
    cout << "Saindo do servidor" << endl;

    delete[] buf;
}
