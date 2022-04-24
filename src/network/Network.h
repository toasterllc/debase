#pragma once

#if __APPLE__
#include "lib/libcurl/build-mac/include/curl.h"
#elif __linux__
#include "lib/libcurl/build-linux/include/curl.h"
#endif

#include "lib/toastbox/Defer.h"
#include "lib/toastbox/RuntimeError.h"
#include "lib/nlohmann/json.h"

namespace Network {

template <typename T_Req, typename T_Resp>
inline void Request(const char* url, const T_Req& req, T_Resp& resp) {
    CURL* curl = curl_easy_init();
    if (!curl) throw Toastbox::RuntimeError("curl_easy_init failed");
    Defer(curl_easy_cleanup(curl));
    
    // Set URL
    curl_easy_setopt(curl, CURLOPT_URL, url);
    
    // Set headers
    struct curl_slist* headers = nullptr;
    Defer(curl_slist_free_all(headers));
    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    
    if constexpr (!std::is_null_pointer_v<T_Req>) {
        // Encode request as json and put it in the request
        // Curl doesn't copy the data, so it needs to stay live until curl is complete!
        nlohmann::json j = req;
        std::string jstr = j.dump();
        curl_easy_setopt(curl, CURLOPT_COPYPOSTFIELDS, jstr.c_str());
    }
    
    std::stringstream respStream;
    auto respWriter = +[] (char* data, size_t _, size_t len, std::stringstream& respStream) -> size_t {
        respStream.write(data, len);
        return len;
    };
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &respStream);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, respWriter);
    
    CURLcode cc = curl_easy_perform(curl);
    if (cc != CURLE_OK) throw Toastbox::RuntimeError("curl_easy_perform failed: %s", curl_easy_strerror(cc));
    
    long httpRespCode = 0;
    cc = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpRespCode);
    if (cc != CURLE_OK) throw Toastbox::RuntimeError("curl_easy_getinfo failed: %s", curl_easy_strerror(cc));
    if (httpRespCode != 200) throw Toastbox::RuntimeError("request failed with HTTP response code %jd", (intmax_t)httpRespCode);
    
    // Decode response
    {
        nlohmann::json j = nlohmann::json::parse(respStream.str());
        j.get_to(resp);
    }
    
//    sleep(7);
    
//    for (;;) sleep(10);
}

} // namespace Network
