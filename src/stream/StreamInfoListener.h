//
// Created by ME on 2023/12/11.
//

#ifndef RAVENNAPI_STREAMMAIN_H
#define RAVENNAPI_STREAMMAIN_H

#include "../pch.h"
#include "../Log.h"
#include "../ConfigData.h"
#include "SDPParser.h"
#include "StreamRecorder.h"

extern char zeroconf_ip_address[NI_MAXHOST];

void setup_sdp_socket();

int sdp_listener_main();

#endif //RAVENNAPI_STREAMMAIN_H
