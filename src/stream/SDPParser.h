//
// Created by ME on 2023/12/12.
//

#ifndef RAVENNAPI_SDPPARSER_H
#define RAVENNAPI_SDPPARSER_H

#include <string>
#include <map>
#include "../ConfigData.h"

using std::string;
using std::map;

class SDPParser {
public:
    static stream_info parse(const string &sdpData);
private:
    SDPParser() = default;
};

#endif //RAVENNAPI_SDPPARSER_H
