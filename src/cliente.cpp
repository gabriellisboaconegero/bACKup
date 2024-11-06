#include "socket.h"
#include "utils.h"
using namespace std;

// Define estados
#define ERR                -1
#define IDLE                0
#define VERIFICA            1
#define BKP_INIT            2
#define BKP_TAM             3
#define BKP_DATA            4
#define BKP_FIM             5
#define RESTA_INIT          6
#define RESTA_TAM           7
#define RESTA_DADOS         8

int idle_state(struct connection_t *conn, struct packet_t *pkt) {
    vector<uint8_t> umsg;
    string msg = "[IDLE]: MSG cliente";
    int res, new_state = IDLE;
    umsg.resize(msg.size());
    copy(msg.begin(), msg.end(), umsg.begin());
    int opt;
    // Apresenta opções de escolha. BACKUP, VERIFICA e RESTAURA
    int opts[3] = {
        PKT_VERIFICA,
        PKT_BACKUP,
        PKT_RESTAURA,
    };
    printf("Escolha uma das opções:\n\t0 - VERIFICA\n\t1 - BACKUP\n\t2 - RESTAURA\n\toutro para sair\n>> ");
    cin >> opt;
    if (opt > 3 && opt < 0)
        exit(1);

    if (opts[opt] == PKT_VERIFICA) {
        // Pede o nome de um arquivo para recuperar e coloca em umsg
        if (conn->send_packet(PKT_VERIFICA, umsg) < 0)
            return ERR;
        new_state = VERIFICA;
    } else if (opts[opt] == PKT_BACKUP) {
        // Pede o nome de um arquivo para fazer backup e coloca em umsg
        if (conn->send_packet(PKT_BACKUP, umsg) < 0)
            return ERR;
        new_state = BKP_INIT;
    } else if (opts[opt] == PKT_RESTAURA) {
        // Pede o nome de um arquivo para restaurar e coloca em umsg
        if (conn->send_packet(PKT_RESTAURA, umsg) < 0)
            return ERR;
        new_state = RESTA_INIT;
    }
    
    return new_state;
}

int verifica_state(struct connection_t *conn, struct packet_t *pkt) {
    vector<uint8_t> umsg;
    int res, round, interval = PACKET_TIMEOUT_INTERVAL;
    struct packet_t r_pkt;

    // Copia ultima menssagem para umsg, para ser enviada em casa de timeout
    umsg.resize(conn->last_pkt.dados.size());
    copy(conn->last_pkt.dados.begin(), conn->last_pkt.dados.end(), umsg.begin());

    // Tenta fazer o timeout, somente tem timeout em NACKS
    // Qualquer outra menssagem vai sair.
    for (round = 0; round < PACKET_RETRASMISSION_ROUNDS; round++) {
#ifdef DEBUG
        printf("[DEBUG]: Tentativa (%d\\%d)\n", round+1, PACKET_RETRASMISSION_ROUNDS);
#endif
        res = conn->recv_packet(interval, pkt);
        // Se deu algum erro, não quer dizer que packet é do
        // tipo PKT_ERRO ou PKT_NACK
        if (res < 0) {
            printf("[ERRO]: %s\n", strerror(errno));
            return ERR;
        }
        if (res != PKT_TIMEOUT && res != PKT_NACK)
            break;

        if (conn->send_packet(conn->last_pkt.tipo, umsg) < 0)
            return ERR;

#ifdef DEBUG
        printf("[DEBUG]: Menssagem enviada com sucesso\n");
        printf("[DEBUG]: "); print_packet(&conn->last_pkt);
#endif
    }
    // Se alcançou o maximo de retransmissões, marca que teve timeout
    if (round == PACKET_RETRASMISSION_ROUNDS) {
        pkt->tipo = PKT_TIMEOUT;
    }

    // Sempre volta para o idle
    return IDLE;
}

void cliente(string interface) {
    struct connection_t conn;
    struct packet_t pkt;
    string msg;
    vector<uint8_t> umsg;
    int state = IDLE;

    if (!conn.connect(interface.data())) {
        cout << "[ERRO]: Erro ao criar conexão com interface (" << interface << ")" << endl;
        cout << "[ERRO]: " << strerror(errno) << endl;
        exit(1);
    }

    while(1) {
        switch (state) {
            case IDLE:
#ifdef DEBUG
                printf("[STATE]: IDLE\n");
#endif
                state = idle_state(&conn, &pkt);
                break;
            case VERIFICA:
#ifdef DEBUG
                printf("[STATE]: VERIFICA\n");
#endif
                state = verifica_state(&conn, &pkt);
                if (pkt.tipo == PKT_TIMEOUT) {
                    printf("[ERRO]: Timeout ao fazer verifica, voltando para idle\n");
                } else if (pkt.tipo == PKT_OK_CKSUM) {
                    printf("[MENSAGEM RECEBIDA]: "); print_packet(&pkt);
                } else if (pkt.tipo == PKT_ERRO) {
                    printf("[CODIGO ERRO]: Um erro foi detectado no servidor\n");
                } else {
                    printf("[ERRO]: Tipo de menssagem não esperada\n");
                }
                break;
            default:
                printf("[ERRO]: ESTADO(%d) Não deveria chegar aqui\n", state);
                exit(1);
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
