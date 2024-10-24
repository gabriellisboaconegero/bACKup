#include "socket.h"
using namespace std;

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

// ===================== Reciever =====================
connection_t::connection_t(int sock, int max_msg_size) {
    socket = sock;
    max_size = max_msg_size;
    err = 0;
}

bool connection_t::recv() {
    socklen_t addr_len = sizeof(addr);
    packet.resize(max_size, 0);
    auto msg_len = recvfrom(socket, packet.data(), packet.size(), 0, (struct sockaddr*)&addr, &addr_len);
    if (msg_len < 0){
        err = errno;
        return false;
    }
    packet.resize(msg_len);
    return true;
}

string connection_t::erro2string() {
    return strerror(err);
}

bool connection_t::valida_packet() {
    // Verifica se é um outgoing packet.
    // Evita receber os pacotes duplicados no loopback
    // Vide:
    //  - https://github.com/the-tcpdump-group/libpcap/blob/1c1d968bb3f3d163fa7f6d6802842a127304dd22/pcap-linux.c#L1352
    //  - https://stackoverflow.com/a/17235405
    //  - man recvfrom
    //  - man packet
    if (addr.sll_pkttype == PACKET_OUTGOING)
        return false;
    // TODO: Trocar par marcador de inicio
    if (packet[0] != 'a')
        return false;
    return true;
}
// ===================== Reciever =====================

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
