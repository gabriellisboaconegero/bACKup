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

// int idle_state(struct connection_t *conn, struct packet_t *pkt) {
//     vector<uint8_t> umsg;
//     string msg = "[IDLE]: Resposta do servidor";
//     int res, new_state = IDLE;
//     umsg.resize(msg.size());
//     copy(msg.begin(), msg.end(), umsg.begin());

//     res = conn->recv_packet(0, pkt);
//     // Se deu algum erro, não quer dizer que packet é do
//     // tipo PKT_ERRO ou PKT_NACK
//     if (res < 0) {
//         printf("[ERRO]: %s\n", strerror(errno));
//         return ERR;
//     }

//     if (res == PKT_VERIFICA) {
//         // Verifica se arquivo existe
//         // Se não exitir manda erro
//         // if (!file_exist(file_name)) {
//         //     // Monta erro corretamente em umsg
//         //     conn.send_packet(PKT_ERRO, umsg);
//         //     continue;
//         // }
//         msg += ": VERIFICA";
//         umsg.resize(msg.size());
//         copy(msg.begin(), msg.end(), umsg.begin());
//         // Calcula checksum e coloca em umsg
//         if (conn->send_packet(PKT_OK_CKSUM, umsg) < 0)
//             return ERR;
//     } else if (res == PKT_BACKUP) {
//         // Verifica se arquivo ja existe, se existir manda erro
//         // if (!file_exist(file_name)) {
//         //  // Monta menssagem de erro corretamente em umsg
//         //  conn.send_packet(PKT_ERRO, umsg);
//         //  continue;
//         // }
//         // Muda estado para BKP_INIT e manda OK
//         msg += ": BACKUP";
//         umsg.resize(msg.size());
//         copy(msg.begin(), msg.end(), umsg.begin());
//         if(conn->send_packet(PKT_OK, umsg) < 0)
//             return ERR;
//         new_state = BKP_INIT;
//     } else if (res == PKT_RESTAURA) {
//         // Verifica se arquivo existe e se usuário tem acesso
//         // if (!file_exist(file_name) && !file_access(file_name)){
//         //  // Monta menssagem de erro corretamente em umsg
//         //  conn.send_packet(PKT_ERRO, umsg);
//         //  continue;
//         // }
//         // Muda estado para RESTA_INIT e manda OK_TAM
//         // Pega tamanho do arquivo e coloca em umsg
//         if (conn->send_packet(PKT_OK_TAM, umsg) < 0)
//             return ERR;
//         new_state = RESTA_INIT;
//     } else {
//         // Pacote não reconhecido, envia NACK
//         if (conn->send_packet(PKT_NACK, umsg) < 0)
//             return ERR;
//     }

//     return new_state;
// }

// int bkp_init_state(struct connection_t *conn, struct packet_t *pkt) {
//     vector<uint8_t> umsg;
//     string msg = "[BKP_INIT]: Resposta do servidor";
//     int res, new_state = BKP_INIT;
//     umsg.resize(msg.size());
//     copy(msg.begin(), msg.end(), umsg.begin());

//     res = conn->recv_packet(0, pkt);
//     // Se deu algum erro, não quer dizer que packet é do
//     // tipo PKT_ERRO ou PKT_NACK
//     if (res < 0) {
//         printf("[ERRO]: %s\n", strerror(errno));
//         return ERR;
//     }

//     if (res == PKT_TAM) {
//         // verifica se tem espaço para criar arquivo, volta para IDLE
//         // if (!has_space(file_tam)) {
//         //  // Monta menssagem de erro corretamente em umsg
//         //  conn.send_packet(PKT_ERRO, umsg);
//         //  state = IDLE;
//         //  continue;
//         // }
//         // Cria arquivo e manda OK, muda para BKP_DATA
//         if (conn->send_packet(PKT_OK, umsg) < 0)
//             return ERR;
//         new_state = BKP_DATA;
//     } else if (res == PKT_VERIFICA ||
//             res == PKT_RESTAURA ||
//             res == PKT_BACKUP)
//     {
//         // Se for alguma das funções principais, voltar para IDLE com erro
//         // Monta menssagem de erro de OPERATION_INTERRUPT e coloca em umsg
//         if (conn->send_packet(PKT_ERRO, umsg) < 0)
//             return ERR;
//         new_state = IDLE;
//     } else {
//         // Não reconheceu pacote, enviar NACK
//         if (conn->send_packet(PKT_NACK, umsg) < 0)
//             return ERR;
//     }

//     return new_state;
// }

// int bkp_data_state(struct connection_t *conn, struct packet_t *pkt) {
//     vector<uint8_t> umsg;
//     string msg = "[BKP_DATA]: Resposta do servidor";
//     int res, new_state = BKP_DATA;
//     umsg.resize(msg.size());
//     copy(msg.begin(), msg.end(), umsg.begin());

//     res = conn->recv_packet(0, pkt);
//     // Se deu algum erro, não quer dizer que packet é do
//     // tipo PKT_ERRO ou PKT_NACK
//     if (res < 0) {
//         printf("[ERRO]: %s\n", strerror(errno));
//         return ERR;
//     }

//     if (res == PKT_DADOS) {
//         // Recebe dados e manda ACK, coloca os dados no arquivo
//         if (conn->send_packet(PKT_ACK, umsg) < 0)
//             return ERR;
//     } else if (res == PKT_FIM_TX_DADOS) {
//         // Finaliza escritura de arquivo e troca para IDLE
//         if (conn->send_packet(PKT_ACK, umsg) < 0)
//             return ERR;
//         new_state = IDLE;
//     } else if (res == PKT_VERIFICA ||
//             res == PKT_RESTAURA ||
//             res == PKT_BACKUP)
//     {
//         // Se for alguma das funções principais, voltar para IDLE com erro
//         // Monta menssagem de erro de OPERATION_INTERRUPT e coloca em umsg
//         if (conn->send_packet(PKT_ERRO, umsg) < 0)
//             return ERR;
//         new_state = IDLE;
//     } else {
//         // Não reconheceu pacote, enviar NACK
//         if (conn->send_packet(PKT_NACK, umsg) < 0)
//             return ERR;
//     }

//     return new_state;
// }

void verifica(struct connection_t *conn) {
    // Verifica se arquivo existe, senão manda erro e sai
    printf("-------------------- VERIFICA --------------------\n");
    if (get_file_name(conn->last_pkt_recv.dados)) {
        conn->send_erro(NO_FILE_ERRO);
        return;
    }
    
    // Faz checksum do arquivo e retorna
    vector<uint8_t> umsg = calculate_cksum();
    struct packet_t pkt = conn->make_packet(PKT_OK_CKSUM, umsg);
    conn->send_packet(&pkt);
    printf("-------------------- VERIFICA --------------------\n");
}

void backup3(struct connection_t *conn) {
    struct packet_t pkt;
    int res;
    
    while (1) {
        res = conn->recv_packet(0, &pkt);
        if (res < 0) {
            printf("[ERRO %s:%s:%d]: %s\n", __FILE__, __func__, __LINE__, strerror(errno));
            return;
        }
        // Salava ultimo recebido
        conn->save_last_recv(&pkt);
        printf("[BACKUP3]: Menssagem recebida: "); print_packet(&pkt);
        // Espera por dados, então manda ack
        if (res == PKT_DADOS) {
            conn->send_ack();
        // FIm de transmissão
        } else if (res == PKT_FIM_TX_DADOS) { 
            printf("[BACKUP3]: Fim da recepção de dados\n");
            conn->send_ack();
            break;
        // Qualquer coisa que não seja DADOS e FIM manda nack
        } else {
            conn->send_nack();
        }
    }
}

void backup2(struct connection_t *conn) {
    struct packet_t pkt;
    int res;

    while(1) {
        res = conn->recv_packet(0, &pkt);
        if (res < 0) {
            printf("[ERRO %s:%s:%d]: %s\n", __FILE__, __func__, __LINE__, strerror(errno));
            return;
        }
        // Salava ultimo recebido
        conn->save_last_recv(&pkt);
        printf("[BACKUP2]: Menssagem recebida: "); print_packet(&pkt);
        // Espera receber TAM do cliente
        if (res == PKT_TAM) {
            break;
        }
        conn->send_nack();
    }
    
    // Verifica se tem espaço ná maquina para armazenar arquivo
    if (!has_disc_space(&conn->last_pkt_recv)) {
        printf("[ERRO]: Sem espaço no disco para receber arquivo\n");
        conn->send_erro(NO_DISK_SPACE_ERRO);
    }

    // Confirma recebimento do TAM
    conn->send_ok();
    backup3(conn);
}

void backup(struct connection_t *conn) {
    struct packet_t pkt;

    printf("-------------------- BACKUP --------------------\n");
    // Se o arquivo ja existir verifica se tem acesso a ele.
    if (get_file_name(conn->last_pkt_recv.dados)) {
        printf("[ERRO]: Sem acesso a arquivo\n");
        conn->send_erro(NO_FILE_ACCESS_ERRO);
        return;
    }

    // Confirma recebimento do nome do arquivo
    conn->send_ok();

    backup2(conn);
    printf("-------------------- BACKUP --------------------\n");
}

void restaura2(struct connection_t *conn) {
    string msg = "[RESTAURA]: Dados do arquivo";
    vector<uint8_t> umsg;
    int res, round, interval = PACKET_TIMEOUT_INTERVAL;
    struct packet_t s_pkt, r_pkt;

    // Pega nome do arquivo e coloca em umsg
    umsg.resize(msg.size());
    copy(msg.begin(), msg.end(), umsg.begin());

    // Loop pelas partes do arquivo
    for (int i = 0; i < 5; i++) {
        // Envia menssagem nome até receber PKT_ACK
        s_pkt = conn->make_packet(PKT_DADOS, umsg);
        for (round = 0; round < PACKET_RETRASMISSION_ROUNDS; round++) {
#ifdef DEBUG
            printf("[DEBUG]: Tentativa (%d\\%d)\n", round+1, PACKET_RETRASMISSION_ROUNDS);
#endif
            if (conn->send_packet(&s_pkt) < 0) {
                printf("[ERRO %s:%s:%d]: %s\n", __FILE__, __func__, __LINE__, strerror(errno));
                return;
            }
#ifdef DEBUG
            printf("[DEBUG]: Menssagem Enviada: "); print_packet(&s_pkt);
#endif
            res = conn->recv_packet(interval, &r_pkt);
            // Se deu algum erro, não quer dizer que packet é do
            // tipo PKT_ERRO ou PKT_NACK
            if (res < 0) {
                printf("[ERRO %s:%s:%d]: %s\n", __FILE__, __func__, __LINE__, strerror(errno));
                return;
            }
#ifdef DEBUG
            printf("[DEBUG]: Menssagem Recebida: "); print_packet(&r_pkt);
#endif
            if (res == PKT_ACK)
                break;
        }
        // Se alcançou o maximo de retransmissões, marca que teve timeout
        if (round == PACKET_RETRASMISSION_ROUNDS) {
            printf("[VERIFICA:TIMEOUT]: Ocorreu timeout tentando verificar o arquivo\n");
            return;
        }
        conn->update_seq();
    }

    msg = "[RESTAURA]: Fim dos dados";
    umsg.resize(msg.size());
    copy(msg.begin(), msg.end(), umsg.begin());

    s_pkt = conn->make_packet(PKT_FIM_TX_DADOS, umsg);
    for (round = 0; round < PACKET_RETRASMISSION_ROUNDS; round++) {
#ifdef DEBUG
        printf("[DEBUG]: Tentativa (%d\\%d)\n", round+1, PACKET_RETRASMISSION_ROUNDS);
#endif
        if (conn->send_packet(&s_pkt) < 0) {
            printf("[ERRO %s:%s:%d]: %s\n", __FILE__, __func__, __LINE__, strerror(errno));
            return;
        }
#ifdef DEBUG
        printf("[DEBUG]: Menssagem enviada: "); print_packet(&s_pkt);
#endif
        res = conn->recv_packet(interval, &r_pkt);
        // Se deu algum erro, não quer dizer que packet é do
        // tipo PKT_ERRO ou PKT_NACK
        if (res < 0) {
            printf("[ERRO %s:%s:%d]: %s\n", __FILE__, __func__, __LINE__, strerror(errno));
            return;
        }
#ifdef DEBUG
        printf("[DEBUG]: Menssagem Recebida: "); print_packet(&r_pkt);
#endif
        if (res == PKT_ACK)
            break;
    }
    // Se alcançou o maximo de retransmissões, marca que teve timeout
    if (round == PACKET_RETRASMISSION_ROUNDS) {
        printf("[RESTAURA:TIMEOUT]: Ocorreu timeout tentando restaurar arquivo\n");
        return;
    }
    conn->update_seq();
}

void restaura(struct connection_t *conn) {
    struct packet_t s_pkt, r_pkt;
    int res, round, interval = PACKET_TIMEOUT_INTERVAL;

    printf("-------------------- RESTAURA --------------------\n");
    // Se o arquivo ja existir verifica se tem acesso a ele.
    if (get_file_name(conn->last_pkt_recv.dados)) {
        printf("[ERRO]: Sem acesso a arquivo\n");
        conn->send_erro(NO_FILE_ACCESS_ERRO);
        return;
    }

    vector<uint8_t> file_size = get_file_size(&conn->last_pkt_recv);
    s_pkt = conn->make_packet(PKT_OK_TAM, file_size);
    conn->save_last_send(&s_pkt);
    // Confirma recebimento do nome do arquivo, e manda tamanho dele
    for (round = 0; round < PACKET_RETRASMISSION_ROUNDS; round++) {
#ifdef DEBUG
        printf("[DEBUG]: Tentativa (%d\\%d)\n", round+1, PACKET_RETRASMISSION_ROUNDS);
#endif
        // Se receber pacote corrompido então envia nack,caso cliente
        // tenha mandado NACK mas corrompeu.
        if (round != 0 && res == PKT_UNKNOW) {
            if (conn->send_nack() < 0) {
                printf("[ERRO %s:%s:%d]: %s\n", __FILE__, __func__, __LINE__, strerror(errno));
                return;
            }
#ifdef DEBUG
            printf("[DEBUG]: Menssagem enviada: (tipo: NACK, seq: 0, size: 14)\n");
#endif
        }else {
            if (conn->send_packet(&s_pkt) < 0) {
                printf("[ERRO %s:%s:%d]: %s\n", __FILE__, __func__, __LINE__, strerror(errno));
                return;
            }
#ifdef DEBUG
            printf("[DEBUG]: Menssagem enviada: "); print_packet(&s_pkt);
#endif
        }
        res = conn->recv_packet(interval, &r_pkt);
        // Se deu algum erro, não quer dizer que packet é do
        // tipo PKT_ERRO ou PKT_NACK
        if (res < 0) {
            printf("[ERRO %s:%s:%d]: %s\n", __FILE__, __func__, __LINE__, strerror(errno));
            return;
        }
        #ifdef DEBUG
        printf("[DEBUG]: Menssagem Recebida: "); print_packet(&r_pkt);
        #endif
        // Se receber NACK continua enviando. Caso contrario
        if (res == PKT_OK || res == PKT_ERRO)
            break;
    }
    // Se alcançou o maximo de retransmissões, marca que teve timeout
    if (round == PACKET_RETRASMISSION_ROUNDS) {
        printf("[RESTAURA:TIMEOUT]: Ocorreu timeout ao restaurar arquivo\n");
        return;
    }
    conn->update_seq();

    if (res == PKT_ERRO) {
        printf("[RESTAURA:ERRO]: Erro aconteceu no cliente.\n\tCLIENTE: %s\n", erro_to_str(r_pkt.dados[0]));
        return;
    }

    restaura2(conn);
    printf("-------------------- RESTAURA --------------------\n");
}

void servidor(string interface) {
    struct connection_t conn;
    struct packet_t pkt;
    int res;
    vector<uint8_t> umsg;
    string msg = "[IDLE]: Resposta do servidor";
    umsg.resize(msg.size());
    copy(msg.begin(), msg.end(), umsg.begin());


    // Cria conxão
    if (!conn.connect(interface.data())) {
        cout << "[ERRO]: Erro ao criar conexão com interface (" << interface << ")" << endl;
        cout << "[ERRO]: " << strerror(errno) << endl;
        exit(1);
    }

    while(1) {
        res = conn.recv_packet(0, &pkt);
        if (res < 0) {
            printf("[ERRO]: RESPOSTA(%d) Não deveria chegar aqui\n", res);
            printf("[ERRNO]: %s\n", strerror(errno));
            exit(1);
        }
        #ifdef DEBUG
        printf("[DEBUG]: MENSSAGEM RECEBIDA: "); print_packet(&pkt);
        #endif
        conn.save_last_recv(&pkt);
        switch (res) {
            // Chama função de restaurar arquivos
            case PKT_RESTAURA:
                restaura(&conn);
                break;
            // Chama função de fazer backup
            case PKT_BACKUP:
                backup(&conn);
                break;
            // Chama função de verificar
            case PKT_VERIFICA:
                verifica(&conn);
                break;
            // Recebeu pacote que não entendeu, então manda nack
            case PKT_UNKNOW:
                conn.send_nack();
                break;
            default:
                printf("[ERRO]: Pacote ignorado (%s)\n", tipo_to_str(res));
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
