#include "socket.h"
using namespace std;

void cliente(char *interface) {
    struct connection_t conn;
    struct packet_t pkt;
    if (!conn.connect(interface)) {
        cout << "[ERRO]: Erro ao criar conexão com interface (" << interface << ")" << endl;
        cout << "[ERRO]: " << strerror(errno) << endl;
        exit(1);
    }

    string msg;
    vector<uint8_t> umsg;
    while(1) {
        cout << "Escreva uma menssagem (q para sair): ";
        cin >> msg;
        if (msg == "q")
            break;
        umsg.resize(msg.size());
        copy(msg.begin(), msg.end(), umsg.begin());
        int res = conn.send_await_packet(1, umsg, &pkt);
        if (res == TIMEOUT_ERR) {
            printf("[ERRO]: TimeOut - Não foi possivel enviar o pacote\n");
        } else if (res == OK) {
            printf("MENSSAGEM RECEBIDA (tipo: %d, seq: %d, size: %d): ",
                    pkt.tipo, pkt.seq, pkt.tam);
            for (int i = 0; i < pkt.tam; i++)
                cout << pkt.dados[i];
            cout << endl;
        } else {
            cout << "[ERROR]: " << strerror(errno) << endl;
        }
    }
    cout << "Saindo do cliente" << endl;
}

int main(int argc, char **argv) {
    char *interface = "dm-675ad8ccf0f1";
    cout << "Inicicalizando cliente (" << interface << ")" << endl;
    cliente(interface);
}
