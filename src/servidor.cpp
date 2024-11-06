#include "socket.h"
#include "utils.h"
#define SEND_NACK 101
using namespace std;

// Define estados
#define IDLE                0
#define BKP_INIT            1
#define BKP_DATA            2
#define RESTA_INIT          3
#define RESTA_DADOS         4
#define RESTA_FIM           5

void servidor(string interface) {
    struct connection_t conn;
    struct packet_t pkt;
    vector<uint8_t> umsg;
    string msg = "Resposta do servidor";
    int state = IDLE, res;

    umsg.resize(msg.size());
    copy(msg.begin(), msg.end(), umsg.begin());

    // Cria conxão
    if (!conn.connect(interface.data())) {
        cout << "[ERRO]: Erro ao criar conexão com interface (" << interface << ")" << endl;
        cout << "[ERRO]: " << strerror(errno) << endl;
        exit(1);
    }

    while(1) {
        switch (state) {
            case IDLE:
                res = conn.recv_packet(0, &pkt);
                if (res < 0) {
                    printf("[ERRO]: %s\n", strerror(errno));
                    continue;
                }
                if (res == PKT_VERIFICA) {
                    // Verifica se arquivo existe
                    // Se não exitir manda erro
                    // if (!file_exist(file_name)) {
                    //     // Monta erro corretamente em umsg
                    //     conn.send_packet(PKT_ERRO, umsg);
                    //     continue;
                    // }
                    // Calcula checksum e coloca em umsg
                    conn.send_packet(PKT_OK_CKSUM, umsg);
                } else if (res == PKT_BACKUP) {
                    // Verifica se arquivo ja existe, se existir manda erro
                    // if (!file_exist(file_name)) {
                    //  // Monta menssagem de erro corretamente em umsg
                    //  conn.send_packet(PKT_ERRO, umsg);
                    //  continue;
                    // }
                    // Muda estado para BKP_INIT e manda OK
                    state = BKP_INIT;
                    conn.send_packet(PKT_OK, umsg);
                } else if (res == PKT_RESTAURA) {
                    // Verifica se arquivo existe e se usuário tem acesso
                    // if (!file_exist(file_name) && !file_access(file_name)){
                    //  // Monta menssagem de erro corretamente em umsg
                    //  conn.send_packet(PKT_ERRO, umsg);
                    //  continue;
                    // }
                    // Muda estado para RESTA_INIT e manda OK_TAM
                    // Pega tamanho do arquivo e coloca em umsg
                    conn.send_packet(PKT_OK_TAM, umsg);
                } else {
                    // Pacote não reconhecido, envia NACK
                    conn.send_packet(PKT_NACK, umsg);
                }
                break;
            case BKP_INIT:
                res = conn.recv_packet(0, &pkt);
                // Verifica se teve erro ao receber, não quer dizer que tipo da 
                // menssagem é PKT_ERRO.
                if (res < 0) {
                    printf("[ERRO]: %s\n", strerror(errno));
                    continue;
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
                    state = BKP_DATA;
                    conn.send_packet(PKT_OK, umsg);
                } else if (res == PKT_VERIFICA ||
                           res == PKT_RESTAURA ||
                           res == PKT_BACKUP)
                {
                    // Se for alguma das funções principais, voltar para IDLE com erro
                    // Monta menssagem de erro de OPERATION_INTERRUPT e coloca em umsg
                    state = IDLE;
                    conn.send_packet(PKT_ERRO, umsg);
                } else {
                    // Não reconheceu pacote, enviar NACK
                    conn.send_packet(PKT_NACK, umsg);
                }
                break;
            case BKP_DATA:
                res = conn.recv_packet(0, &pkt);
                // Verifica se teve erro ao receber, não quer dizer que tipo da 
                // menssagem é PKT_ERRO.
                if (res < 0) {
                    printf("[ERRO]: %s\n", strerror(errno));
                    continue;
                }

                if (res == PKT_DADOS) {
                    // Recebe dados e manda ACK
                    conn.send_packet(PKT_ACK, umsg);
                } else if (res == PKT_FIM_TX_DADOS) {
                    // Finaliza escritura de arquivo e troca para IDLE
                    conn.send_packet(PKT_ACK, umsg);
                } else if (res == PKT_VERIFICA ||
                           res == PKT_RESTAURA ||
                           res == PKT_BACKUP)
                {
                    // Se for alguma das funções principais, voltar para IDLE com erro
                    // Monta menssagem de erro de OPERATION_INTERRUPT e coloca em umsg
                    state = IDLE;
                    conn.send_packet(PKT_ERRO, umsg);
                } else {
                    // Não reconheceu pacote, enviar NACK
                    conn.send_packet(PKT_NACK, umsg);
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
