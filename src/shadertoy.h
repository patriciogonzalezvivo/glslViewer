#include <fstream>
#include <iostream>
#include <string>

#include <json/json.h>
#include <curl/curl.h>

//write_data call back from CURLOPT_WRITEFUNCTION
//responsible to read and fill "stream" with the data.
size_t write_data(void *ptr, size_t _size, size_t _nmemb, void *_stream) {
    std::string data((const char*) ptr, (size_t) _size*_nmemb);
    *((std::stringstream*) _stream) << data;
    return _size*_nmemb;
}

std::string getURL(const std::string& _url) {
    CURL * curl = curl_easy_init();
    
    curl_easy_setopt(curl, CURLOPT_URL, _url.c_str());
    /* example.com is redirected, so we tell libcurl to follow redirection */
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1); //Prevent "longjmp causes uninitialized stack frame" bug
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "deflate");
    std::stringstream out;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out);
    /* Perform the request, res will get the return code */
    CURLcode res = curl_easy_perform(curl);
    /* Check for errors */
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));
    }
    return out.str();
}

std::string getShaderToy(const std::string &_shaderId){
    std::string tmp = getURL("https://www.shadertoy.com/api/v1/shaders/"+_shaderId+"?key=rt8tWH");
    
    Json::Value data;
    Json::Reader jsonReader;
    jsonReader.parse(tmp, data, false);
    
    std::string str = data["Shader"]["renderpass"][0]["code"].asString();
    
    return str;
}
