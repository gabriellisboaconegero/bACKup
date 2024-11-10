#pragma once
#include <iostream>
#include <cstring>
#include <set>
#include <vector>
#include <ifaddrs.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <cstdint>


bool get_file_name(std::vector<uint8_t> &name);
std::vector<uint8_t> calculate_cksum();
bool has_disc_space(struct packet_t *pkt);
std::vector<uint8_t> get_file_size(struct packet_t *pkt);
const char *erro_to_str(int tipo);
const char *tipo_to_str(int);
void print_packet(struct packet_t *pkt);
int get_interfaces(std::vector<std::string> &);
