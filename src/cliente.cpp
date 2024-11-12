#include "socket.h"
#include "utils.h"
using namespace std;

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
        if (res == PKT_OK_CKSUM || res == PKT_ERRO) {
            // Salva pacote lido e recebido
            conn->save_last_recv(&r_pkt);
            conn->save_last_send(&s_pkt);

            break;
        }
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
            if (res == PKT_ACK) {
                // Salva pacote lido e recebido
                conn->save_last_recv(&r_pkt);
                conn->save_last_send(&s_pkt);

                break;
            }
        }
        // Se alcançou o maximo de retransmissões, marca que teve timeout
        if (round == PACKET_RETRASMISSION_ROUNDS) {
            printf("[VERIFICA:TIMEOUT]: Ocorreu timeout tentando verificar o arquivo\n");
            return;
        }
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
        if (res == PKT_ACK) {
            // Salva pacote lido e recebido
            conn->save_last_recv(&r_pkt);
            conn->save_last_send(&s_pkt);

            break;
        }
    }
    // Se alcançou o maximo de retransmissões, marca que teve timeout
    if (round == PACKET_RETRASMISSION_ROUNDS) {
        printf("[VERIFICA:TIMEOUT]: Ocorreu timeout tentando verificar o arquivo\n");
        return;
    }
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
        if (res == PKT_OK || res == PKT_ERRO) {
            // Salva pacote lido e recebido
            conn->save_last_recv(&r_pkt);
            conn->save_last_send(&s_pkt);

            break;
        }
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
        if (res == PKT_OK || res == PKT_ERRO) {
            // Salva pacote lido e recebido
            conn->save_last_recv(&r_pkt);
            conn->save_last_send(&s_pkt);

            break;
        }
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

    backup2(conn);
}

void restaura2(struct connection_t *conn) {
    struct packet_t pkt;
    int res;
    
    while (1) {
        res = conn->recv_packet(RECEIVER_MAX_TIMEOUT, &pkt);
        if (res < 0) {
            printf("[ERRO %s:%s:%d]: %s\n", __FILE__, __func__, __LINE__, strerror(errno));
            return;
        }
        if (res == PKT_TIMEOUT) {
            printf("[TIMEOUT]: Cliente ficou inativo por muito tempo\n");
            printf("[TIMEOUT]: Cancelando operação de RESTAURA\n");
            return;
        }
        printf("[RESTAURA2]: Menssagem recebida: "); print_packet(&pkt);
        if (SEQ_MOD(conn->last_pkt_recv.seq) == SEQ_MOD(pkt.seq)) {
#ifdef DEBUG
            printf("[DEBUG]: Pacote (tipo: %s, seq: %d) já processado\n", tipo_to_str(pkt.tipo), pkt.seq);
#endif
            conn->send_packet(&conn->last_pkt_send);
            continue;
        }
        // Espera por dados, então manda ack
        if (res == PKT_DADOS) {
            // Salva ultimo recebido
            conn->save_last_recv(&pkt);
            conn->send_ack(1);
        // FIm de transmissão
        } else if (res == PKT_FIM_TX_DADOS) { 
            // Salva ultimo recebido
            conn->save_last_recv(&pkt);
            printf("[RESTAURA2]: Fim da recepção de dados\n");
            conn->send_ack(1);
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
        if (res == PKT_OK_TAM || res == PKT_ERRO) {
            // Salva pacote lido e recebido
            conn->save_last_recv(&r_pkt);
            conn->save_last_send(&s_pkt);

            break;
        }
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
    // Senão tiver espaço manda erro.
    if (!has_disc_space(&r_pkt)) {
        conn->send_erro(NO_DISK_SPACE_ERRO, 1);
        return;
    }
    //  Envia OK para sinalizar que pode receber os arquivos
    conn->send_ok(1);

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
    // Cria conexão com socket
    if (!conn.connect(interface.data())) {
        cout << "[ERRO]: Erro ao criar conexão com interface (" << interface << ")" << endl;
        cout << "[ERRO]: " << strerror(errno) << endl;
        exit(1);
    }

    while(1) {
        printf("Escolha uma das opções:\n\t0 - VERIFICA\n\t1 - BACKUP\n\t2 - RESTAURA\n\toutro para sair\n>> ");
        cin >> opt;
        if (opt > 3 && opt < 0)
            exit(1);

        // Limpa socket antes da próxima ação
        if (!conn.reset_connection(interface.data())) {
            printf("[ERRO]: Não foi possível restabelecer a conexão: %s\n", strerror(errno));
            exit(1);
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
