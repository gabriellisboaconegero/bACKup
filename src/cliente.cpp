#include "socket.h"
#include "utils.h"
using namespace std;

// Define estados
#define IDLE                0
#define VERIFICA            1
#define BKP_INIT            2
#define BKP_TAM             3
#define BKP_DATA            4
#define BKP_FIM             5
#define RESTA_INIT          6
#define RESTA_TAM           7
#define RESTA_DADOS         8

void cliente(string interface) {
    struct connection_t conn;
    struct packet_t pkt;
    string msg;
    vector<uint8_t> umsg;
    int state = IDLE, opt;

    if (!conn.connect(interface.data())) {
        cout << "[ERRO]: Erro ao criar conexão com interface (" << interface << ")" << endl;
        cout << "[ERRO]: " << strerror(errno) << endl;
        exit(1);
    }

    while(1) {
        switch (state) {
            case IDLE:
                // Apresenta opções de escolha. BACKUP, VERIFICA e RESTAURA
                cout << "Escolha um aquivo para recuperar (q para sair): ";
                cin >> msg;
                if (msg == "q")
                    break;
                umsg.resize(msg.size());
                copy(msg.begin(), msg.end(), umsg.begin());
                opt = PKT_BACKUP;
                if (opt == PKT_BACKUP) {

                }
                // else if (opt == PKT_VERIFICA) {

                // } else {

                // }
                break;
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
