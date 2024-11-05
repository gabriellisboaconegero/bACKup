#include "socket.h"
#include "utils.h"
#define SEND_NACK 101
using namespace std;

void servidor(string interface) {
    struct connection_t conn;
    struct packet_t pkt;
    vector<uint8_t> umsg;
    string msg = "Resposta do servidor";
    if (!conn.connect(interface.data())) {
        cout << "[ERRO]: Erro ao criar conexão com interface (" << interface << ")" << endl;
        cout << "[ERRO]: " << strerror(errno) << endl;
        exit(1);
    }

    while(1) {
        int res = conn.recv_packet(0, &pkt,
            [](struct packet_t *pkt, vector<uint8_t> &buf) -> int {
                // Se houver errro na desserialização do pacote
                if (!pkt->deserialize(buf))
                    return SEND_NACK;

                // Se pacote não for o esperado
                if (pkt->tipo != PKT_BACKUP   &&
                    pkt->tipo != PKT_RESTAURA &&
                    pkt->tipo != PKT_VERIFICA)
                    return SEND_NACK;

                return OK;
            });
        // Limpa umsg
        umsg.clear();
        umsg.resize(20, 0);
        switch (res) {
            case SEND_NACK:
                printf("[ERRO]: Pacote com err, enviando NACK\n");
                conn.send_packet(PKT_NACK, umsg);
                break;
            case OK:
                printf("[MENSSAGEM RECEBIDA]: ");
                print_packet(&pkt);
                cout << "Respondendo cliente" << endl;
                umsg.resize(msg.size());
                copy(msg.begin(), msg.end(), umsg.begin());
                conn.send_packet(PKT_OK_TAM, umsg);
                break;
            default:
                cout << "[ERRO]: Erro ao receber pacote do socket" << endl;
                cout << "[ERRO]: " << strerror(errno) << endl;
        }
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
