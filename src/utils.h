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

template <typename T> std::vector<uint8_t> to_uint8_t(T v) {
    std::vector<uint8_t> buf(sizeof(T)/sizeof(uint8_t), 0);
    for (int i = 0; i < int(buf.size()); i++) {
        buf[i] = (uint8_t)(v & ((1<<(sizeof(uint8_t)*8)) - 1));
        v >>= (sizeof(uint8_t)*8);
    }
    return buf;
}

template <typename T> T uint8_t_to(std::vector<uint8_t> buf) {
    T res = 0;
    for (int i = int(sizeof(T))-1; i > 0; i--) {
        res |= (T)(buf[i]);
        res <<= (sizeof(uint8_t)*8);
    }
    res |= (T)(buf[0]);
    return res;
}


bool get_file_name(std::vector<uint8_t> &name);
int calculate_cksum(std::filesystem::path file_path, uint *cksum);
const char *erro_to_str(int tipo);
const char *tipo_to_str(int);
void print_packet(struct packet_t *pkt);
int get_interfaces(std::vector<std::string> &);
