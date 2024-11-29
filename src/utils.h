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
#include <filesystem>

#define BYTES1K 1024

bool get_file_name(std::vector<uint8_t> &name);
int calculate_cksum(std::filesystem::path file_path, std::vector<uint8_t> *umsg);
bool has_disc_space(struct packet_t *pkt);
std::vector<uint8_t> get_file_size(struct packet_t *pkt);
int get_file_size(std::string filename, size_t *size);
std::vector<uint8_t> size_t_to_uint8_t(size_t v);
size_t uint8_t_to_size_t(std::vector<uint8_t> buf);
const char *erro_to_str(int tipo);
const char *tipo_to_str(int);
void print_packet(struct packet_t *pkt);
int get_interfaces(std::vector<std::string> &);
