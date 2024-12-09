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

    // Verifica crc 8 bits dos campos tam, seq, tipo, dados e crc.
    // Para fazer a verificação basta verificar se crc8 aplicado nos
    // campos tam, tipo, seq, dados e crc são 0.
    // Vide https://www.sunshine2k.de/articles/coding/crc/understanding_crc.html,
    // na seção 2.1 CRC Verification.
    if(crc8(buf.begin()+1, buf.begin()+buf.size()) != 0){
        printf("[WARNING]: CRC incorreto\n");
        this->tipo = PKT_UNKNOW;
        return false;
    }

    // Pega tamanho
    this->tam = buf[1] >> 2;
    // Pega sequencia
    this->seq = ((buf[1] & 0x03) << 3) | (buf[2] >> 5);
    // Pega tipo
    this->tipo = buf[2] & 0x1f;

    // Copia os dados da menssagem, que começam depois de mi, tam, seq e tipo e acabam antes de crc
    this->dados.resize(PACKET_MAX_DADOS_SIZE, 0);
    // copy(buf.begin()+3, buf.begin()+3+PACKET_MAX_DADOS_SIZE, this->dados.begin());
    auto it = buf.begin() + 3; // Início dos dados no buffer
    auto end = buf.end() - 1;  // Exclui o CRC do final

    for (int i = 0; i < int(this->dados.size()); i++) {
        uint8_t byte = *it++;
        this->dados[i] = byte;
        // Se encontrar um byte 0x88 ou 0x81, pule o próximo byte (0xFF) se existir
        if ((byte == 0x88 || byte == 0x81) && it != end && *it == 0xFF) {
            ++it; // Ignora o byte 0xFF
        }
    }
    return true;
}

// Serializa os dados de um packet em um buffer.
vector<uint8_t> packet_t::serialize() {
    // Os valores do buffer vão ficar (em binário de 1 byte)
    // buf[0] = 0bmmmmmmmm
    // buf[1] = 0bttttttss
    // buf[2] = 0bsssTTTTT
    vector<uint8_t> buf;

    // Marcador de inicio (m)
    buf.push_back(PACKET_MI);

    // Tamanho (t)
    buf.push_back(this->tam << 2);

    // Sequencia (s)
    buf[1] |= (this->seq & 0x18) >> 3; // 00011000
    buf.push_back((this->seq & 0x07) << 5); // 00000111

    // Tipo (T)
    buf[2] |= this->tipo & 0x1f;       // 00011111

    // Coloca dados no buffer, dado offset de 3 bytes
    // copy(this->dados.begin(), this->dados.end(), buf.begin()+3);
    for (int i = 0; i < int(this->dados.size()); i++) {
        uint8_t byte = this->dados[i];
        buf.push_back(byte);
        // Verifica se o byte é 0x88 ou 0x81 e insere 0xFF após ele
        if (byte == 0x88 || byte == 0x81) {
            buf.push_back(0xff);
        }
    }
    // gera crc 8 bits
    
    // Crc gerado utiliza campos tam, tipo, seq e dados.
    buf.push_back(crc8(buf.begin()+1, buf.begin()+buf.size()-1));

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
#else
    (void)(addr);
#endif

    // Verifica Marcador de inicio
    if (buf[0] != PACKET_MI)
        return false;

    return true;
}
// ===================== Packet =====================

// ===================== Connection =====================
#ifdef SIMULATE_UNSTABLE
void connection_t::reset_count() {
    this->packet_lost_count = 0;
    this->packet_currupted_count = 0;
}

void connection_t::print_count() {
    printf("[SIMULATION]: Nº Pacotes Peridos: %ld\n", this->packet_lost_count);
    printf("[SIMULATION]: Nº Pacotes Corrompidos: %ld\n", this->packet_currupted_count);
}
#endif
// Cria raw socket de conexão e inicializa struct.
// Retorna false se não foi possivel criar socket.
// Retorn true c.c.
bool connection_t::connect(const char *interface) {
    this->socket = cria_raw_socket(interface);
    this->seq = 0;
    this->first_pkt = true;
    this->last_pkt_send.dados.clear();
    this->last_pkt_recv.dados.clear();
    this->last_pkt_send.dados.resize(PACKET_MAX_DADOS_SIZE, '\0');
    this->last_pkt_recv.dados.resize(PACKET_MAX_DADOS_SIZE, '\0');
    if (this->socket < 0)
        return false;
    return true;
}

// Limpa conexão, ou seja limpa buffer de leitura e escrita
// para não receber demais.
// Retorna false em caso de erro
// Retorna true c.c.
bool connection_t::reset_connection() {
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

#ifdef DEBUG3
        printf("%ld\n", msg_len);
        for (auto i : buf)
            printf("%x ", i);
        printf("\n");
        printf("OUTGGOING: %d\n", this->addr.sll_pkttype == PACKET_OUTGOING);
#endif

#ifdef SIMULATE_UNSTABLE
        if (rand() % 1000 < LOST_PACKET_CHANCE) {
            printf("[SIMULATION]: Pacote perdido\n");
            this->packet_lost_count++;
            continue;
        }
        if (rand() % 1000 < CURRUPTED_PACKET_CHANCE) {
            printf("[SIMULATION]: Pacote corrompido\n");
            this->packet_currupted_count++;
            buf[2]++;
        }
#endif
        // Se o pacote for valido então retona Ok
        if (!is_valid_packet(this->addr, buf))
            continue;
        if (!pkt->deserialize(buf)) {
#if DEBUG2
            printf("[DEBUG]: Recebido (tipo: UNKNOWN)\n");
#endif
            return PKT_UNKNOW;
        }
#ifdef DEBUG2
        printf("[DEBUG]: Recebido: "); print_packet(pkt);
#endif
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

#ifdef DEBUG3
        printf("%ld\n", buf.size());
        for (auto i : buf)
            printf("%x ", i);
        printf("\n");
#endif
    // Faz o send
    if (send(this->socket, buf.data(), buf.size(), 0) < 0)
        return SEND_ERR;
#ifdef DEBUG2
    printf("[DEBUG]: Enviado: ");print_packet(pkt);
#endif

    if (save)
        this->save_last_send(pkt);

    return OK;
}

// Envia um paacote e espera outro em retorno. Para saber quais tipos de pacote de retorno
// usa os tipos no parametro 'esperados'. Se for algum dos pacotes, salva ele e o enviado.
// Salva pacote recebido em r_pkt.
// Retorna PKT_TIMEOUT se der timeout em todos o rounds.
// Retorna SEND_ERR ou RECV_ERR se houver erro no socket.
// Retorna tipo recebido em r_pkt c.c.
int connection_t::send_await_packet(
        struct packet_t *s_pkt, struct packet_t *r_pkt,
        vector<uint8_t> esperados, int interval, bool nacks)
{
    int round, res, temp;
    bool finished = false;
    int timeout1, timeout2;
    timeout1 = timeout2 = interval;
    // Itera o número de tentaivas para reconectar, aumenta tempo de timeout igual fibonacci
    // timeout[n] = timeout[n-1] + timeout[n-2]
    for (round = 0; round < PACKET_RETRASMISSION_ROUNDS && !finished; round++) {
#ifdef DEBUG
        printf("[DEBUG]: Tentativa (%d\\%d)\n", round+1, PACKET_RETRASMISSION_ROUNDS);
#endif
        // Se a flag nacks tiver habilitada faz o envio de nacks ao receber unknow.
        // Se receber pacote corrompido então envia nack,caso servidor
        // tenha mandado OK_TAM mas corrompeu.
        if (nacks && round != 0 && res == PKT_UNKNOW) {
            if (this->send_nack() < 0) {
                printf("[ERRO %s:%s:%d]: %s\n", __FILE__, __func__, __LINE__, strerror(errno));
                return SEND_ERR;
            }
        }else {
            if (this->send_packet(s_pkt) < 0) {
                printf("[ERRO %s:%s:%d]: %s\n", __FILE__, __func__, __LINE__, strerror(errno));
                return SEND_ERR;
            }
        }
        res = this->recv_packet(timeout2, r_pkt);
        // Se deu algum erro, não quer dizer que packet é do
        // tipo PKT_ERRO ou PKT_NACK
        if (res < 0) {
            printf("[ERRO %s:%s:%d]: %s\n", __FILE__, __func__, __LINE__, strerror(errno));
            return RECV_ERR;
        }
        // Aumenta timeout seguindo fibonacci
        temp = timeout1;
        timeout1 = timeout2;
        timeout2 += temp;

        // Se for algum esperado sai do laço, guardando o ultimo
        // pacote salvo e recebido.
        for (auto es : esperados) {
            if (res == es) {
                this->save_last_recv(r_pkt);
                this->save_last_send(s_pkt);
                finished = true;
                break;
            }
        }
    }
    // Se alcançou o maximo de retransmissões, marca que teve timeout
    if (round == PACKET_RETRASMISSION_ROUNDS)
        return PKT_TIMEOUT;

    return res;
}

struct packet_t connection_t::make_packet(int tipo, vector<uint8_t> umsg) {
    struct packet_t pkt;
    // Trunca menssagem para 63 bytes sempre, problema do usuário se ele mandar
    // dados muito grande.
    pkt.dados.clear();
    pkt.dados.resize(PACKET_MAX_DADOS_SIZE, '\0');

    pkt.tam = min((uint8_t)(PACKET_MAX_DADOS_SIZE), (uint8_t)(umsg.size()));
    // Coloca sequência do pacote a ser enviado
    pkt.seq = this->seq;
    pkt.tipo = tipo;
    // Copia no maximo o tamanho do buffer de dados
    copy_n(umsg.begin(), pkt.tam, pkt.dados.begin());

    return pkt;
}

struct packet_t connection_t::make_packet(int tipo, string str) {
    vector<uint8_t> umsg;
    umsg.resize(str.size());
    copy(str.begin(), str.end(), umsg.begin());
    return this->make_packet(tipo, umsg);
}

int connection_t::send_erro(uint8_t erro_id, int save) {
    vector<uint8_t> umsg(1, 0);
    umsg[0] = erro_id;
    struct packet_t pkt = make_packet(PKT_ERRO, umsg);
    return this->send_packet(&pkt, save);
}

int connection_t::send_nack() {
    struct packet_t pkt = make_packet(PKT_NACK, "nack");
    return this->send_packet(&pkt);
}

int connection_t::send_ack(int save) {
    struct packet_t pkt = make_packet(PKT_ACK, "ack");
    return this->send_packet(&pkt, save);
}

int connection_t::send_ok(int save) {
    struct packet_t pkt = make_packet(PKT_OK, "ok");
    return this->send_packet(&pkt, save);
}

void connection_t::save_last_recv(struct packet_t *pkt) {
    // Guarda ultimo pacote enviado
    this->last_pkt_recv.tipo = pkt->tipo;
    this->last_pkt_recv.tam = pkt->tam;
    this->last_pkt_recv.seq = pkt->seq;
    copy(pkt->dados.begin(), pkt->dados.end(), this->last_pkt_recv.dados.begin());
    this->first_pkt = false;
}

void connection_t::save_last_send(struct packet_t *pkt) {
    // Guarda ultimo pacote enviado
    this->last_pkt_send.tipo = pkt->tipo;
    this->last_pkt_send.tam = pkt->tam;
    this->last_pkt_send.seq = pkt->seq;
    copy(pkt->dados.begin(), pkt->dados.end(), this->last_pkt_send.dados.begin());
    this->update_seq();
}

void connection_t::update_seq() {
    // Atualiza sequência apenas se tudo der certo
    this->seq = SEQ_MOD(this->seq + 1);
}

// ===================== Connection =====================
