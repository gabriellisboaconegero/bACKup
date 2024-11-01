#include "utils.h"
using namespace std;

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

