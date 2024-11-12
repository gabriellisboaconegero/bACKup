#include "socket.h"
#include "utils.h"
using namespace std;

// ler os seguintes manuais para entender melhor o que essa função faz
//      man packet
//      man socket
//      man setsockopt
int cria_raw_socket(const char* nome_interface_rede) {
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

// Gera o crc 8 bits de uma certa sequência de bytes.
// vide https://www.sunshine2k.de/articles/coding/crc/understanding_crc.html 
uint8_t crc8(vector<uint8_t>::iterator buf_begin,
                 vector<uint8_t>::iterator buf_end)
{
    uint8_t crc = 0;
    for (; buf_begin != buf_end; ++buf_begin) {
        crc ^= *buf_begin;
        for (int j = 0; j < 8; j++){
            if (crc & 0x80)
                crc = (crc << 1) ^ GENERATOR_POLY;
            else
                crc <<= 1;
        }
    }
    return crc;
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

    // Quer dizer que não tem a quantidade correta de bytes enviados em dados
    if (int(buf.size()) < PACKET_MIN_SIZE)
        return false;

    // Verifica crc 8 bits dos campos tam, seq, tipo, dados e crc.
    // Para fazer a verificação basta verificar se crc8 aplicado nos
    // campos tam, tipo, seq, dados e crc são 0.
    // Vide https://www.sunshine2k.de/articles/coding/crc/understanding_crc.html,
    // na seção 2.1 CRC Verification.
    if(crc8(buf.begin()+1, buf.begin()+buf.size()) != 0){
#ifdef DEBUG
        printf("[ERRO]: CRC incorreto\n");
#endif
        this->tipo = PKT_UNKNOW;
        return false;
    }

    // Pega tamanho
    this->tam = buf[1] >> 2;
    // Pega sequencia
    this->seq = ((buf[1] & 0x03) << 3) | (buf[2] >> 5);
    // Pega tipo
    this->tipo = buf[2] & 0x1f;

    // Verifica se dados estão no tamanho correto
    if (int(buf.size()) - PACKET_MIN_SIZE != int(this->tam))
        return false;

    // Copia os dados da menssagem, que começam depois de mi, tam, seq e tipo e acabam antes de crc
    this->dados.resize(tam, 0);
    copy(buf.begin()+3, buf.begin()+3+this->tam, this->dados.begin());

    return true;
}

// Serializa os dados de um packet em um buffer.
vector<uint8_t> packet_t::serialize() {
    // Os valores do buffer vão ficar (em binário de 1 byte)
    // buf[0] = 0bmmmmmmmm
    // buf[1] = 0bttttttss
    // buf[2] = 0bsssTTTTT
    vector<uint8_t> buf(PACKET_MIN_SIZE+this->dados.size(), 0);

    // Marcador de inicio (m)
    buf[0] = PACKET_MI;

    // Tamanho (t)
    buf[1] = this->tam << 2;

    // Sequencia (s)
    buf[1] |= (this->seq & 0x18) >> 3; // 00011000
    buf[2]  = (this->seq & 0x07) << 5; // 00000111

    // Tipo (T)
    buf[2] |= this->tipo & 0x1f;       // 00011111

    // Coloca dados no buffer, dado offset de 3 bytes
    copy(this->dados.begin(), this->dados.end(), buf.begin()+3);
    // gera crc 8 bits
    
    // Crc gerado utiliza campos tam, tipo, seq e dados.
    buf[buf.size() - 1] = crc8(buf.begin()+1, buf.begin()+buf.size()-1);

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
#ifndef DOCKER_TEST
    if (addr.sll_pkttype == PACKET_OUTGOING)
        return false;
#endif

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
bool connection_t::connect(const char *interface) {
    this->socket = cria_raw_socket(interface);
    this->seq = 0;
    this->first_pkt = true;
    if (this->socket < 0)
        return false;
    return true;
}

// Limpa conexão, ou seja limpa buffer de leitura e escrita
// para não receber demais.
// Retorna false em caso de erro
// Retorna true c.c.
bool connection_t::reset_connection(const char *interface) {
    // Faz loop não bloqueante para retirar menssagens
    char buf[1024];
    while (recv(this->socket, buf, sizeof(buf), MSG_DONTWAIT) > 0);
#ifdef DEBUG
    printf("[DEBUG]: Limpando conexão com busy recv: %s\n", strerror(errno));
#endif
    // São esses os erros se tiver vazio o buffer
    return errno == EAGAIN || errno == EWOULDBLOCK;
}

long long timestamp() {
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return tp.tv_sec*1000 + tp.tv_usec/1000;
}

// Espera por um pacote dado um certo intervalo. Se intervalo for 0
// então não existe timeout.
// Retorna RECV_ERR (-1) se houver algum erro durante execução.
// Retorna PKT_TIMEOUT se tiver ocorrido o timeout
// Retorn o tipo do pacote c.c.
int connection_t::recv_packet(int interval, struct packet_t *pkt) {
    socklen_t addr_len = sizeof(this->addr);
    long long comeco = timestamp();
    vector<uint8_t> buf;
    struct timeval timeout;
    timeout.tv_sec = interval/1000;
    timeout.tv_usec = (interval % 1000) * 1000;

    // Coloca timeout no socket, ver https://wiki.inf.ufpr.br/todt/doku.php?id=timeouts
    if (setsockopt(this->socket, SOL_SOCKET, SO_RCVTIMEO, (char*) &timeout, sizeof(timeout)) < 0)
        return RECV_ERR;

    do {
        buf.resize(MAX_MSG_LEN, 0);

        auto msg_len = recvfrom(this->socket, buf.data(), buf.size(), 0, (struct sockaddr*)&(this->addr), &addr_len);
        // Se tiver dado timeout continua o loop.
        // Se tiver dado algum erro, retorna que deu erro.
        if (msg_len < 0) {
            // Veja sobre SO_RECVTIMEO em
            //      man 7 socket
            // Mas a ideia é que se der timeout esse são os possíveis erros
            // que são retornados no timeout
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINPROGRESS)
                continue;
            return RECV_ERR;
        }

        // Trunca para quantos bytes foram recebidos
        buf.resize(msg_len);

#ifdef DEBUG2
        printf("%ld\n", msg_len);
        for (auto i : buf)
            printf("%x ", i);
        printf("\n");
        printf("OUTGGOING: %d\n", conn->addr.sll_pkttype == PACKET_OUTGOING);
#endif

        // Se o pacote for valido então retona Ok
        if (!is_valid_packet(this->addr, buf))
            continue;
        if (!pkt->deserialize(buf))
            return PKT_UNKNOW;
        return pkt->tipo;
    // Continua o loop apenas se não tiver dado timeout ou
    // intervalo de timeout <= 0, ou seja continua para sempre.
    } while (interval <= 0 || timestamp() - comeco <= interval);

    return PKT_TIMEOUT;
}

// Envia um packet.
// Retorna MSG_TO_BIG caso a menssagem tenha mais de 63 bytes
// Retorna SEND_ERR em caso de erro ao fazer send
// Retorna OK c.c
int connection_t::send_packet(struct packet_t *pkt, int save) {
    vector<uint8_t> buf = pkt->serialize();
    // DEBUG: Altera um bit para verificar se CRC funciona
    // buf[3] ^= 0x80;
    // Testa corrupção de OK_TAM e NACK
    // if (pkt->tipo == PKT_OK_TAM || pkt->tipo == PKT_NACK)
    //     if (rand() % 10 < 5)
    //         buf[3] = '*';

    // Faz o send
    if (send(this->socket, buf.data(), buf.size(), 0) < 0)
        return SEND_ERR;

    if (save)
        this->save_last_send(pkt);

    return OK;
}

struct packet_t connection_t::make_packet(int tipo, vector<uint8_t> &umsg) {
    struct packet_t pkt;
    // Trunca menssagem, problema do usuário se ele mandar
    // dados muito grande.
    if (umsg.size() > 63)
        pkt.dados.resize(63);
    else
        pkt.dados.resize(umsg.size());

    pkt.tam = (uint8_t)(umsg.size());
    // Coloca sequência do pacote a ser enviado
    pkt.seq = this->seq;
    pkt.tipo = tipo;
    copy_n(umsg.begin(), pkt.dados.size(), pkt.dados.begin());

    return pkt;
}

int connection_t::send_erro(uint8_t erro_id, int save) {
    vector<uint8_t> umsg(14, 0);
    umsg[0] = erro_id;
    struct packet_t pkt = make_packet(PKT_ERRO, umsg);
    return this->send_packet(&pkt, save);
}

int connection_t::send_nack() {
    vector<uint8_t> umsg(14, 0);
    struct packet_t pkt = make_packet(PKT_NACK, umsg);
    return this->send_packet(&pkt);
}

int connection_t::send_ack(int save) {
    vector<uint8_t> umsg(14, 0);
    struct packet_t pkt = make_packet(PKT_ACK, umsg);
    return this->send_packet(&pkt, save);
}

int connection_t::send_ok(int save) {
    vector<uint8_t> umsg(14, 0);
    struct packet_t pkt = make_packet(PKT_OK, umsg);
    return this->send_packet(&pkt, save);
}

void connection_t::save_last_recv(struct packet_t *pkt) {
    // Guarda ultimo pacote enviado
    this->last_pkt_recv.tipo = pkt->tipo;
    this->last_pkt_recv.tam = pkt->tam;
    this->last_pkt_recv.seq = pkt->seq;
    this->last_pkt_recv.dados.resize(pkt->tam);
    copy(pkt->dados.begin(), pkt->dados.end(), this->last_pkt_recv.dados.begin());
    this->first_pkt = false;
}

void connection_t::save_last_send(struct packet_t *pkt) {
    // Guarda ultimo pacote enviado
    this->last_pkt_send.tipo = pkt->tipo;
    this->last_pkt_send.tam = pkt->tam;
    this->last_pkt_send.seq = pkt->seq;
    this->last_pkt_send.dados.resize(pkt->tam);
    copy(pkt->dados.begin(), pkt->dados.end(), this->last_pkt_send.dados.begin());
    this->update_seq();
}

void connection_t::update_seq() {
    // Atualiza sequência apenas se tudo der certo
    this->seq = SEQ_MOD(this->seq + 1);
}

// Envia um pacote e espera por uma resposta. O pacote recebido é processado
// pela função process_buf, onde ela decide quais ações tomar com o buffer
// que foi recebido. É garantido que o buffer vai ter o marcador de inicio
// do protocolo.
// Atualiza seq da connection apenas se retorno de process_buf for OK
// Faz retransmissão de dados se tiver timeout, configurar PACKET_TIMEOUT e
// PACKET_RESTRANSMISSION.
// Retorna MSG_TO_BIG se menssagem for grande demais.
// Retorna SEND_ERR em caso de erro ao fazer send.
// Retorna RECV_TIMEOUT em caso de não recebimento de resposta.
// Retorna valor da função de parametro c.c.
// int connection_t::send_await_packet(
//         uint8_t tipo, std::vector<uint8_t> &msg,
//         struct packet_t *r_pkt, function<int(struct packet_t *, vector<uint8_t> &)> process_buf)
// {
//     if (msg.size() > 63)
//         return MSG_TO_BIG;
// 
//     struct packet_t s_pkt;
//     int i, res = OK;
//     s_pkt.tam = (uint8_t)(msg.size());
//     // Coloca sequência do pacote a ser enviado
//     s_pkt.seq = this->seq;
//     s_pkt.tipo = tipo;
//     s_pkt.dados.resize(msg.size());
//     copy(msg.begin(), msg.end(), s_pkt.dados.begin());
//     vector<uint8_t> s_buf = s_pkt.serialize();
//     int interval = PACKET_TIMEOUT_INTERVAL;
// 
//     for (i = 0; i < PACKET_RETRASMISSION_ROUNDS; i++) {
// #ifdef DEBUG
//         printf("[DEBUG]: Tentativa (%d\\%d)\n", i+1, PACKET_RETRASMISSION_ROUNDS);
// #endif
//         // Faz o send
//         if (send(this->socket, s_buf.data(), s_buf.size(), 0) < 0)
//             return SEND_ERR;
// #ifdef DEBUG
//         printf("[DEBUG]: Menssagem enviada com sucesso\n");
//         printf("[DEBUG]: "); print_packet(&s_pkt);
// #endif
//         
//         // Tenta receber packet com timeout definido. Se receber timeout
//         res = this->recv_packet(interval, r_pkt, process_buf);
//         if (res == RECV_TIMEOUT)
//             continue;
//         if (res == RECV_ERR)
//             return RECV_ERR;
//         break;
//     }
//     // Se alcançou o maximo de retransmissões
//     if (i == PACKET_RETRASMISSION_ROUNDS)
//         return RECV_TIMEOUT;
// 
//     // Atualiza sequência apenas se retorno de process_buf for OK
//     if (res == OK)
//         this->seq = (this->seq + 1) % (1<<SEQ_SIZE);
//     return res;
// }

// ===================== Connection =====================
