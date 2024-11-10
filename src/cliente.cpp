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

// int idle_state(struct connection_t *conn, struct packet_t *pkt) {
//     vector<uint8_t> umsg;
//     string msg = "[IDLE]: MSG cliente";
//     int new_state = IDLE;
//     umsg.resize(msg.size());
//     copy(msg.begin(), msg.end(), umsg.begin());
//     int opt;
//     // Apresenta opções de escolha. BACKUP, VERIFICA e RESTAURA
//     int opts[3] = {
//         PKT_VERIFICA,
//         PKT_BACKUP,
//         PKT_RESTAURA,
//     };
//     printf("Escolha uma das opções:\n\t0 - VERIFICA\n\t1 - BACKUP\n\t2 - RESTAURA\n\t3 - testa recv\n\toutro para sair\n>> ");
//     cin >> opt;
//     if (opt > 4 && opt < 0)
//         exit(1);

//     if (opt == 3){
//         conn->recv_packet(1000, pkt);
//         print_packet(pkt);
//         return new_state;
//     }
//     if (opts[opt] == PKT_VERIFICA) {
//         // Pede o nome de um arquivo para recuperar e coloca em umsg
//         if (conn->send_packet(PKT_VERIFICA, umsg) < 0)
//             return ERR;
//         new_state = VERIFICA;
//     } else if (opts[opt] == PKT_BACKUP) {
//         // Pede o nome de um arquivo para fazer backup e coloca em umsg
//         if (conn->send_packet(PKT_BACKUP, umsg) < 0)
//             return ERR;
//         new_state = BKP_INIT;
//     } else if (opts[opt] == PKT_RESTAURA) {
//         // Pede o nome de um arquivo para restaurar e coloca em umsg
//         if (conn->send_packet(PKT_RESTAURA, umsg) < 0)
//             return ERR;
//         new_state = RESTA_INIT;
//     }
    
//     return new_state;
// }

// int verifica_state(struct connection_t *conn, struct packet_t *pkt) {
//     vector<uint8_t> umsg;
//     int res, round, interval = PACKET_TIMEOUT_INTERVAL;
//     struct packet_t r_pkt;

//     // Copia ultima menssagem para umsg, para ser enviada em casa de timeout
//     umsg.resize(conn->last_pkt.dados.size());
//     copy(conn->last_pkt.dados.begin(), conn->last_pkt.dados.end(), umsg.begin());

//     // Tenta fazer o timeout, somente tem timeout em NACKS
//     // Qualquer outra menssagem vai sair.
//     for (round = 0; round < PACKET_RETRASMISSION_ROUNDS; round++) {
// #ifdef DEBUG
//         printf("[DEBUG]: Tentativa (%d\\%d)\n", round+1, PACKET_RETRASMISSION_ROUNDS);
// #endif
//         res = conn->recv_packet(interval, pkt);
//         // Se deu algum erro, não quer dizer que packet é do
//         // tipo PKT_ERRO ou PKT_NACK
//         if (res < 0) {
//             printf("[ERRO]: %s\n", strerror(errno));
//             return ERR;
//         }
//         if (res != PKT_TIMEOUT && res != PKT_NACK)
//             break;

//         if (conn->send_packet(conn->last_pkt.tipo, umsg) < 0)
//             return ERR;

// #ifdef DEBUG
//         printf("[DEBUG]: Menssagem enviada com sucesso\n");
//         printf("[DEBUG]: "); print_packet(&conn->last_pkt);
// #endif
//     }
//     // Se alcançou o maximo de retransmissões, marca que teve timeout
//     if (round == PACKET_RETRASMISSION_ROUNDS) {
//         pkt->tipo = PKT_TIMEOUT;
//     }

//     // Sempre volta para o idle
//     return IDLE;
// }

// int bkp_init_state(struct connection_t *conn, struct packet_t *pkt) {
//     vector<uint8_t> umsg;
//     string msg = "[BKP_INIT]: MSG cliente";
//     int res, new_state = BKP_INIT, round, interval = PACKET_TIMEOUT_INTERVAL;
//     struct packet_t r_pkt;

//     // Copia ultima menssagem para umsg, para ser enviada em casa de timeout
//     umsg.resize(conn->last_pkt.dados.size());
//     copy(conn->last_pkt.dados.begin(), conn->last_pkt.dados.end(), umsg.begin());

//     // Tenta fazer o timeout, somente tem timeout em NACKS
//     // Qualquer outra menssagem vai sair.
//     for (round = 0; round < PACKET_RETRASMISSION_ROUNDS; round++) {
// #ifdef DEBUG
//         printf("[DEBUG]: Tentativa (%d\\%d)\n", round+1, PACKET_RETRASMISSION_ROUNDS);
// #endif
//         res = conn->recv_packet(interval, pkt);
//         // Se deu algum erro, não quer dizer que packet é do
//         // tipo PKT_ERRO ou PKT_NACK
//         if (res < 0) {
//             printf("[ERRO]: %s\n", strerror(errno));
//             return ERR;
//         }
//         if (res != PKT_TIMEOUT && res != PKT_NACK)
//             break;

//         if (conn->send_packet(conn->last_pkt.tipo, umsg) < 0)
//             return ERR;

// #ifdef DEBUG
//         printf("[DEBUG]: Menssagem enviada com sucesso\n");
//         printf("[DEBUG]: "); print_packet(&conn->last_pkt);
// #endif
//     }
//     // Se alcançou o maximo de retransmissões, marca que teve timeout
//     if (round == PACKET_RETRASMISSION_ROUNDS) {
//         res = PKT_TIMEOUT;
//         pkt->tipo = PKT_TIMEOUT;
//     }

//     if (res == PKT_OK){
//         // Calcula tamanho do arquivo corretamente e coloca em umsg
//         umsg.resize(msg.size());
//         copy(msg.begin(), msg.end(), umsg.begin());
//         if (conn->send_packet(PKT_TAM, umsg) < 0)
//             return ERR;
//         new_state = BKP_TAM;
//     } else {
//         // Qualquer outra resposta volta para IDLE sem enviar nada
//         new_state = IDLE;
//     }
//     return new_state;
// }

// int bkp_tam_state(struct connection_t *conn, struct packet_t *pkt) {
//     vector<uint8_t> umsg;
//     string msg = "[BKP_TAM]: MSG cliente";
//     int res, new_state = BKP_TAM, round, interval = PACKET_TIMEOUT_INTERVAL;
//     struct packet_t r_pkt;

//     // Copia ultima menssagem para umsg, para ser enviada em casa de timeout
//     umsg.resize(conn->last_pkt.dados.size());
//     copy(conn->last_pkt.dados.begin(), conn->last_pkt.dados.end(), umsg.begin());

//     // Tenta fazer o timeout, somente tem timeout em NACKS
//     // Qualquer outra menssagem vai sair.
//     for (round = 0; round < PACKET_RETRASMISSION_ROUNDS; round++) {
// #ifdef DEBUG
//         printf("[DEBUG]: Tentativa (%d\\%d)\n", round+1, PACKET_RETRASMISSION_ROUNDS);
// #endif
//         res = conn->recv_packet(interval, pkt);
//         // Se deu algum erro, não quer dizer que packet é do
//         // tipo PKT_ERRO ou PKT_NACK
//         if (res < 0) {
//             printf("[ERRO]: %s\n", strerror(errno));
//             return ERR;
//         }
//         if (res != PKT_TIMEOUT && res != PKT_NACK)
//             break;

//         if (conn->send_packet(conn->last_pkt.tipo, umsg) < 0)
//             return ERR;

// #ifdef DEBUG
//         printf("[DEBUG]: Menssagem enviada com sucesso\n");
//         printf("[DEBUG]: "); print_packet(&conn->last_pkt);
// #endif
//     }
//     // Se alcançou o maximo de retransmissões, marca que teve timeout
//     if (round == PACKET_RETRASMISSION_ROUNDS) {
//         res = PKT_TIMEOUT;
//         pkt->tipo = PKT_TIMEOUT;
//     }

//     if (res == PKT_OK){
//         // Pega primeira parte dos dados do arquivo e envia
//         msg += ": DADOS 1/2";
//         umsg.resize(msg.size());
//         copy(msg.begin(), msg.end(), umsg.begin());
//         if (conn->send_packet(PKT_DADOS, umsg) < 0)
//             return ERR;
//         new_state = BKP_DATA;
//     } else {
//         // Qualquer outra resposta volta para IDLE sem enviar nada
//         new_state = IDLE;
//     }
//     return new_state;
// }

// int bkp_data_state(struct connection_t *conn, struct packet_t *pkt, int num) {
//     vector<uint8_t> umsg;
//     string msg = "[BKP_DATA]: MSG cliente";
//     int res, new_state = BKP_DATA, round, interval = PACKET_TIMEOUT_INTERVAL;
//     struct packet_t r_pkt;

//     // Copia ultima menssagem para umsg, para ser enviada em casa de timeout
//     umsg.resize(conn->last_pkt.dados.size());
//     copy(conn->last_pkt.dados.begin(), conn->last_pkt.dados.end(), umsg.begin());

//     // Tenta fazer o timeout, somente tem timeout em NACKS
//     // Qualquer outra menssagem vai sair.
//     for (round = 0; round < PACKET_RETRASMISSION_ROUNDS; round++) {
// #ifdef DEBUG
//         printf("[DEBUG]: Tentativa (%d\\%d)\n", round+1, PACKET_RETRASMISSION_ROUNDS);
// #endif
//         res = conn->recv_packet(interval, pkt);
//         // Se deu algum erro, não quer dizer que packet é do
//         // tipo PKT_ERRO ou PKT_NACK
//         if (res < 0) {
//             printf("[ERRO]: %s\n", strerror(errno));
//             return ERR;
//         }
//         if (res != PKT_TIMEOUT && res != PKT_NACK)
//             break;

//         if (conn->send_packet(conn->last_pkt.tipo, umsg) < 0)
//             return ERR;

// #ifdef DEBUG
//         printf("[DEBUG]: Menssagem enviada com sucesso\n");
//         printf("[DEBUG]: "); print_packet(&conn->last_pkt);
// #endif
//     }
//     // Se alcançou o maximo de retransmissões, marca que teve timeout
//     if (round == PACKET_RETRASMISSION_ROUNDS) {
//         res = PKT_TIMEOUT;
//         pkt->tipo = PKT_TIMEOUT;
//     }

//     if (res == PKT_ACK){
//         // Continua pegando os proximos pedaços da menssagem e enviando.
//         // Se for o ultimo pedaço envia PKT_FIM_TX_DADOS
//         if (num) {
//             msg += ": DADOS 2/2";
//             umsg.resize(msg.size());
//             copy(msg.begin(), msg.end(), umsg.begin());
//             if (conn->send_packet(PKT_DADOS, umsg) < 0)
//                 return ERR;
//             new_state = BKP_DATA;
//         } else {
//             msg += ": FIM";
//             umsg.resize(msg.size());
//             copy(msg.begin(), msg.end(), umsg.begin());
//             if (conn->send_packet(PKT_FIM_TX_DADOS, umsg) < 0)
//                 return ERR;
//             new_state = BKP_FIM;
//         }
//     } else {
//         // Qualquer outra resposta volta para IDLE sem enviar nada
//         new_state = IDLE;
//     }
//     return new_state;
// }

// int bkp_fim_state(struct connection_t *conn, struct packet_t *pkt) {
//     vector<uint8_t> umsg;
//     int res, round, interval = PACKET_TIMEOUT_INTERVAL;
//     struct packet_t r_pkt;

//     // Copia ultima menssagem para umsg, para ser enviada em casa de timeout
//     umsg.resize(conn->last_pkt.dados.size());
//     copy(conn->last_pkt.dados.begin(), conn->last_pkt.dados.end(), umsg.begin());

//     // Tenta fazer o timeout, somente tem timeout em NACKS
//     // Qualquer outra menssagem vai sair.
//     for (round = 0; round < PACKET_RETRASMISSION_ROUNDS; round++) {
// #ifdef DEBUG
//         printf("[DEBUG]: Tentativa (%d\\%d)\n", round+1, PACKET_RETRASMISSION_ROUNDS);
// #endif
//         res = conn->recv_packet(interval, pkt);
//         // Se deu algum erro, não quer dizer que packet é do
//         // tipo PKT_ERRO ou PKT_NACK
//         if (res < 0) {
//             printf("[ERRO]: %s\n", strerror(errno));
//             return ERR;
//         }
//         if (res != PKT_TIMEOUT && res != PKT_NACK)
//             break;

//         if (conn->send_packet(conn->last_pkt.tipo, umsg) < 0)
//             return ERR;

// #ifdef DEBUG
//         printf("[DEBUG]: Menssagem enviada com sucesso\n");
//         printf("[DEBUG]: "); print_packet(&conn->last_pkt);
// #endif
//     }
//     // Se alcançou o maximo de retransmissões, marca que teve timeout
//     if (round == PACKET_RETRASMISSION_ROUNDS) {
//         res = PKT_TIMEOUT;
//         pkt->tipo = PKT_TIMEOUT;
//     }

//     // Sempre volta para o IDLE
//     return IDLE;
// }

void verifica(struct connection_t *conn) {
    // Verifica se arquivo existe, senão manda erro e sai
    string msg = "[VERIFICA]: Nome do arquivo";
    vector<uint8_t> umsg;
    int res, round, interval = PACKET_TIMEOUT_INTERVAL;
    struct packet_t s_pkt, r_pkt;

    // Pega nome do arquivo e coloca em umsg
    umsg.resize(msg.size());
    copy(msg.begin(), msg.end(), umsg.begin());

    s_pkt = conn->make_packet(PKT_VERIFICA, umsg);

    for (round = 0; round < PACKET_RETRASMISSION_ROUNDS; round++) {
#ifdef DEBUG
        printf("[DEBUG]: Tentativa (%d\\%d)\n", round+1, PACKET_RETRASMISSION_ROUNDS);
#endif
        if (conn->send_packet(&s_pkt) < 0) {
            printf("[ERRO %s:%s:%d]: %s\n", __FILE__, __func__, __LINE__, strerror(errno));
            return;
        }
#ifdef DEBUG
        printf("[DEBUG]: Menssagem enviada com sucesso\n");
        printf("[DEBUG]: "); print_packet(&s_pkt);
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
        if (res == PKT_OK_CKSUM || res == PKT_ERRO)
            break;
    }
    // Se alcançou o maximo de retransmissões, marca que teve timeout
    if (round == PACKET_RETRASMISSION_ROUNDS) {
        printf("[VERIFICA:TIMEOUT]: Ocorreu timeout tentando verificar o arquivo\n");
        return;
    }
    if (res == PKT_ERRO) {
        printf("[VERIFICA:ERRO]: Erro aconteceu no servidor.\n\tSERVIDOR: %s\n", erro_to_str(r_pkt.dados[0]));
        return;
    }
    conn->save_last_send(&s_pkt);
    conn->update_seq();

    // Calcula cksum também e verifica se é igual
    if (calculate_cksum() == r_pkt.dados) {
        printf("[VERIFICA]: CHECKSUM correto entre servidor e cliente\n");
    } else {
        printf("[VERIFICA]: CHECKSUM incorreto entre servidor e cliente\n");
    }
}

void backup3(struct connection_t *conn) {
    string msg = "[BACKUP]: Dados do arquivo";
    vector<uint8_t> umsg;
    int res, round, interval = PACKET_TIMEOUT_INTERVAL;
    struct packet_t s_pkt, r_pkt;

    // Pega nome do arquivo e coloca em umsg
    umsg.resize(msg.size());
    copy(msg.begin(), msg.end(), umsg.begin());

    for (int i = 0; i < 5; i++) {
        // Envia menssagem nome até receber PKT_ERRO ou PKT_OK
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

    msg = "[BACKUP]: Fim dos dados";
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
        printf("[VERIFICA:TIMEOUT]: Ocorreu timeout tentando verificar o arquivo\n");
        return;
    }
    conn->update_seq();
}

void backup2(struct connection_t *conn) {
    string msg = "[BACKUP]: Tamanho do arquivo";
    vector<uint8_t> umsg;
    int res, round, interval = PACKET_TIMEOUT_INTERVAL;
    struct packet_t s_pkt, r_pkt;

    // Pega nome do arquivo e coloca em umsg
    umsg.resize(msg.size());
    copy(msg.begin(), msg.end(), umsg.begin());

    s_pkt = conn->make_packet(PKT_TAM, umsg);

    // Envia menssagem nome até receber PKT_ERRO ou PKT_OK
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
        if (res == PKT_OK || res == PKT_ERRO)
            break;
    }
    // Se alcançou o maximo de retransmissões, marca que teve timeout
    if (round == PACKET_RETRASMISSION_ROUNDS) {
        printf("[VERIFICA:TIMEOUT]: Ocorreu timeout tentando verificar o arquivo\n");
        return;
    }
    if (res == PKT_ERRO) {
        printf("[VERIFICA:ERRO]: Erro aconteceu no servidor.\n\tSERVIDOR: %s\n", erro_to_str(r_pkt.dados[0]));
        return;
    }
    conn->update_seq();

    backup3(conn);
}

void backup(struct connection_t *conn) {
    // Verifica se arquivo existe, senão manda erro e sai
    string msg = "[BACKUP]: Nome do arquivo";
    vector<uint8_t> umsg;
    int res, round, interval = PACKET_TIMEOUT_INTERVAL;
    struct packet_t s_pkt, r_pkt;

    // Pega nome do arquivo e coloca em umsg
    umsg.resize(msg.size());
    copy(msg.begin(), msg.end(), umsg.begin());

    s_pkt = conn->make_packet(PKT_BACKUP, umsg);

    // Envia menssagem nome até receber PKT_ERRO ou PKT_OK
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
        if (res == PKT_OK || res == PKT_ERRO)
            break;
    }
    // Se alcançou o maximo de retransmissões, marca que teve timeout
    if (round == PACKET_RETRASMISSION_ROUNDS) {
        printf("[VERIFICA:TIMEOUT]: Ocorreu timeout tentando verificar o arquivo\n");
        return;
    }
    if (res == PKT_ERRO) {
        printf("[VERIFICA:ERRO]: Erro aconteceu no servidor.\n\tSERVIDOR: %s\n", erro_to_str(r_pkt.dados[0]));
        return;
    }
    conn->update_seq();

    backup2(conn);
}

void restaura2(struct connection_t *conn) {
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
        printf("[RESTAURA2]: Menssagem recebida: "); print_packet(&pkt);
        // Espera por dados, então manda ack
        if (res == PKT_DADOS) {
            conn->send_ack();
        // FIm de transmissão
        } else if (res == PKT_FIM_TX_DADOS) { 
            printf("[RESTAURA2]: Fim da recepção de dados\n");
            conn->send_ack();
            break;
        // Qualquer coisa que não seja DADOS e FIM manda nack
        } else {
            conn->send_nack();
        }
    }
}

void restaura(struct connection_t *conn) {
    // Verifica se arquivo existe, senão manda erro e sai
    string msg = "[RESTAURA]: Nome do arquivo";
    vector<uint8_t> umsg;
    int res, round, interval = PACKET_TIMEOUT_INTERVAL;
    struct packet_t s_pkt, r_pkt;

    // Pega nome do arquivo e coloca em umsg
    umsg.resize(msg.size());
    copy(msg.begin(), msg.end(), umsg.begin());

    s_pkt = conn->make_packet(PKT_RESTAURA, umsg);

    // Envia menssagem nome até receber PKT_ERRO ou PKT_OK
    for (round = 0; round < PACKET_RETRASMISSION_ROUNDS; round++) {
#ifdef DEBUG
        printf("[DEBUG]: Tentativa (%d\\%d)\n", round+1, PACKET_RETRASMISSION_ROUNDS);
#endif
        // Se receber pacote corrompido então envia nack,caso servidor
        // tenha mandado OK_TAM mas corrompeu.
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
        if (res == PKT_OK_TAM || res == PKT_ERRO)
            break;
    }
    // Se alcançou o maximo de retransmissões, marca que teve timeout
    if (round == PACKET_RETRASMISSION_ROUNDS) {
        printf("[VERIFICA:TIMEOUT]: Ocorreu timeout tentando verificar o arquivo\n");
        return;
    }
    conn->update_seq();

    if (res == PKT_ERRO) {
        printf("[VERIFICA:ERRO]: Erro aconteceu no servidor.\n\tSERVIDOR: %s\n", erro_to_str(r_pkt.dados[0]));
        return;
    }
    // Senão tiver espaço manda erro.
    if (!has_disc_space(&r_pkt)) {
        conn->send_erro(NO_DISK_SPACE_ERRO);
        return;
    }
    //  Envia OK para sinalizar que pode receber os arquivos
    conn->send_ok();

    restaura2(conn);
}

void cliente(string interface) {
    struct connection_t conn;
    struct packet_t pkt;
    int opt;
    // Apresenta opções de escolha. BACKUP, VERIFICA e RESTAURA
    int opts[3] = {
        PKT_VERIFICA,
        PKT_BACKUP,
        PKT_RESTAURA,
    };

    if (!conn.connect(interface.data())) {
        cout << "[ERRO]: Erro ao criar conexão com interface (" << interface << ")" << endl;
        cout << "[ERRO]: " << strerror(errno) << endl;
        exit(1);
    }

    while(1) {
        printf("Escolha uma das opções:\n\t0 - VERIFICA\n\t1 - BACKUP\n\t2 - RESTAURA\n\t3 - testa recv\n\toutro para sair\n>> ");
        cin >> opt;
        if (opt > 4 && opt < 0)
            exit(1);

        if (opt == 3){
            conn.recv_packet(PACKET_TIMEOUT_INTERVAL, &pkt);
            print_packet(&pkt);
            continue;
        }
        switch (opts[opt]) {
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
            case PKT_NACK:
                conn.send_nack();
                break;
            default:
                printf("[ERRO]: Impossivel chegar aqui (%s:%s:%d)\n", __FILE__, __func__, __LINE__);
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
