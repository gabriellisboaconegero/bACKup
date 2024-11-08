#pragma once
#include <iostream>
#include <cstring>
#include <set>
#include <vector>
#include <ifaddrs.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>

const char *erro_to_str(int tipo);
const char *tipo_to_str(int);
void print_packet(struct packet_t *pkt);
int get_interfaces(std::vector<std::string> &);
