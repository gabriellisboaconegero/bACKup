#include "socket.h"
#include "utils.h"
#include <fstream>
using namespace std;
namespace fs = std::filesystem;

static unsigned long crctab[] = {
    0x00000000,
    0x04c11db7, 0x09823b6e, 0x0d4326d9, 0x130476dc, 0x17c56b6b,
    0x1a864db2, 0x1e475005, 0x2608edb8, 0x22c9f00f, 0x2f8ad6d6,
    0x2b4bcb61, 0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd,
    0x4c11db70, 0x48d0c6c7, 0x4593e01e, 0x4152fda9, 0x5f15adac,
    0x5bd4b01b, 0x569796c2, 0x52568b75, 0x6a1936c8, 0x6ed82b7f,
    0x639b0da6, 0x675a1011, 0x791d4014, 0x7ddc5da3, 0x709f7b7a,
    0x745e66cd, 0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
    0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5, 0xbe2b5b58,
    0xbaea46ef, 0xb7a96036, 0xb3687d81, 0xad2f2d84, 0xa9ee3033,
    0xa4ad16ea, 0xa06c0b5d, 0xd4326d90, 0xd0f37027, 0xddb056fe,
    0xd9714b49, 0xc7361b4c, 0xc3f706fb, 0xceb42022, 0xca753d95,
    0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1, 0xe13ef6f4,
    0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d, 0x34867077, 0x30476dc0,
    0x3d044b19, 0x39c556ae, 0x278206ab, 0x23431b1c, 0x2e003dc5,
    0x2ac12072, 0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16,
    0x018aeb13, 0x054bf6a4, 0x0808d07d, 0x0cc9cdca, 0x7897ab07,
    0x7c56b6b0, 0x71159069, 0x75d48dde, 0x6b93dddb, 0x6f52c06c,
    0x6211e6b5, 0x66d0fb02, 0x5e9f46bf, 0x5a5e5b08, 0x571d7dd1,
    0x53dc6066, 0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
    0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e, 0xbfa1b04b,
    0xbb60adfc, 0xb6238b25, 0xb2e29692, 0x8aad2b2f, 0x8e6c3698,
    0x832f1041, 0x87ee0df6, 0x99a95df3, 0x9d684044, 0x902b669d,
    0x94ea7b2a, 0xe0b41de7, 0xe4750050, 0xe9362689, 0xedf73b3e,
    0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2, 0xc6bcf05f,
    0xc27dede8, 0xcf3ecb31, 0xcbffd686, 0xd5b88683, 0xd1799b34,
    0xdc3abded, 0xd8fba05a, 0x690ce0ee, 0x6dcdfd59, 0x608edb80,
    0x644fc637, 0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb,
    0x4f040d56, 0x4bc510e1, 0x46863638, 0x42472b8f, 0x5c007b8a,
    0x58c1663d, 0x558240e4, 0x51435d53, 0x251d3b9e, 0x21dc2629,
    0x2c9f00f0, 0x285e1d47, 0x36194d42, 0x32d850f5, 0x3f9b762c,
    0x3b5a6b9b, 0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
    0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623, 0xf12f560e,
    0xf5ee4bb9, 0xf8ad6d60, 0xfc6c70d7, 0xe22b20d2, 0xe6ea3d65,
    0xeba91bbc, 0xef68060b, 0xd727bbb6, 0xd3e6a601, 0xdea580d8,
    0xda649d6f, 0xc423cd6a, 0xc0e2d0dd, 0xcda1f604, 0xc960ebb3,
    0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7, 0xae3afba2,
    0xaafbe615, 0xa7b8c0cc, 0xa379dd7b, 0x9b3660c6, 0x9ff77d71,
    0x92b45ba8, 0x9675461f, 0x8832161a, 0x8cf30bad, 0x81b02d74,
    0x857130c3, 0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640,
    0x4e8ee645, 0x4a4ffbf2, 0x470cdd2b, 0x43cdc09c, 0x7b827d21,
    0x7f436096, 0x7200464f, 0x76c15bf8, 0x68860bfd, 0x6c47164a,
    0x61043093, 0x65c52d24, 0x119b4be9, 0x155a565e, 0x18197087,
    0x1cd86d30, 0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
    0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088, 0x2497d08d,
    0x2056cd3a, 0x2d15ebe3, 0x29d4f654, 0xc5a92679, 0xc1683bce,
    0xcc2b1d17, 0xc8ea00a0, 0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb,
    0xdbee767c, 0xe3a1cbc1, 0xe760d676, 0xea23f0af, 0xeee2ed18,
    0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4, 0x89b8fd09,
    0x8d79e0be, 0x803ac667, 0x84fbdbd0, 0x9abc8bd5, 0x9e7d9662,
    0x933eb0bb, 0x97ffad0c, 0xafb010b1, 0xab710d06, 0xa6322bdf,
    0xa2f33668, 0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4
};

// Gera o crc 32 bits de uma certa sequência de bytes.
// vide https://www.sunshine2k.de/articles/coding/crc/understanding_crc.html#ch6
uint partial_crc32(vector<uint8_t>::iterator buf_begin,
                 vector<uint8_t>::iterator buf_end, uint crc=0)
{
    for (; buf_begin != buf_end; ++buf_begin) {
        uint8_t pos = (uint8_t)((crc ^ (*buf_begin << 24)) >> 24);
        crc = (uint)((crc << 8) ^ (uint)(crctab[pos]));
    }
    return crc;
}

int calculate_cksum(fs::path file_path, uint *cksum) {
    vector<uint8_t> buf(BYTES1K);
    uint crc = 0;
    ifstream ifs;
    buf.resize(BYTES1K);

    // Abre arquivo e itera sobre ele até final ou erro
    ifs.open(file_path, ifstream::binary);
    while (ifs.good()) {
        // Lê tamanho do buffer
        ifs.read((char *)&buf[0], buf.size());
        // Faz um resize no buffer para quantos bytes foram lidos por read.
        // Basicamente evitar de mandar um buffer que tenha 0's no final quando terminar
        // de ler o arquivo.
        buf.resize(ifs.gcount());
        crc = partial_crc32(buf.begin(), buf.end(), crc);
    }
    // Para iteração por erro e não por ter acabado arquivo
    if (!ifs.eof()) {
        printf("[ERRO]: Algo aconteceu ao transmitir o arquivo.");
        printf("\t[ERRO]: %s\n", strerror(errno));
        return -1;
    }
    
#ifdef DEBUG
    printf("[DEBUG]: cksum do arquivo (%s) é: %u\n", file_path.c_str(), crc);
#endif
    *cksum = crc;

    return 0;
}

const char *erro_to_str(int tipo) {
    switch (tipo) {
        case NO_FILE_ERRO:          return "Arquivo inexistente"; break;
        case NO_FILE_ACCESS_ERRO:   return "Arquivo com acesso negado"; break;
        case NO_DISK_SPACE_ERRO:    return "Sem espaço em disco para tamanho solicitado"; break;
        case READING_FILE_ERRO:     return "Erro lendo arquivo"; break;
    }
    return "[ERRO]: Unreachable";
}

const char *tipo_to_str(int tipo) {
    switch (tipo) {
        case PKT_ACK: return "ACK"; break;
        case PKT_NACK: return "NACK"; break;
        case PKT_OK: return "OK"; break;
        case PKT_BACKUP: return "BACKUP"; break;
        case PKT_RESTAURA: return "RESTAURA"; break;
        case PKT_VERIFICA: return "VERIFICA"; break;
        case PKT_OK_CKSUM: return "OK_CKSUM"; break;
        case PKT_OK_TAM: return "OK_TAM"; break;
        case PKT_TAM: return "TAM"; break;
        case PKT_DADOS: return "DADOS"; break;
        case PKT_FIM_TX_DADOS: return "FIM_TX_DADOS"; break;
        case PKT_ERRO: return "ERRO"; break;
        case PKT_UNKNOW: return "UNKNOW"; break;
    }
    return "[ERRO]: Unreachable";
}

void print_packet(struct packet_t *pkt) {
    if (pkt == NULL){
        printf("(NULL PKT)\n");
        return;
    }
    printf("(tipo: %s, seq: %d, size: %d): ",
                    tipo_to_str(pkt->tipo), pkt->seq, pkt->tam);
    for (uint8_t i = 0; i < pkt->tam; i++)
        printf("%x ", pkt->dados[i]);
    printf("\n");
}


int get_interfaces(vector<string> &interface_names) {
    struct ifaddrs *interfaces, *ifa;
    set<string> interface_set;

    // Obtém a lista de interfaces de rede
    if (getifaddrs(&interfaces) == -1) {
        perror("Erro ao obter interfaces de rede");
        return -1;
    }

    // Itera sobre as interfaces e adiciona os nomes ao set para evitar duplicatas
    for (ifa = interfaces; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr)
            continue;

        interface_set.insert(ifa->ifa_name);
    }
    interface_names.resize(interface_set.size());
    copy(interface_set.begin(), interface_set.end(), interface_names.begin());

    // Libera a memória alocada para a lista de interfaces
    freeifaddrs(interfaces);
    return 0;
}

int send_file(struct connection_t *conn, fs::path file_path) {
    vector<uint8_t> umsg(PACKET_MAX_DADOS_SIZE);
    int res;
    struct packet_t s_pkt, r_pkt;
    ifstream ifs;

    // Abre arquivo e itera sobre ele até final ou erro
    ifs.open(file_path, ifstream::binary);
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
            printf("[TIMEOUT]: Ocorreu timeout tentando transmitir arquivo\n");
            return -1;
        }
    }
    // Para iteração por erro e não por ter acabado arquivo
    // Não retorna ainda, manda FIM_TX
    if (!ifs.eof()) {
        printf("[ERRO]: Algo aconteceu ao transmitir o arquivo.");
        printf("\t[ERRO]: %s\n", strerror(errno));
    }

    s_pkt = conn->make_packet(PKT_FIM_TX_DADOS, "");
    // Envia menssagem nome até receber PKT_ACK
    res = conn->send_await_packet(&s_pkt, &r_pkt, {PKT_ACK}, PACKET_TIMEOUT_INTERVAL);
    // Se alcançou o maximo de retransmissões, marca que teve timeout
    if (res == PKT_TIMEOUT) {
        printf("[TIMEOUT]: Ocorreu timeout tentando finalizar transmissão do arquivo\n");
        return -1;
    }
    printf("[SEND_FILE]: Transmissão do arquivo (%s) completo.\n", file_path.filename().c_str());

    return 0;
}

int recv_file(struct connection_t *conn, fs::path file_path) {
    struct packet_t pkt;
    int res;
    ofstream ofs;

    ofs.open(file_path.c_str(), ofstream::out | ofstream::binary | ofstream::trunc);
    while (1) {
        res = conn->recv_packet(RECEIVER_MAX_TIMEOUT, &pkt);
        if (res < 0) {
            printf("[ERRO %s:%s:%d]: %s\n", __FILE__, __func__, __LINE__, strerror(errno));
            return -1;
        }

        // Timeout de deconexão. Ficou muito tempo esperando receber dados.
        // É esperado que RECEIVER_MAX_TIMEOUT seja alto.
        if (res == PKT_TIMEOUT) {
            printf("[TIMEOUT]: Inatividade por muito tempo. Cancelando operação\n");
            return -1;
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

            // Escreve conteudo no arquivo
            ofs.write((char *)&pkt.dados[0], pkt.tam);

            conn->send_ack(1);
            // FIm de transmissão
        } else if (res == PKT_FIM_TX_DADOS) { 
            // Salva ultimo recebido
            conn->save_last_recv(&pkt);
            conn->send_ack(1);
            break;
            // Qualquer coisa que não seja DADOS e FIM manda nack
        } else {
            conn->send_nack();
        }
    }
    ofs.close();
    printf("[RECV_FILE]: Recebimento do arquivo (%s) completo.\n", file_path.filename().c_str());
    
    return 0;
}

void recv_file_name(struct connection_t *conn, string *file_name) {
    file_name->resize(conn->last_pkt_recv.tam);
    copy_n(conn->last_pkt_recv.dados.begin(), conn->last_pkt_recv.tam, file_name->begin());
#ifdef DEBUG
    printf("[DEBUG]: Nome do arquivo recebido: %s\n", file_name->data());
#endif
}

int process_file_name(fs::path *file_path, string file_name, const char *dir_path) {
    fs::file_status file_st;

    *file_path = file_name;
    // Pega arquivo do diretório de backup
    *file_path = (fs::current_path() / dir_path) / *file_path;
    file_st = fs::status(*file_path);
    // Verifica se arquivo existe, senão manda erro e sai
    if (fs::exists(file_st)) {
        if (fs::perms::none == (file_st.permissions() & fs::perms::owner_write)) {
            return NO_FILE_ACCESS_ERRO;
        }
    } else {
        return NO_FILE_ERRO;
    }

    return 0;
}
