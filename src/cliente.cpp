#include "socket.h"
#include "utils.h"
#include <fstream>
using namespace std;

void verifica(struct connection_t *conn) {
    // Verifica se arquivo existe, senão manda erro e sai
    string msg = "[VERIFICA]: Nome do arquivo";
    vector<uint8_t> umsg;
    int res;
    struct packet_t s_pkt, r_pkt;

    // Pega nome do arquivo e coloca em umsg
    umsg.resize(msg.size());
    copy(msg.begin(), msg.end(), umsg.begin());

    s_pkt = conn->make_packet(PKT_VERIFICA, umsg);

    res = conn->send_await_packet(&s_pkt, &r_pkt, {PKT_ERRO, PKT_OK_CKSUM}, PACKET_TIMEOUT_INTERVAL);
    // Se alcançou o maximo de retransmissões, marca que teve timeout
    if (res == PKT_TIMEOUT) {
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

void backup3(struct connection_t *conn, string file_name, size_t file_size) {
    string msg;
    vector<uint8_t> umsg(PACKET_MAX_DADOS_SIZE);
    int res;
    struct packet_t s_pkt, r_pkt;
    ifstream ifs;

    // Abre arquivo e itera sobre ele até final ou erro
    ifs.open(file_name, ifstream::binary);
    while (ifs.good()) {
        // Lê tamanho do buffer
        ifs.read((char *)&umsg[0], umsg.size());
        // Faz um resize no buffer para quantos bytes foram lidos por read.
        // Basicamente evitar de mandar um buffer que tenha 0's no final quando terminar
        // de ler o arquivo.
        umsg.resize(ifs.gcount());
        s_pkt = conn->make_packet(PKT_DADOS, umsg);
        // Envia menssagem nome até receber PKT_ACK
        res = conn->send_await_packet(&s_pkt, &r_pkt, {PKT_ACK}, PACKET_TIMEOUT_INTERVAL);
        // Se alcançou o maximo de retransmissões, marca que teve timeout
        if (res == PKT_TIMEOUT) {
            printf("[BACKUP3:TIMEOUT]: Ocorreu timeout tentando fazer backup do arquivo\n");
            return;
        }
    }
    // Para iteração por erro e não por ter acabado arquivo
    if (!ifs.eof()) {
        printf("[ERRO]: Algo aconteceu ao transmitir o arquivo.");
        printf("\t[ERRO]: %s\n", strerror(errno));
    }

    msg = "[BACKUP]: Fim dos dados";
    umsg.resize(msg.size());
    copy(msg.begin(), msg.end(), umsg.begin());

    s_pkt = conn->make_packet(PKT_FIM_TX_DADOS, umsg);
    // Envia menssagem nome até receber PKT_ACK
    res = conn->send_await_packet(&s_pkt, &r_pkt, {PKT_ACK}, PACKET_TIMEOUT_INTERVAL);
    // Se alcançou o maximo de retransmissões, marca que teve timeout
    if (res == PKT_TIMEOUT) {
        printf("[BACKUP3:TIMEOUT]: Ocorreu timeout tentando fazer backup do arquivo\n");
        return;
    }
}

void backup2(struct connection_t *conn, string file_name, size_t file_size) {
    string msg = "[BACKUP]: Tamanho do arquivo";
    vector<uint8_t> umsg;
    int res;
    struct packet_t s_pkt, r_pkt;

    // Pega tamanho do arquivo e coloca em umsg
    umsg = size_t_to_uint8_t(file_size);
#ifdef DEBUG
    printf("[DEBUG]: Tamanho enviado (%ld)\n", file_size);
#endif

    s_pkt = conn->make_packet(PKT_TAM, umsg);

    // Envia menssagem nome até receber PKT_ERRO ou PKT_OK
    res = conn->send_await_packet(&s_pkt, &r_pkt, {PKT_ERRO, PKT_OK}, PACKET_TIMEOUT_INTERVAL);
    // Se alcançou o maximo de retransmissões, marca que teve timeout
    if (res == PKT_TIMEOUT) {
        printf("[BACKUP2:TIMEOUT]: Ocorreu timeout tentando fazer backup do arquivo\n");
        return;
    }
    if (res == PKT_ERRO) {
        printf("[BACKUP2:ERRO]: Erro aconteceu no servidor.\n\tSERVIDOR: %s\n", erro_to_str(r_pkt.dados[0]));
        return;
    }

    backup3(conn, file_name, file_size);
}

void backup(struct connection_t *conn, string file_name, size_t file_size) {
    // Verifica se arquivo existe, senão manda erro e sai
    vector<uint8_t> umsg;
    int res;
    struct packet_t s_pkt, r_pkt;

    // Pega nome do arquivo e coloca em umsg
    umsg.resize(file_name.size());
    copy(file_name.begin(), file_name.end(), umsg.begin());

    s_pkt = conn->make_packet(PKT_BACKUP, umsg);

    // envia nome do arquivo e espera por PKT_ERRO ou PKT_OK
    res = conn->send_await_packet(&s_pkt, &r_pkt, {PKT_ERRO, PKT_OK}, PACKET_TIMEOUT_INTERVAL);
    if (res == MSG_TO_BIG) {
        printf("Arquivo %s tem nome grande demais. Insira arquivo com nome menor\n", file_name.data());
        return;
    }

    if (res < 0)
        return;

    // Se alcançou o maximo de retransmissões, marca que teve timeout
    if (res == PKT_TIMEOUT) {
        printf("[BACKUP:TIMEOUT]: Ocorreu timeout tentando fazer backup do arquivo\n");
        return;
    }
    if (res == PKT_ERRO) {
        printf("[BACKUP:ERRO]: Erro aconteceu no servidor.\n\tSERVIDOR: %s\n", erro_to_str(r_pkt.dados[0]));
        return;
    }

    backup2(conn, file_name, file_size);
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

        // Timeout de deconexão. Ficou muito tempo esperando receber dados.
        // É esperado que RECEIVER_MAX_TIMEOUT seja alto.
        if (res == PKT_TIMEOUT) {
            printf("[TIMEOUT]: Cliente ficou inativo por muito tempo\n");
            printf("[TIMEOUT]: Cancelando operação de RESTAURA\n");
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
    int res;
    struct packet_t s_pkt, r_pkt;

    // Pega nome do arquivo e coloca em umsg
    umsg.resize(msg.size());
    copy(msg.begin(), msg.end(), umsg.begin());

    s_pkt = conn->make_packet(PKT_RESTAURA, umsg);

    // Habilita flag de nacks, ela permite enviar nack ao receber unknow pkt
    res = conn->send_await_packet(&s_pkt, &r_pkt,
                {PKT_ERRO, PKT_OK_TAM}, PACKET_TIMEOUT_INTERVAL, true);
    // Se alcançou o maximo de retransmissões, marca que teve timeout
    if (res == PKT_TIMEOUT) {
        printf("[RESTAURA:TIMEOUT]: Ocorreu timeout tentando restaurar arquivo\n");
        return;
    }
    if (res == PKT_ERRO) {
        printf("[RESTAURA:ERRO]: Erro aconteceu no servidor.\n\tSERVIDOR: %s\n", erro_to_str(r_pkt.dados[0]));
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
    string file_name;
    size_t file_size;
    vector<uint8_t> file_size_buf;
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
        if (opt > 3 || opt < 0)
            exit(1);

        // Limpa socket antes da próxima ação
        if (!conn.reset_connection(interface.data())) {
            printf("[ERRO]: Não foi possível restabelecer a conexão: %s\n", strerror(errno));
            exit(1);
        }

        switch (opts[opt]) {
            case PKT_BACKUP:
                printf("Digite o caminho do arquivo: ");
                cin >> file_name;
                if (file_name.size() > PACKET_MAX_DADOS_SIZE) {
                    printf("[ERRO]: Nome de arquivo grande demais (%ld). Deve ter no máximo %d\n",
                            file_name.size(), PACKET_MAX_DADOS_SIZE);
                    break;
                }
                if (get_file_size(file_name, &file_size) < 0) {
                    printf("[ERRO]: Não foi possível fazer backup do arquivo (%s)\n", file_name.data());
                    printf("\t[ERRO]: %s\n", strerror(errno));
                    break;
                }
                backup(&conn, file_name, file_size);
                break;
                // Chama função de verificar
            // Chama função de restaurar arquivos
            case PKT_RESTAURA:
                restaura(&conn);
                break;
                // Chama função de fazer backup
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
