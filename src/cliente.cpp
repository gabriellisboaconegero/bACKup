#include "socket.h"
#include "utils.h"
#define SEND_NACK 101
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
        cout << "Escolha um aquivo para recuperar (q para sair): ";
        cin >> msg;
        if (msg == "q")
            break;
        umsg.resize(msg.size());
        copy(msg.begin(), msg.end(), umsg.begin());
        int res = conn.send_await_packet(PKT_RESTAURA, umsg, &pkt,
            [](struct packet_t *pkt, vector<uint8_t> &buf) -> int {
                // Se houver errro na desserialização do pacote
                if (!pkt->deserialize(buf))
                    return SEND_NACK;

                // Se for um nack continua mandando
                if (pkt->tipo == PKT_NACK)
                    return DONT_ACCEPT;

                // Se pacote não for o esperad
                if (pkt->tipo != PKT_OK_TAM &&
                    pkt->tipo != PKT_NACK   &&
                    pkt->tipo != PKT_ERRO)
                    return SEND_NACK;

                return OK;
            }
        );

        // Limpa umsg
        umsg.clear();
        umsg.resize(20, 0);

        switch (res) {
            case RECV_TIMEOUT:
                printf("[ERRO]: TimeOut - Não foi possivel enviar o pacote\n");
                break;
            case SEND_NACK:
                printf("[ERRO]: Pacote com err\n");
                break;
            case OK:
                printf("[MENSSAGEM RECEBIDA]: ");
                print_packet(&pkt);
                break;
            default:
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
