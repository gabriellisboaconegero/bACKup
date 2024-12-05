#include "socket.h"
#include "utils.h"
#include <fstream>
#include <filesystem>
#include <exception>
using namespace std;

namespace fs = std::filesystem;

void verifica(struct connection_t *conn, fs::path file_path) {
    // Verifica se arquivo existe, senão manda erro e sai
    uint cksum;
    int res;
    struct packet_t s_pkt, r_pkt;

    // Pega apenas o nome do arquivo.
    string file_name = file_path.filename();
    // Verifica se nome do arquivo está correto (tamanho maximo)
    if (file_name.size() > PACKET_MAX_DADOS_SIZE) {
        printf("[ERRO]: Nome de arquivo grande demais (%ld). Deve ter no máximo %d\n",
                file_name.size(), PACKET_MAX_DADOS_SIZE);
        return;
    }

    // Envia nome do arquivo
    s_pkt = conn->make_packet(PKT_VERIFICA, file_name);
    res = conn->send_await_packet(&s_pkt, &r_pkt, {PKT_ERRO, PKT_OK_CKSUM}, PACKET_TIMEOUT_INTERVAL);
    if (res < 0) {
        printf("[ERRO %s:%s:%d]: %s\n", __FILE__, __func__, __LINE__, strerror(errno));
        return;
    }
    // Se alcançou o maximo de retransmissões, marca que teve timeout
    if (res == PKT_TIMEOUT) {
        printf("[TIMEOUT]: Ocorreu timeout tentando verificar o arquivo\n");
        return;
    }
    if (res == PKT_ERRO) {
        printf("[ERRO]: Erro aconteceu no servidor.\n\tSERVIDOR: %s\n", erro_to_str(r_pkt.dados[0]));
        return;
    }

    // Calcula cksum também e verifica se é igual
    if (calculate_cksum(file_path, &cksum) < 0) {
        printf("[ERRO]: Não foi possível calcular o cksum do arquivo (%s)\n", file_path.c_str());
        return;
    }

    if (cksum == uint8_t_to<uint>(r_pkt.dados)) {
        printf("[VERIFICA]: CHECKSUM \x1B[1;32mcorreto\x1B[m entre servidor e cliente\n");
    } else {
        printf("[VERIFICA]: CHECKSUM \x1B[1;31mincorreto\x1B[m entre servidor e cliente\n");
    }
}

void backup2(struct connection_t *conn, fs::path file_path, size_t file_size) {
    int res;
    struct packet_t s_pkt, r_pkt;

#ifdef DEBUG
    printf("[DEBUG]: Tamanho enviado (%ld)\n", file_size);
#endif
    // Pega tamanho do arquivo e coloca em umsg
    s_pkt = conn->make_packet(PKT_TAM, to_uint8_t<size_t>(file_size));

    // Envia menssagem nome até receber PKT_ERRO ou PKT_OK
    res = conn->send_await_packet(&s_pkt, &r_pkt, {PKT_ERRO, PKT_OK}, PACKET_TIMEOUT_INTERVAL);
    if (res < 0) {
        printf("[ERRO %s:%s:%d]: %s\n", __FILE__, __func__, __LINE__, strerror(errno));
        return;
    }
    // Se alcançou o maximo de retransmissões, marca que teve timeout
    if (res == PKT_TIMEOUT) {
        printf("[TIMEOUT]: Ocorreu timeout tentando fazer backup do arquivo\n");
        return;
    }
    if (res == PKT_ERRO) {
        printf("[ERRO]: Erro aconteceu no servidor.\n\tSERVIDOR: %s\n", erro_to_str(r_pkt.dados[0]));
        return;
    }

    send_file(conn, file_path);
}

void backup(struct connection_t *conn, fs::path file_path) {
    // Verifica se arquivo existe, senão manda erro e sai
    vector<uint8_t> umsg;
    int res;
    struct packet_t s_pkt, r_pkt;
    size_t file_size;
    string file_name;

    // Pega tamanho do arquivo
    try {
        file_size = fs::file_size(file_path);
    }
    catch (fs::filesystem_error &e) {
        printf("[ERRO]: Não foi possível fazer backup do arquivo (%s)\n", file_path.c_str());
        printf("\t[ERRO]: %s\n", e.what());
        return;
    }

    // Pega apenas o nome do arquivo.
    file_name = file_path.filename();
    if (file_name.size() > PACKET_MAX_DADOS_SIZE) {
        printf("[ERRO]: Nome de arquivo grande demais (%ld). Deve ter no máximo %d\n",
                file_name.size(), PACKET_MAX_DADOS_SIZE);
        return;
    }

    // Envia nome do arquivo
    s_pkt = conn->make_packet(PKT_BACKUP, file_name);
    // envia nome do arquivo e espera por PKT_ERRO ou PKT_OK
    res = conn->send_await_packet(&s_pkt, &r_pkt, {PKT_ERRO, PKT_OK}, PACKET_TIMEOUT_INTERVAL);
    if (res < 0) {
        printf("[ERRO %s:%s:%d]: %s\n", __FILE__, __func__, __LINE__, strerror(errno));
        return;
    }

    // Se alcançou o maximo de retransmissões, marca que teve timeout
    if (res == PKT_TIMEOUT) {
        printf("[TIMEOUT]: Ocorreu timeout tentando fazer backup do arquivo\n");
        return;
    }
    if (res == PKT_ERRO) {
        printf("[ERRO]: Erro aconteceu no servidor.\n\tSERVIDOR: %s\n", erro_to_str(r_pkt.dados[0]));
        return;
    }

    backup2(conn, file_path, file_size);
}

void restaura(struct connection_t *conn, fs::path file_path) {
    vector<uint8_t> umsg;
    int res;
    struct packet_t s_pkt, r_pkt;
    fs::space_info sp;
    string file_name;

    file_name = file_path.filename();
    // Verifica se tamanho do nome do arquivo está no tamanho correto
    if (file_name.size() > PACKET_MAX_DADOS_SIZE) {
        printf("[ERRO]: Nome de arquivo grande demais (%ld). Deve ter no máximo %d\n",
                file_name.size(), PACKET_MAX_DADOS_SIZE);
        return;
    }

    res = process_file_name(&file_path, file_name, RESTORE_FILES_PATH);
    if (res == NO_FILE_ACCESS_ERRO) {
        printf("[ERRO]: Sem acesso a arquivo (%s)\n", file_path.c_str());
        return;
    }
    if (res != 0)
        printf("[WARNING]: Arquivo (%s) vai ser sobrescrito\n", file_path.c_str());

    s_pkt = conn->make_packet(PKT_RESTAURA, file_name);
    // Habilita flag de nacks, ela permite enviar nack ao receber unknow pkt
    res = conn->send_await_packet(&s_pkt, &r_pkt,
                {PKT_ERRO, PKT_OK_TAM}, PACKET_TIMEOUT_INTERVAL, true);
    // Se alcançou o maximo de retransmissões, marca que teve timeout
    if (res == PKT_TIMEOUT) {
        printf("[TIMEOUT]: Ocorreu timeout tentando restaurar arquivo\n");
        return;
    }
    if (res == PKT_ERRO) {
        printf("[ERRO]: Erro aconteceu no servidor.\n\tSERVIDOR: %s\n", erro_to_str(r_pkt.dados[0]));
        return;
    }

    size_t file_size = uint8_t_to<size_t>(conn->last_pkt_recv.dados);
#ifdef DEBUG
    printf("[DEBUG]: Tamanho recebido (%ld)\n", file_size);
#endif

    // Verifica se tem espaço ná maquina para armazenar arquivo
    sp = fs::space(RESTORE_FILES_PATH);
    size_t available_size = sp.available;
#ifdef DEBUG
    printf("[DEBUG]: Espaço livre na máquina: %ld bytes\n", available_size);
#endif
    if (file_size >= available_size) {
        printf("[ERRO]: Sem espaço no disco para receber arquivo\n");
        conn->send_erro(NO_DISK_SPACE_ERRO, 1);
        return;
    }
    //  Envia OK para sinalizar que pode receber o arquivo
    conn->send_ok(1);

    recv_file(conn, file_path);
}

void cliente(string interface) {
    struct connection_t conn;
    struct packet_t pkt;
    int opt;
    fs::path file_path;
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

    // Cria diretório de backup
    try {
        fs::create_directories(RESTORE_FILES_PATH);
#ifdef DEBUG
        printf("[DEBUG]: Diretório %s de restauramento criado\n", RESTORE_FILES_PATH);
#endif
    } catch (fs::filesystem_error &e) {
        printf("[ERRO]: Ocorreu um erro ao criar diretório de restauramento (%s)\n", RESTORE_FILES_PATH);
        printf("\t[ERRO]: %s\n", e.what());
        return;
    }

    while(1) {
        printf("Escolha uma das opções:\n\t0 - VERIFICA\n\t1 - BACKUP\n\t2 - RESTAURA\n\toutro para sair\n>> ");
        cin >> opt;
        if (opt > 3 || opt < 0)
            exit(1);

        // Limpa socket antes da próxima ação
        if (!conn.reset_connection()) {
            printf("[ERRO]: Não foi possível restabelecer a conexão: %s\n", strerror(errno));
            exit(1);
        }
#ifdef SIMULATE_UNSTABLE
        conn.reset_count();
#endif

        switch (opts[opt]) {
            // Chama função de fazer backup
            case PKT_BACKUP:
                printf("Digite o caminho do arquivo: ");
                cin >> file_path;
                // Pega tamanho do arquivo, lida com erro se tiver
                backup(&conn, file_path);
                break;
            // Chama função de restaurar arquivos
            case PKT_RESTAURA:
                printf("Digite o caminho do arquivo: ");
                cin >> file_path;
                restaura(&conn, file_path);
                break;
            // Chama função de verificar
            case PKT_VERIFICA:
                printf("Digite o caminho do arquivo: ");
                cin >> file_path;
                // Pega tamanho do arquivo, lida com erro se tiver
                verifica(&conn, file_path);
                break;
                // Recebeu pacote que não entendeu, então manda nack
            case PKT_NACK:
                conn.send_nack();
                break;
            default:
                printf("[ERRO]: Impossivel chegar aqui (%s:%s:%d)\n", __FILE__, __func__, __LINE__);
                exit(1);
        }
#ifdef SIMULATE_UNSTABLE
        conn.print_count();
#endif
    }
    cout << "Saindo do cliente" << endl;
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
    cout << "Inicicalizando cliente (" << interfaces[num] << ")" << endl;
    cliente(interfaces[num].data());
}
