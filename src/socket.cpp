#include "socket.h"
using namespace std;
#define PACKET_MI 0x7e // 0b01111110
// Bitmask de cada campo do packet
// Marcador de inicio (8bits / 1 byte)
// hex: f    f     0    0     0    0     0    0
// bin: 1111 1111  0000 0000  0000 0000  0000 0000
#define MI_BITMASK 0xff000000
#define MI_OFFSET 24
#define get_mi(inter) ((inter & MI_BITMASK) >> MI_OFFSET)

// Tamanho(6 bits)
// hex: 0    0     f    c     0    0     0    0
// bin: 0000 0000  1111 1100  0000 0000  0000 0000
#define TAM_BITMASK 0x00fc0000
#define TAM_OFFSET 18

// Sequencia (5 bits)
// hex: 0    0     0    3     e    0     0    0
// bin: 0000 0000  0000 0011  1110 0000  0000 0000
#define SEQ_BITMASK 0x0003e000
#define SEQ_OFFSET 13

// Tipo (5 bits)
// hex: 0    0     0    0     1    f     0    0
// bin: 0000 0000  0000 0000  0001 1111  0000 0000
#define TIPO_BITMASK 0x00001f00
#define TIPO_OFFSET 8

// ler os seguintes manuais para entender melhor o que essa função faz
//      man packet
//      man socket
//      man setsockopt
int cria_raw_socket(char* nome_interface_rede) {
    // Cria arquivo para o socket sem qualquer protocolo
    int soquete = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (soquete == -1) {
        fprintf(stderr, "Erro ao criar socket: Verifique se você é root!\n");
        exit(-1);
    }
 
    int ifindex = if_nametoindex(nome_interface_rede);
 
    struct sockaddr_ll endereco = {0};
    endereco.sll_family = AF_PACKET;
    endereco.sll_protocol = htons(ETH_P_ALL);
    endereco.sll_ifindex = ifindex;
    // Inicializa socket
    if (bind(soquete, (struct sockaddr*) &endereco, sizeof(endereco)) == -1) {
        fprintf(stderr, "Erro ao fazer bind no socket\n");
        exit(-1);
    }
 
    struct packet_mreq mr = {0};
    mr.mr_ifindex = ifindex;
    mr.mr_type = PACKET_MR_PROMISC;
    // Não joga fora o que identifica como lixo: Modo promíscuo
    if (setsockopt(soquete, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr)) == -1) {
        fprintf(stderr, "Erro ao fazer setsockopt: "
            "Verifique se a interface de rede foi especificada corretamente.\n");
        exit(-1);
    }
 
    return soquete;
}


// ===================== Packet =====================
    // Faz o parsing do pacote, retornando verdadeiro se o pacote é válido
    // Valida o pacote com MI e loopback.
    // Verifica se o pacote é valido de acordo com protocolo
    // Ou seja:
    //  MI é 0b01111110 (0x7e)
    //  Se não é pacote de loopback
pair<struct packet_t, bool> parse_packet(struct sockaddr_ll addr, std::vector<char> &buf) {
    // Verifica se é um outgoing packet.
    // Evita receber os pacotes duplicados no loopback
    // Vide:
    //  - https://github.com/the-tcpdump-group/libpcap/blob/1c1d968bb3f3d163fa7f6d6802842a127304dd22/pcap-linux.c#L1352
    //  - https://stackoverflow.com/a/17235405
    //  - man recvfrom
    //  - man packet
    struct packet_t res;
    if (addr.sll_pkttype == PACKET_OUTGOING)
        return make_pair(res, true);

    // Necessariamente tem marcador de inicio, tam, seq, tipo e crc, somando da 4 bytes
    if (buf.size() < 4)
        return make_pair(res, true);

    int first_4bytes = 0;
    first_4bytes |= (buf[0] << 24);
    first_4bytes |= (buf[1] << 16);
    first_4bytes |= (buf[2] << 8);

    // Verifica marcador de inicio
    if ((first_4bytes & MI_BITMASK) >> MI_OFFSET != PACKET_MI)
        return make_pair(res, true);
    
    // Pega tamanho
    res.tam = first_4bytes & TAM_BITMASK;
    // Pega sequencia
    res.seq = first_4bytes & SEQ_BITMASK;
    // Pega tipo
    res.tipo = first_4bytes & TIPO_BITMASK;
    
    // Quer dizer que não tem a quantidade correta de bytes enviados
    if (buf.size() - 4 < res.tam)
        return make_pair(res, true);

    // Verifica crc 8 bits dos campos tam, seq, tipo e dados
    /* if(!verify_crc8()) */
    /*     return make_pair(res, false); */

    // Copia os dados da menssagem, que começam depois de mi, tam, seq e tipo e acabam antes de crc
    res.dados.resize(res.tam, 0);
    copy(buf.begin()+3, buf.begin()+3+res.tam, res.dados.begin());
    return make_pair(res, false);
}
// ===================== Packet =====================

// ===================== Connection =====================
bool connection_t::connect(char *interface, int max_msg_size) {
    max_size = max_msg_size;
    socket = cria_raw_socket(interface);
    if (socket < 0)
        return false;
    return true;
}

// Recebe menssagem do socket, fica buscando até uma menssagem válida
pair<struct packet_t, bool> connection_t::recv_packet() {
    socklen_t addr_len = sizeof(addr);
    // Buffer para receber menssagem
    vector<char> buf;
    pair<struct packet_t, bool> res;
    do {
        // Prepara buffer de recebimento
        buf.resize(max_size, 0);

        // Recebe
        auto msg_len = recvfrom(socket, buf.data(), buf.size(), 0, (struct sockaddr*)&addr, &addr_len);
        cout << msg_len << endl;
        if (msg_len < 0)
            return make_pair(packet, true);

        // Trunca para quantos bytes foram recebidos
        buf.resize(msg_len);
        // Se o pacote não for valido continua tentando escutar
        res = parse_packet(addr, buf);
    }while(res.second);

    return res;
}

bool connection_t::send_packet(int tipo, string &msg) {
    if (msg.size() > 64)
        return false;
    int seq = 0;
    vector<char> buf(4+msg.size(), 0);

    int first_4byte = 0;
    // Coloca marcador de inicio
    first_4byte |= PACKET_MI;

    // Coloca tamanho
    first_4byte <<= 6;
    first_4byte |= (char)(msg.size() & 0b111111);

    // Coloca sequencia
    first_4byte <<= 5;
    first_4byte |= (char)(seq & 0b11111);

    // Coloca tipo
    first_4byte <<= 5;
    first_4byte |= (char)(tipo & 0b11111);

    // Coloca dentro do buffer
    buf[0] = (char)((first_4byte & 0xff0000) >> 16);
    buf[1] = (char)((first_4byte & 0x00ff00) >> 8);
    buf[2] = (char)((first_4byte & 0x0000ff) >> 0);

    // Coloca dados no buffer
    copy(msg.begin(), msg.end(), buf.begin()+3);
    // gera crc 8 bits
    /* buf[buf.size() - 1] = gen_crc(buf); */

    // Faz o send
    if (send(socket, buf.data(), buf.size(), 0) < 0)
        return false;
    return true;
}

// ===================== Connection =====================
