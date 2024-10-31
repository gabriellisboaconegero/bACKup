#include "socket.h"
using namespace std;

void servidor(char *interface) {
    struct connection_t conn;
    struct packet_t packet;
    if (!conn.connect(interface, MAX_MSG_LEN)) {
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

        printf("MENSSAGEM RECEBIDA (tipo: %d, size: %ld): ", packet.tipo, packet.dados.size());
        for (int i = 0; i < int(packet.dados.size()); i++)
            cout << packet.dados[i];
        cout << endl;
    }
    cout << "Saindo do servidor" << endl;
}

int main(int argc, char **argv) {
    char *interface = "lo";
    cout << "Inicicalizando servidor (" << interface << ")" << endl;
    servidor(interface);
}
