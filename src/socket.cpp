#include "socket.h"
using namespace std;

#ifdef DEBUG
void print_packet(struct packet_t *pkt) {
    printf("(tipo: %d, seq: %d, size: %d): ",
                    pkt->tipo, pkt->seq, pkt->tam);
    for (int i = 0; i < pkt->tam; i++)
        cout << pkt->dados[i];
    cout << endl;
}
#endif

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
// Desserializa os dados de um buffer em um packet.
// Retorna falso se o packet é inválido.
// Retorna true, c.c.
bool packet_t::deserialize(vector<uint8_t> &buf) {
    // Os valores do buffer são (em binário de 1 byte)
    // buf[0] = 0bmmmmmmmm
    // buf[1] = 0bttttttss
    // buf[2] = 0bsssTTTTT

    // Pega tamanho
    tam = buf[1] >> 2;
    // Pega sequencia
    seq = ((buf[1] & 0x03) << 3) | (buf[2] >> 5);
    // Pega tipo
    tipo = buf[2] & 0x1f;

    // Quer dizer que não tem a quantidade correta de bytes enviados em dados
    if (int(buf.size()) - PACKET_MIN_SIZE < int(tam))
        return false;

    // Verifica crc 8 bits dos campos tam, seq, tipo e dados
    // if(!verify_crc8())
    //     return false;

    // Copia os dados da menssagem, que começam depois de mi, tam, seq e tipo e acabam antes de crc
    dados.resize(tam, 0);
    copy(buf.begin()+3, buf.begin()+3+tam, dados.begin());

    return true;
}

// Serializa os dados de um packet em um buffer.
vector<uint8_t> packet_t::serialize() {
    // Os valores do buffer vão ficar (em binário de 1 byte)
    // buf[0] = 0bmmmmmmmm
    // buf[1] = 0bttttttss
    // buf[2] = 0bsssTTTTT
    vector<uint8_t> buf(PACKET_MIN_SIZE+dados.size(), 0);

    // Marcador de inicio (m)
    buf[0] = PACKET_MI;

    // Tamanho (t)
    buf[1] = tam << 2;

    // Sequencia (s)
    buf[1] |= (seq & 0x18) >> 3; // 00011000
    buf[2]  = (seq & 0x07) << 5; // 00000111

    // Tipo (T)
    buf[2] |= tipo & 0x1f;       // 00011111

    // Coloca dados no buffer, dado offset de 3 bytes
    copy(dados.begin(), dados.end(), buf.begin()+3);
    // gera crc 8 bits
    /* buf[buf.size() - 1] = gen_crc(buf); */

    return buf;
}
// Valida o pacote com MI e loopback.
// Verifica se o pacote é valido de acordo com protocolo
// Ou seja:
//  MI é 0b01111110 (0x7e)
//  Se não é pacote de loopback
bool is_valid_packet(struct sockaddr_ll addr, std::vector<uint8_t> &buf) {
    // Verifica se é um outgoing packet.
    // Evita receber os pacotes duplicados no loopback
    // Vide:
    //  - https://github.com/the-tcpdump-group/libpcap/blob/1c1d968bb3f3d163fa7f6d6802842a127304dd22/pcap-linux.c#L1352
    //  - https://stackoverflow.com/a/17235405
    //  - man recvfrom
    //  - man packet
    // if (addr.sll_pkttype == PACKET_OUTGOING)
    //     return false;

    // Necessariamente tem marcador de inicio, tam, seq, tipo e crc, somando da 4 bytes
    if (buf.size() < PACKET_MIN_SIZE)
        return false;

    // Verifica Marcador de inicio
    if (buf[0] != PACKET_MI)
        return false;

    return true;
}
// ===================== Packet =====================

// ===================== Connection =====================
// Cria raw socket de conexão e inicializa struct.
// Retorna false se não foi possivel criar socket.
// Retorn true c.c.
bool connection_t::connect(char *interface) {
    socket = cria_raw_socket(interface);
    seq = 0;
    if (socket < 0)
        return false;
    return true;
}

// Recebe o pacote, fica buscando até achar menssagem do protocolo
// Retorna false em caso de erro, true c.c.
int connection_t::recv_packet(struct packet_t *pkt) {
    socklen_t addr_len = sizeof(addr);
    // Buffer para receber menssagem
    vector<uint8_t> buf;
    do {
        // Prepara buffer de recebimento
        buf.resize(MAX_MSG_LEN, 0);

        // Recebe
        auto msg_len = recvfrom(socket, buf.data(), buf.size(), 0, (struct sockaddr*)&addr, &addr_len);
#ifdef DEBUG
        printf("%ld\n", msg_len);
#endif
        if (msg_len < 0)
            return RECV_ERR;

        // Trunca para quantos bytes foram recebidos
        buf.resize(msg_len);
#ifdef DEBUG
        for (auto i : buf)
            printf("%x ", i);
        printf("\n");
        printf("OUTGGOING: %d\n", addr.sll_pkttype == PACKET_OUTGOING);
#endif
    // Se o pacote não for valido continua tentando escutar. Se o pacote não puder
    // ser identificado também continua a escutar.
    }while(!is_valid_packet(addr, buf) || !pkt->deserialize(buf));

    return OK;
}

// Envia um packet.
// Retorna MSG_TO_BIG caso a menssagem tenha mais de 63 bytes
// Retorna SEND_ERR em caso de erro ao fazer send
// Retorna OK c.c
int connection_t::send_packet(uint8_t tipo, vector<uint8_t> &msg) {
    if (msg.size() > 63)
        return MSG_TO_BIG;

    struct packet_t pkt;
    pkt.tam = (uint8_t)(msg.size());
    // Coloca sequência do pacote a ser enviado
    pkt.seq = seq;
    pkt.tipo = tipo;
    pkt.dados.resize(msg.size());
    copy(msg.begin(), msg.end(), pkt.dados.begin());
    vector<uint8_t> buf = pkt.serialize();

    // Faz o send
    if (send(socket, buf.data(), buf.size(), 0) < 0)
        return SEND_ERR;

    // Atualiza sequência apenas se tudo der certo
    seq = (seq + 1) % (1<<SEQ_SIZE);
    return OK;
}

long long timestamp() {
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return tp.tv_sec*1000 + tp.tv_usec/1000;
}

// Recebe um pacote esperando por determinado intervalo. Coloca o pacote lido em pkt
// se ele for de acordo com o protocolo.
// Retorna RECV_ERR se der timeout
// Retorna OK c.c
int recv_with_timeout(struct connection_t *conn, int interval, struct packet_t *pkt) {
    socklen_t addr_len = sizeof(conn->addr);
    long long comeco = timestamp();
    vector<uint8_t> buf;
    struct timeval timeout = {
        .tv_sec = interval/1000,
        .tv_usec = (interval % 1000) * 1000
    };

    // Coloca timeout no socket, ver https://wiki.inf.ufpr.br/todt/doku.php?id=timeouts
    setsockopt(conn->socket, SOL_SOCKET, SO_RCVTIMEO, (char*) &timeout, sizeof(timeout));
    do {
        buf.resize(MAX_MSG_LEN, 0);

        auto msg_len = recvfrom(conn->socket, buf.data(), buf.size(), 0, (struct sockaddr*)&(conn->addr), &addr_len);
        // Se tiver dado timeout/outro erro apenas continua o loop
        if (msg_len < 0)
            continue;

        // Trunca para quantos bytes foram recebidos
        buf.resize(msg_len);
#ifdef DEBUG
        printf("%ld\n", msg_len);
        for (auto i : buf)
            printf("%x ", i);
        printf("\n");
        printf("OUTGGOING: %d\n", conn->addr.sll_pkttype == PACKET_OUTGOING);
#endif

        // Se o pacote for valido então retona Ok
        if (is_valid_packet(conn->addr, buf) && pkt->deserialize(buf))
            return OK;
    } while (timestamp() - comeco <= interval);

    return RECV_ERR;
}

// Envia um pacote e espera por uma resposta.
// Faz retransmissão de dados se tiver timeout, configurar PACKET_TIMEOUT e
// PACKET_RESTRANSMISSION.
// Retorna MSG_TO_BIG se menssagem for grande demais.
// Retorna SEND_ERR em caso de erro ao fazer send.
// Retorna TIMEOUT_ERR em caso de não recebimento de resposta.
// Retorna OK c.c.
int connection_t::send_await_packet(uint8_t tipo, std::vector<uint8_t> &msg, struct packet_t *r_pkt) {
    if (msg.size() > 63)
        return MSG_TO_BIG;

    struct packet_t s_pkt;
    int i;
    s_pkt.tam = (uint8_t)(msg.size());
    // Coloca sequência do pacote a ser enviado
    s_pkt.seq = seq;
    s_pkt.tipo = tipo;
    s_pkt.dados.resize(msg.size());
    copy(msg.begin(), msg.end(), s_pkt.dados.begin());
    vector<uint8_t> s_buf = s_pkt.serialize();
    int interval = PACKET_TIMEOUT_INTERVAL;

    for (i = 0; i < PACKET_RETRASMISSION_ROUNDS; i++) {
#ifdef DEBUG
        printf("[DEBUG]: Tentativa (%d\\%d)\n", i+1, PACKET_RETRASMISSION_ROUNDS);
#endif
        // Faz o send
        if (send(socket, s_buf.data(), s_buf.size(), 0) < 0)
            return SEND_ERR;
#ifdef DEBUG
        printf("[DEBUG]: Menssagem enviada com sucesso\n");
        printf("[DEBUG]: "); print_packet(&s_pkt);
#endif
        
        if (recv_with_timeout(this, interval, r_pkt) >= 0)
            break;
    }
    // Se alcançou o maximo de retransmissões
    if (i == PACKET_RETRASMISSION_ROUNDS)
        return TIMEOUT_ERR;

    // Atualiza sequência
    seq = (seq + 1) % (1<<SEQ_SIZE);
    return OK;
}

// ===================== Connection =====================
