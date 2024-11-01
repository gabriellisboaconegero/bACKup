#include "socket.h"
#include "utils.h"
using namespace std;

void servidor(string interface) {
    struct connection_t conn;
    struct packet_t packet;
    if (!conn.connect(interface.data())) {
        cout << "[ERRO]: Erro ao criar conexão com interface (" << interface << ")" << endl;
        cout << "[ERRO]: " << strerror(errno) << endl;
        exit(1);
    }

    while(1) {
        if (!conn.recv_packet(&packet)) {
            cout << "[ERRO]: Erro ao receber pacote do socket" << endl;
            cout << "[ERRO]: " << strerror(errno) << endl;
            continue;
        }
        string msg = "Resposta do servidor";
        vector<uint8_t> umsg;
        umsg.resize(msg.size());
        copy(msg.begin(), msg.end(), umsg.begin());
        conn.send_packet(PKT_ACK, umsg);

        printf("MENSSAGEM RECEBIDA (tipo: %d, seq: %d, size: %d): ",
                packet.tipo, packet.seq, packet.tam);
        for (int i = 0; i < packet.tam; i++)
            cout << packet.dados[i];
        cout << endl;
    }
    cout << "Saindo do servidor" << endl;
}

int main(int argc, char **argv) {
    int num;
    cout << "Escolha a interface de rede para usar: " << endl;
    vector<string> interfaces;
    if (get_interfaces(interfaces) < 0) {
        cout << "[ERRO]: Erro ao obter as interfaces de rede" << endl;
        exit(1);
    }
    for (int i = 0; i < int(interfaces.size()); i++) {
        printf("\t%d - %s\n", i, interfaces[i].data());
    }
    cout << ">> ";
    cin >> num;
    num %= int(interfaces.size());
    cout << "Inicicalizando servidor (" << interfaces[num] << ")" << endl;
    servidor(interfaces[num].data());
}
