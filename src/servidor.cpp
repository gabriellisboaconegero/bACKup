#include "socket.h"
#include "utils.h"
using namespace std;

void verifica(struct connection_t *conn) {
    // Verifica se arquivo existe, senão manda erro e sai
    printf("-------------------- VERIFICA --------------------\n");
    if (get_file_name(conn->last_pkt_recv.dados)) {
        conn->send_erro(NO_FILE_ERRO, 1);
        return;
    }
    
    // Faz checksum do arquivo e retorna
    vector<uint8_t> umsg = calculate_cksum();
    struct packet_t pkt = conn->make_packet(PKT_OK_CKSUM, umsg);
    if (conn->send_packet(&pkt, 1) < 0) {
        printf("[ERRO %s:%s:%d]: %s\n", __FILE__, __func__, __LINE__, strerror(errno));
        return;
    }
    printf("-------------------- VERIFICA --------------------\n");
}

void backup3(struct connection_t *conn) {
    struct packet_t pkt;
    int res;
    
    while (1) {
        res = conn->recv_packet(RECEIVER_MAX_TIMEOUT, &pkt);
        if (res < 0) {
            printf("[ERRO %s:%s:%d]: %s\n", __FILE__, __func__, __LINE__, strerror(errno));
            return;
        }

        // Timeout de deconexão. Ficou muito tempo esperando receber dados.
        // É esperado que RECEIVER_MAX_TIMEOUT seja alto.
        if (res == PKT_TIMEOUT) {
            printf("[TIMEOUT]: Servidor ficou inativo por muito tempo\n");
            printf("[TIMEOUT]: Cancelando operação de BACKUP\n");
            return;
        }

        if (res == PKT_UNKNOW) {
            conn->send_nack();
            continue;
        }

        // Verifica se já processou o pacote
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
            printf("[BACKUP3]: Fim da recepção de dados\n");
            conn->send_ack(1);
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
        res = conn->recv_packet(RECEIVER_MAX_TIMEOUT, &pkt);
        if (res < 0) {
            printf("[ERRO %s:%s:%d]: %s\n", __FILE__, __func__, __LINE__, strerror(errno));
            return;
        }

        // Timeout de deconexão. Ficou muito tempo esperando receber dados.
        // É esperado que RECEIVER_MAX_TIMEOUT seja alto.
        if (res == PKT_TIMEOUT) {
            printf("[TIMEOUT]: Servidor ficou inativo por muito tempo\n");
            printf("[TIMEOUT]: Cancelando operação de BACKUP\n");
            return;
        }

        if (res == PKT_UNKNOW) {
            conn->send_nack();
            continue;
        }

        // Verifica se já processou o pacote
        if (SEQ_MOD(conn->last_pkt_recv.seq) == SEQ_MOD(pkt.seq)) {
#ifdef DEBUG
            printf("[DEBUG]: Pacote (tipo: %s, seq: %d) já processado\n", tipo_to_str(pkt.tipo), pkt.seq);
#endif
            conn->send_packet(&conn->last_pkt_send);
            continue;
        }
        // Espera receber TAM do cliente
        if (res == PKT_TAM) {
            // Salava ultimo recebido
            conn->save_last_recv(&pkt);
            break;
        }
        conn->send_nack();
    }
    
    // Verifica se tem espaço ná maquina para armazenar arquivo
    if (!has_disc_space(&conn->last_pkt_recv)) {
        printf("[ERRO]: Sem espaço no disco para receber arquivo\n");
        conn->send_erro(NO_DISK_SPACE_ERRO, 1);
        return;
    }

    // Confirma recebimento do TAM
    conn->send_ok(1);
    backup3(conn);
}

void backup(struct connection_t *conn) {
    struct packet_t pkt;

    printf("-------------------- BACKUP --------------------\n");
    // Se o arquivo ja existir verifica se tem acesso a ele.
    if (get_file_name(conn->last_pkt_recv.dados)) {
        printf("[ERRO]: Sem acesso a arquivo\n");
        conn->send_erro(NO_FILE_ACCESS_ERRO, 1);
        return;
    }

    // Confirma recebimento do nome do arquivo
    conn->send_ok(1);

    backup2(conn);
    printf("-------------------- BACKUP --------------------\n");
}

void restaura2(struct connection_t *conn) {
    string msg = "[RESTAURA]: Dados do arquivo";
    vector<uint8_t> umsg;
    int res;
    struct packet_t s_pkt, r_pkt;

    // Pega nome do arquivo e coloca em umsg
    umsg.resize(msg.size());
    copy(msg.begin(), msg.end(), umsg.begin());

    // Loop pelas partes do arquivo
    for (int i = 0; i < 5; i++) {
        s_pkt = conn->make_packet(PKT_DADOS, umsg);
        // Envia menssagem nome até receber PKT_ACK
        res = conn->send_await_packet(&s_pkt, &r_pkt, {PKT_ACK}, PACKET_TIMEOUT_INTERVAL);
        // Se alcançou o maximo de retransmissões, marca que teve timeout
        if (res == PKT_TIMEOUT) {
            printf("[RESTAURA2:TIMEOUT]: Ocorreu timeout tentando fazer backup do arquivo\n");
            return;
        }
    }

    msg = "[RESTAURA]: Fim dos dados";
    umsg.resize(msg.size());
    copy(msg.begin(), msg.end(), umsg.begin());

    s_pkt = conn->make_packet(PKT_FIM_TX_DADOS, umsg);
    // Envia menssagem nome até receber PKT_ACK
    res = conn->send_await_packet(&s_pkt, &r_pkt, {PKT_ACK}, PACKET_TIMEOUT_INTERVAL);
    // Se alcançou o maximo de retransmissões, marca que teve timeout
    if (res == PKT_TIMEOUT) {
        printf("[RESTAURA2:TIMEOUT]: Ocorreu timeout tentando fazer backup do arquivo\n");
        return;
    }
}

void restaura(struct connection_t *conn) {
    struct packet_t s_pkt, r_pkt;
    int res;

    printf("-------------------- RESTAURA --------------------\n");
    // Se o arquivo ja existir verifica se tem acesso a ele.
    if (get_file_name(conn->last_pkt_recv.dados)) {
        printf("[ERRO]: Sem acesso a arquivo\n");
        conn->send_erro(NO_FILE_ACCESS_ERRO, 1);
        return;
    }

    vector<uint8_t> file_size = get_file_size(&conn->last_pkt_recv);
    s_pkt = conn->make_packet(PKT_OK_TAM, file_size);

    // Habilita flag de nacks, ela permite enviar nack ao receber unknow pkt
    res = conn->send_await_packet(&s_pkt, &r_pkt,
                {PKT_ERRO, PKT_OK}, PACKET_TIMEOUT_INTERVAL, true);
    // Se alcançou o maximo de retransmissões, marca que teve timeout
    if (res == PKT_TIMEOUT) {
        printf("[RESTAURA:TIMEOUT]: Ocorreu timeout tentando restaurar arquivo\n");
        return;
    }
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
        printf("Escutando...\n");
        res = conn.recv_packet(0, &pkt);
        if (res < 0) {
            printf("[ERRO]: RESPOSTA(%d) Não deveria chegar aqui\n", res);
            printf("[ERRNO]: %s\n", strerror(errno));
            exit(1);
        }
        // Recebeu pacote que não entendeu, então manda nack.
        // Antes de verificar se ja processou, para não enviar
        // NACK como ja processado.
        if (res == PKT_UNKNOW) {
            conn.send_nack();
            continue;
        }

        // verifica se já processou pacote recebido
        if (!conn.first_pkt &&
                SEQ_MOD(conn.last_pkt_recv.seq) == SEQ_MOD(pkt.seq))
        {
#ifdef DEBUG
            printf("[DEBUG]: Pacote (tipo: %s, seq: %d) já processado\n", tipo_to_str(pkt.tipo), pkt.seq);
#endif
            conn.send_packet(&conn.last_pkt_send);
            continue;
        }
        switch (res) {
            // Chama função de restaurar arquivos
            case PKT_RESTAURA:
                conn.save_last_recv(&pkt);
                restaura(&conn);
                break;
            // Chama função de fazer backup
            case PKT_BACKUP:
                conn.save_last_recv(&pkt);
                backup(&conn);
                break;
            // Chama função de verificar
            case PKT_VERIFICA:
                conn.save_last_recv(&pkt);
                verifica(&conn);
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
