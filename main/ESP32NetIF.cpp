#include "ESP32NetIF.h"

const char *ESP32NetIF::logTag = "NetIF";
void (*ESP32NetIF::handler)(int client, char* data, unsigned int len) = 0;

ESP32NetIF::ESP32NetIF(unsigned int port, void (*requestsHandler)(int client, char* data, unsigned int len)){
    this->port = port;
    this->handler = requestsHandler;
}

void ESP32NetIF::handleWS(int client, char* data, unsigned int len, std::string (*handler)(std::string dt, void* param), void* param4handler){
    int i;
    std::string qqk(data,len);
    std::string tmp;
    int start = qqk.find("Sec-WebSocket-Key");
    if (start != std::string::npos){
        int end = qqk.find("\r\n", start);
        if (end != std::string::npos){
            start+=19;
            tmp = qqk.substr(start,(end-start));
            tmp += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

            char* shaRes = (char*) malloc(20);
            esp_sha(SHA1, (const unsigned char*)tmp.c_str(), tmp.size(), (unsigned char*) shaRes);

            mbedtls_base64_encode(NULL, 0, (size_t*) &i, (const unsigned char*)shaRes, 20);
            char* bbuf = (char*) malloc(i);
            mbedtls_base64_encode((unsigned char *)bbuf, i, (size_t*) &i, (const unsigned char*)shaRes, 20);
            
			free(shaRes);

            send(client, HEADERS_SW_PROTOCOLS_101, HEADERS_SW_PROTOCOLS_101_LEN, 0);
			send(client, bbuf, i, 0);
			send(client, "\r\n\r\n", 4, 0);

            free(bbuf);
            
            const unsigned int LN_LIMIT = 1023;
            uint8_t maskOffset = 2;
            uint64_t payloadLen = 0;

            char *wsBF = (char*) malloc(1024);
            int valread;
            do{
                memset(wsBF,0,1024);
                valread = read(client , wsBF, 1024);
                if (valread <= 0){
                    break;
                }
                else{
                    tmp = ""; //Reset
                    /*
                        THIS SUPPORTS SINGLE FRAMES ONLY (0x8x)
                    */
                    //if (wsBF[0] == 0x80){} //continuation frame not supported
                    /*else*/ if (wsBF[0] == 0x81){ // text frame
                        //printf("got text");
                        if (wsBF[1] == 0xFE){
                            payloadLen = (uint64_t) ((wsBF[2] <<8) | wsBF[3]);
                            maskOffset = 4;
                        }
                        else if (wsBF[1] == 0xFF){
                            for (int b = 2; b < 8; b++){
                                payloadLen = (uint64_t)(payloadLen << 8) | wsBF[b];
                            }
                            maskOffset = 10;
                        }
                        else{
                            payloadLen = (uint64_t) wsBF[1] & ~0x80;
                            maskOffset = 2;
                        }

                        if (payloadLen > LN_LIMIT){
                            continue;
                        }
                        for (int ww=0; ww<payloadLen; ww++){
                            tmp += (unsigned char) (wsBF[(maskOffset+4) + ww] ^ wsBF[(ww%4)+maskOffset]);
                        }
                        std::string f = handler(tmp, param4handler);
                        uint64_t ln = f.size();
                        std::string res;
                        res+=(char)0x81;

                        if (ln <= 125){
                            res+=(char)f.size(); //TODO: masking for websocket Server -> Client
                            res+=f;
                            send(client, res.c_str(), res.size(), 0);
                        }
                        else if (ln <= 65535){
                            res+=(char)0x7E;
                            res+=(char)((ln>>8) & 0xFF);
                            res+=(char)(ln & 0xFF);
                            res+=f;
                            send(client, res.c_str(), res.size(), 0);
                        }
                        else{
                            res+=(char)0x7F;
                            for (int b = 4; b >= 0; b--){
                                res += (char)((ln >> 8*b) & 0xFF);
                            }
                            res+=f;
                            send(client, res.c_str(), res.size(), 0);
                        }
                    }
                    //else if (wsBF[0] == 0x82){} //binary frame
                    else if (wsBF[0] == 0x88){ // close frame
                        break;
                    }
                    else if (wsBF[0] == 0x89){ // ping
                        //printf("got ping\n");
                        //Send Pong
                        std::string res;
                        res+=(char)0x8A;
                        res+=(char)0; //TODO: masking for websocket Server -> Client
                        send(client, res.c_str(), res.size(), 0);
                    }
                    //else if (wsBF[0] == 0x0A){} // pong
                    // else{} -> Do nothing
                }
            }while (1);
            free(wsBF);
			close(client);
        }
    }
    else{
        send(client, PAGE_BAD_REQUEST_400, PAGE_BAD_REQUEST_400_LEN, 0);
        return;
    }
}

void ESP32NetIF::handleClient(void* cli){
    int client = *(int*)cli;
    char *bf = (char*) malloc(1024);
    int valread;
    do{
        memset(bf,0,1024);
        valread = read(client , bf, 1024);
        if (valread <= 0){
            ESP32NetIF::handler(client, bf, 0);
            break;
        }
        ESP32NetIF::handler(client, bf, valread);
    }while (1);
    free(bf);
    close(client);
    vTaskDelete(NULL);
}

void ESP32NetIF::serveTask(void* port){
    unsigned int pport = *(unsigned int*)port;
    while (1){
        int socketfd = socket(AF_INET, SOCK_STREAM, 0);
        if (socketfd == 0){
            continue;
        }
		int enable = 1;
        setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(pport);
        int addrlen = sizeof(address);

        if (bind(socketfd, (struct sockaddr *)&address, sizeof(address)) < 0){
			close(socketfd);
			continue;
        }

        if (listen(socketfd, 350) < 0){
            continue;
        }

        while (1){
            int client = accept(socketfd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
            if (client == 0){
				close(client);
                continue;
            }
            else if (client < 0){
				close(client);
                break;
            }
            else{
                ESP_LOGI(ESP32NetIF::logTag, "New client connected.");
                xTaskCreate(ESP32NetIF::handleClient,"handleClientTask", 4096, &client, 5,NULL);
            }
        }
        close(socketfd);
    }
}

void ESP32NetIF::serveForever(){
    xTaskCreate(ESP32NetIF::serveTask,"serveTask", 4096, &this->port, 5,NULL);
}