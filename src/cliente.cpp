#include "socket.h"
#include "utils.h"
using namespace std;

void cliente(string interface) {
    struct connection_t conn;
    struct packet_t pkt;
    if (!conn.connect(interface.data())) {
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
        int res = conn.send_await_packet(PKT_RESTAURA, umsg, &pkt,
            [](struct packet_t *pkt) -> bool {
                uint8_t tipo = pkt->tipo;
                return tipo == PKT_ACK || tipo == PKT_NACK || tipo == PKT_ERRO;
            }
        );
        if (res == TIMEOUT_ERR) {
            printf("[ERRO]: TimeOut - Não foi possivel enviar o pacote\n");
        } else if (res == OK) {
            printf("[MENSSAGEM RECEBIDA]: ");
            print_packet(&pkt);
        } else {
            cout << "[ERROR]: " << strerror(errno) << endl;
        }
    }
    cout << "Saindo do cliente" << endl;
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
    cout << "Inicicalizando cliente (" << interfaces[num] << ")" << endl;
    cliente(interfaces[num].data());
}
