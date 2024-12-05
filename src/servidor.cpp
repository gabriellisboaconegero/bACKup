#include "socket.h"
#include "utils.h"
#include <fstream>
#include <filesystem>
#include <exception>
using namespace std;
namespace fs = std::filesystem;

void verifica(struct connection_t *conn) {
    struct packet_t s_pkt, r_pkt;
    fs::path file_path;
    string file_name;
    uint cksum;
    int res;

    printf("-------------------- VERIFICA --------------------\n");
    recv_file_name(conn, &file_name);
    res = process_file_name(&file_path, file_name, BACKUP_FILES_PATH);
    if (res == NO_FILE_ACCESS_ERRO) {
        printf("[ERRO]: Sem acesso a arquivo (%s)\n", file_path.c_str());
        conn->send_erro(NO_FILE_ACCESS_ERRO, 1);
        return;

    }
    if (res == NO_FILE_ERRO) {
        printf("[ERRO]: Arquivo não existente (%s)\n", file_path.c_str());
        conn->send_erro(NO_FILE_ERRO, 1);
        return;
    }

    if (calculate_cksum(file_path, &cksum) < 0) {
        conn->send_erro(READING_FILE_ERRO, 1);
        return;
    }

    // Manda pacote com crc calculado
    struct packet_t pkt = conn->make_packet(PKT_OK_CKSUM, to_uint8_t<uint>(cksum));
    if (conn->send_packet(&pkt, 1) < 0) {
        printf("[ERRO %s:%s:%d]: %s\n", __FILE__, __func__, __LINE__, strerror(errno));
        return;
    }
    printf("-------------------- VERIFICA --------------------\n");
}

void backup2(struct connection_t *conn, fs::path file_path) {
    struct packet_t pkt;
    fs::space_info sp;
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
    
    size_t file_size = uint8_t_to<size_t>(conn->last_pkt_recv.dados);
#ifdef DEBUG
    printf("[DEBUG]: Tamanho recebido (%ld)\n", file_size);
#endif

    // Verifica se tem espaço ná maquina para armazenar arquivo
    sp = fs::space(BACKUP_FILES_PATH);
    size_t available_size = sp.available;
#ifdef DEBUG
    printf("[DEBUG]: Espaço livre na máquina: %ld bytes\n", available_size);
#endif
    if (file_size >= available_size) {
        printf("[ERRO]: Sem espaço no disco para receber arquivo\n");
        conn->send_erro(NO_DISK_SPACE_ERRO, 1);
        return;
    }

    // Confirma recebimento do TAM
    conn->send_ok(1);
    recv_file(conn, file_path);
}

void backup(struct connection_t *conn) {
    struct packet_t pkt;
    fs::path file_path;
    string file_name;
    int res;

    printf("-------------------- BACKUP --------------------\n");
    recv_file_name(conn, &file_name);
    res = process_file_name(&file_path, file_name, BACKUP_FILES_PATH);
    if (res == NO_FILE_ACCESS_ERRO) {
        printf("[ERRO]: Sem acesso a arquivo (%s)\n", file_path.c_str());
        conn->send_erro(NO_FILE_ACCESS_ERRO, 1);
        return;

    }
    if (res != 0)
        printf("[WARNING]: Arquivo (%s) vai ser sobrescrito\n", file_path.c_str());

    // Confirma recebimento do nome do arquivo
    conn->send_ok(1);

    backup2(conn, file_path);
    printf("-------------------- BACKUP --------------------\n");
}

void restaura(struct connection_t *conn) {
    vector<uint8_t> umsg;
    struct packet_t s_pkt, r_pkt;
    fs::path file_path;
    fs::file_status file_st;
    string file_name;
    size_t file_size;
    int res;

    printf("-------------------- RESTAURA --------------------\n");
    recv_file_name(conn, &file_name);
    res = process_file_name(&file_path, file_name, BACKUP_FILES_PATH);
    if (res == NO_FILE_ACCESS_ERRO) {
        printf("[ERRO]: Sem acesso a arquivo (%s)\n", file_path.c_str());
        conn->send_erro(NO_FILE_ACCESS_ERRO, 1);
        return;

    }
    if (res == NO_FILE_ERRO) {
        printf("[ERRO]: Arquivo não existente (%s)\n", file_path.c_str());
        conn->send_erro(NO_FILE_ERRO, 1);
        return;
    }

    try {
        file_size = fs::file_size(file_path);
    }
    catch (fs::filesystem_error &e) {
        printf("[ERRO]: Não foi possível restaurar arquivo (%s)\n", file_path.c_str());
        printf("\t[ERRO]: %s\n", e.what());
        conn->send_erro(READING_FILE_ERRO, 1);
        return;
    }

    s_pkt = conn->make_packet(PKT_OK_TAM, to_uint8_t<size_t>(file_size));

    // Habilita flag de nacks, ela permite enviar nack ao receber unknow pkt
    res = conn->send_await_packet(&s_pkt, &r_pkt,
                {PKT_ERRO, PKT_OK}, PACKET_TIMEOUT_INTERVAL, true);
    // Se alcançou o maximo de retransmissões, marca que teve timeout
    if (res == PKT_TIMEOUT) {
        printf("[TIMEOUT]: Ocorreu timeout tentando restaurar arquivo\n");
        return;
    }
    if (res == PKT_ERRO) {
        printf("[ERRO]: Erro aconteceu no cliente.\n\tCLIENTE: %s\n", erro_to_str(r_pkt.dados[0]));
        return;
    }

    send_file(conn, file_path);
    printf("-------------------- RESTAURA --------------------\n");
}

void servidor(string interface) {
    struct connection_t conn;
    struct packet_t pkt;
    int res;


    // Cria conxão
    if (!conn.connect(interface.data())) {
        cout << "[ERRO]: Erro ao criar conexão com interface (" << interface << ")" << endl;
        cout << "[ERRO]: " << strerror(errno) << endl;
        exit(1);
    }
    // Cria diretório de backup
    try {
        fs::create_directories(BACKUP_FILES_PATH);
#ifdef DEBUG
        printf("[DEBUG]: Diretório %s de backup criado\n", BACKUP_FILES_PATH);
#endif
    } catch (fs::filesystem_error &e) {
        printf("[ERRO]: Ocorreu um erro ao criar diretório de backup (%s)\n", BACKUP_FILES_PATH);
        printf("\t[ERRO]: %s\n", e.what());
        return;
    }

    while(1) {
        printf("Escutando...\n");
#ifdef SIMULATE_UNSTABLE
        conn.reset_count();
#endif
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
#ifdef SIMULATE_UNSTABLE
        conn.print_count();
#endif
    }
    cout << "Saindo do servidor" << endl;
}

int main() {
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
