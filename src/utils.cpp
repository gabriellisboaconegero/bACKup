#include "socket.h"
#include "utils.h"
using namespace std;

const char *erro_to_str(int tipo) {
    switch (tipo) {
        case NO_FILE_ERRO:          return "Arquivo inexistente"; break;
        case NO_FILE_ACCESS_ERRO:   return "Arquivo com acesso negado"; break;
        case NO_DISK_SPACE_ERRO:    return "Sem espaço em disco para tamanho colicitado"; break;
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
    for (int i = 0; i < int(pkt->dados.size()); i++)
        cout << pkt->dados[i];
    cout << endl;
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

