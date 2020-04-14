//
// Created by henning on 4/14/20.
//

#ifndef HW4_STRINGMANIPULATION_H
#define HW4_STRINGMANIPULATION_H

#include <string>
#include <vector>
#include <algorithm>
#include <iostream>

std::string parse_ip_and_port(std::string peer_formatted_string) {
    // the peer format is ipv4:xxx.xxx.xxx.xxx:port. So, strip ipv4, then save the rest.
    size_t pos = 0;
    std::string ip_address, port;
    std::string delimiter = ":";

    // strip ipvX
    pos = peer_formatted_string.find(delimiter);
    peer_formatted_string.erase(0, pos + delimiter.length());

    return peer_formatted_string;
}

std::string up_case_str(const std::string& str) {
    std::string str_copy = str;
    std::transform(str_copy.begin(), str_copy.end(), str_copy.begin(), ::toupper);
    return str_copy;
}

std::vector<std::string> split(const std::string& str) {
    std::istringstream iss(str);
    std::vector<std::string> results((std::istream_iterator<std::string>(iss)),
                                     std::istream_iterator<std::string>());
    return results;
}

#endif //HW4_STRINGMANIPULATION_H
