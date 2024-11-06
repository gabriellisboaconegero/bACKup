#include "socket.h"
#include "utils.h"
#define SEND_NACK 101
using namespace std;

// Define estados
#define ERR                -1
#define IDLE                0
#define BKP_INIT            1
#define BKP_DATA            2
#define RESTA_INIT          3
#define RESTA_DADOS         4
#define RESTA_FIM           5

int idle_state(struct connection_t *conn, struct packet_t *pkt) {
    vector<uint8_t> umsg;
    string msg = "[IDLE]: Resposta do servidor";
    int res, new_state = IDLE;
    umsg.resize(msg.size());
    copy(msg.begin(), msg.end(), umsg.begin());

    res = conn->recv_packet(0, pkt);
    // Se deu algum erro, não quer dizer que packet é do
    // tipo PKT_ERRO ou PKT_NACK
    if (res < 0) {
        printf("[ERRO]: %s\n", strerror(errno));
        return ERR;
    }

    if (res == PKT_VERIFICA) {
        // Verifica se arquivo existe
        // Se não exitir manda erro
        // if (!file_exist(file_name)) {
        //     // Monta erro corretamente em umsg
        //     conn.send_packet(PKT_ERRO, umsg);
        //     continue;
        // }
        msg += ": VERIFICA";
        umsg.resize(msg.size());
        copy(msg.begin(), msg.end(), umsg.begin());
        // Calcula checksum e coloca em umsg
        if (conn->send_packet(PKT_OK_CKSUM, umsg) < 0)
            return ERR;
    } else if (res == PKT_BACKUP) {
        // Verifica se arquivo ja existe, se existir manda erro
        // if (!file_exist(file_name)) {
        //  // Monta menssagem de erro corretamente em umsg
        //  conn.send_packet(PKT_ERRO, umsg);
        //  continue;
        // }
        // Muda estado para BKP_INIT e manda OK
        msg += ": BACKUP";
        umsg.resize(msg.size());
        copy(msg.begin(), msg.end(), umsg.begin());
        if(conn->send_packet(PKT_OK, umsg) < 0)
            return ERR;
        new_state = BKP_INIT;
    } else if (res == PKT_RESTAURA) {
        // Verifica se arquivo existe e se usuário tem acesso
        // if (!file_exist(file_name) && !file_access(file_name)){
        //  // Monta menssagem de erro corretamente em umsg
        //  conn.send_packet(PKT_ERRO, umsg);
        //  continue;
        // }
        // Muda estado para RESTA_INIT e manda OK_TAM
        // Pega tamanho do arquivo e coloca em umsg
        if (conn->send_packet(PKT_OK_TAM, umsg) < 0)
            return ERR;
        new_state = RESTA_INIT;
    } else {
        // Pacote não reconhecido, envia NACK
        if (conn->send_packet(PKT_NACK, umsg) < 0)
            return ERR;
    }

    return new_state;
}

int bkp_init_state(struct connection_t *conn, struct packet_t *pkt) {
    vector<uint8_t> umsg;
    string msg = "[BKP_INIT]: Resposta do servidor";
    int res, new_state = BKP_INIT;
    umsg.resize(msg.size());
    copy(msg.begin(), msg.end(), umsg.begin());

    res = conn->recv_packet(0, pkt);
    // Se deu algum erro, não quer dizer que packet é do
    // tipo PKT_ERRO ou PKT_NACK
    if (res < 0) {
        printf("[ERRO]: %s\n", strerror(errno));
        return ERR;
    }

    if (res == PKT_TAM) {
        // verifica se tem espaço para criar arquivo, volta para IDLE
        // if (!has_space(file_tam)) {
        //  // Monta menssagem de erro corretamente em umsg
        //  conn.send_packet(PKT_ERRO, umsg);
        //  state = IDLE;
        //  continue;
        // }
        // Cria arquivo e manda OK, muda para BKP_DATA
        if (conn->send_packet(PKT_OK, umsg) < 0)
            return ERR;
        new_state = BKP_DATA;
    } else if (res == PKT_VERIFICA ||
            res == PKT_RESTAURA ||
            res == PKT_BACKUP)
    {
        // Se for alguma das funções principais, voltar para IDLE com erro
        // Monta menssagem de erro de OPERATION_INTERRUPT e coloca em umsg
        if (conn->send_packet(PKT_ERRO, umsg) < 0)
            return ERR;
        new_state = IDLE;
    } else {
        // Não reconheceu pacote, enviar NACK
        if (conn->send_packet(PKT_NACK, umsg) < 0)
            return ERR;
    }

    return new_state;
}

int bkp_data_state(struct connection_t *conn, struct packet_t *pkt) {
    vector<uint8_t> umsg;
    string msg = "[BKP_DATA]: Resposta do servidor";
    int res, new_state = BKP_DATA;
    umsg.resize(msg.size());
    copy(msg.begin(), msg.end(), umsg.begin());

    res = conn->recv_packet(0, pkt);
    // Se deu algum erro, não quer dizer que packet é do
    // tipo PKT_ERRO ou PKT_NACK
    if (res < 0) {
        printf("[ERRO]: %s\n", strerror(errno));
        return ERR;
    }

    if (res == PKT_DADOS) {
        // Recebe dados e manda ACK, coloca os dados no arquivo
        if (conn->send_packet(PKT_ACK, umsg) < 0)
            return ERR;
    } else if (res == PKT_FIM_TX_DADOS) {
        // Finaliza escritura de arquivo e troca para IDLE
        if (conn->send_packet(PKT_ACK, umsg) < 0)
            return ERR;
        new_state = IDLE;
    } else if (res == PKT_VERIFICA ||
            res == PKT_RESTAURA ||
            res == PKT_BACKUP)
    {
        // Se for alguma das funções principais, voltar para IDLE com erro
        // Monta menssagem de erro de OPERATION_INTERRUPT e coloca em umsg
        if (conn->send_packet(PKT_ERRO, umsg) < 0)
            return ERR;
        new_state = IDLE;
    } else {
        // Não reconheceu pacote, enviar NACK
        if (conn->send_packet(PKT_NACK, umsg) < 0)
            return ERR;
    }

    return new_state;
}

void servidor(string interface) {
    struct connection_t conn;
    struct packet_t pkt;
    int state = IDLE;

    // Cria conxão
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
                if (pkt.tipo == PKT_BACKUP || pkt.tipo == PKT_VERIFICA || pkt.tipo == PKT_RESTAURA) {
                    printf("[MENSAGEM RECEBIDA]: "); print_packet(&pkt);
                } else if (pkt.tipo == PKT_ERRO) {
                    printf("[CODIGO ERRO]: Um erro foi detectado no servidor\n");
                } else {
                    printf("[ERRO]: Tipo de menssagem não esperada\n"); print_packet(&pkt);
                }
                break;
            case BKP_INIT:
#ifdef DEBUG
                printf("[STATE]: BKP_INIT\n");
#endif
                state = bkp_init_state(&conn, &pkt);
                if (pkt.tipo == PKT_TAM) {
                    printf("[MENSAGEM RECEBIDA]: "); print_packet(&pkt);
                } else if (pkt.tipo == PKT_ERRO) {
                    printf("[CODIGO ERRO]: Um erro foi detectado no servidor\n");
                } else {
                    printf("[ERRO]: Tipo de menssagem não esperada\n"); print_packet(&pkt);
                }
                break;
            case BKP_DATA:
#ifdef DEBUG
                printf("[STATE]: BKP_DATA\n");
#endif
                state = bkp_data_state(&conn, &pkt);
                if (pkt.tipo == PKT_DADOS || pkt.tipo == PKT_FIM_TX_DADOS) {
                    printf("[MENSAGEM RECEBIDA]: "); print_packet(&pkt);
                } else if (pkt.tipo == PKT_ERRO) {
                    printf("[CODIGO ERRO]: Um erro foi detectado no servidor\n");
                } else {
                    printf("[ERRO]: Tipo de menssagem não esperada\n"); print_packet(&pkt);
                }
                break;
            default:
                printf("[ERRO]: ESTADO(%d) Não deveria chegar aqui\n", state);
                exit(1);
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
