#pragma once
#include <iostream>
#include <cstring>
#include <set>
#include <vector>
#include <ifaddrs.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>

int get_interfaces(std::vector<std::string> &);
