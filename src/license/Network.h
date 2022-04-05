#pragma once
#include "lib/libcurl/include/curl/curl.h"

namespace License {

void TrialStart() {
    CURL* curl = nullptr;
    CURLcode res = CURLE_OK;
    
    curl_global_init(CURL_GLOBAL_DEFAULT);
    
    
    
}

} // namespace License
