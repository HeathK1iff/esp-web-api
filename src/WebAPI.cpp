#include "WebAPI.h"

#ifdef ESP8266
    #include <Updater.h>
    #include <WiFiUdp.h>
#elif ESP32
    #include <SPIFFS.h>
    #include <Update.h>
#endif

#define DEFAULT_MIME_TYPE "text/plain"
#define HTML_MIME_TYPE "text/html"

#ifndef DEFAULT_HTML_TAG_LIST_BUFFER_LENGTH
    #define DEFAULT_HTML_TAG_LIST_BUFFER_LENGTH 512
#endif

WebAPI::HandlerListItem::HandlerListItem(const char* baseUrl,  WebAPIHandler *handler){
    _child = NULL;

    memset(_baseUrl, '\0', URL_BASE_LENGTH);
    strcat(_baseUrl, "/api");
    
    if (baseUrl[0] != '/'){
        strcat(_baseUrl, "/");
    } 
    
    strcat(_baseUrl, baseUrl);
    int len = strlen(_baseUrl) - 1;

    if (_baseUrl[len] == '/'){
        _baseUrl[len] = '\0';
    }

    _handler = handler;
}


const char* WebAPI::HandlerListItem::getBaseUrl(){
    return _baseUrl;
}

const char* WebAPI::HandlerListItem::_getUrlWithTag(char* buf, const char* tag){
    strcat(buf, _baseUrl);
    strcat(buf, "/");
    strcat(buf, tag);
    return buf; 
}

void WebAPI::HandlerListItem::splitUrl(const char* url, char **baseUrl, char **tag){
    *baseUrl = (char*) url;
    char* propVal = strrchr(url, '/');            
    --propVal[0] = '\0';
    propVal++;
    *tag = propVal;
}

WebAPIHandler* WebAPI::HandlerListItem::getHandler(const char* baseUrl){
    if (strcmp(_baseUrl, baseUrl) == 0)
        return _handler;

    if (_child != NULL)
        return _child->getHandler(baseUrl);
    
    return NULL;
}

void WebAPI::HandlerListItem::setChild(HandlerListItem *child){
    _child = child;
} 

WebAPI::HandlerListItem *WebAPI::HandlerListItem::getLastChild(){
    if (_child != NULL)
        return _child->getLastChild();

    return this;
}

void WebAPI::_splitUrl(char* url, char **baseUrl, char **prop){
    *baseUrl = url;
    char* propVal = strrchr(url, '/');            
    --propVal[0] = '\0';
    propVal++;
    *prop = propVal;
}


void WebAPI::begin(){
    _server->on("/api/firmware.do", HTTP_GET, [&](){
        _server->send_P(200, PSTR("text/html"), "");
    });
    
    _server->on("/api/firmware.do", HTTP_POST, [&](){
        char result[20];
        memset(result, '\0', 20);
        strcat(result, (Update.hasError()) ? "FAIL" : "SUCCESS");
        if (Update.hasError()){
            strcat(result, ":");
            char buf[10];
            strcat(result, itoa(Update.getError(), buf, 10));
            _server->send(500, "text/plain", result);
        } else {
            _server->client().setNoDelay(true);
            _server->send(200, "text/plain", result);
            delay(100);
            ESP.restart();
            _server->client().stop();
        }
    }, [&](){
        HTTPUpload &upload = _server->upload();
        if (upload.status == UPLOAD_FILE_START)
        {
            #ifdef ESP8266
            WiFiUDP::stopAll();
            #endif
            _updateError = false;
            
            #ifdef ESP32
            if (upload.name == "fs") {
                if (!Update.begin(SPIFFS.totalBytes(), U_SPIFFS))
                    _updateError = true;
            } else {
            #endif
                uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
                if (!Update.begin(maxSketchSpace, U_FLASH))
                    _updateError = true;
            #ifdef ESP32
            }
            #endif
        } else if ((upload.status == UPLOAD_FILE_WRITE) && (!_updateError)){
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
                _updateError = true;
        } else if ((upload.status == UPLOAD_FILE_END) && (!_updateError)) {
            if (!Update.end(true))
                _updateError = true;
        } else if (upload.status == UPLOAD_FILE_ABORTED){
            Update.end();
        }
        delay(10);
    });
}

#ifdef ESP8266
WebAPI::WebAPI(ESP8266WebServer* server){
    _server = server;
    _root = NULL;
}

#elif ESP32
WebAPI::WebAPI(WebServer* server){
    _server = server;
    _root = NULL;
}
#endif

void WebAPI::addHandler(const char* baseUrl, WebAPIHandler *handler){
    HandlerListItem* item = new HandlerListItem(baseUrl, handler);

    if (_root != NULL){
        HandlerListItem *last = _root->getLastChild();
        last->setChild(item);
    } else {
        _root = item;
    }

    int tagIndex = 0;

    while (tagIndex < handler->getTagArray().Array_size){    
        char url_buf[URL_BASE_LENGTH];
        memset(url_buf, '\0', URL_BASE_LENGTH);
        const char* url = item->_getUrlWithTag(url_buf, handler->getTagArray().Array[tagIndex].Name);

        if ((handler->getTagArray().Array[tagIndex].Type == ttProperty) || (handler->getTagArray().Array[tagIndex].Type == ttReadOnlyProperty)){
                _server->on(url, HTTP_GET, [&](){          
                char* baseUrl;
                char* prop;  
                HandlerListItem::splitUrl(_server->uri().c_str(), &baseUrl, &prop);
                
                WebAPIHandler* handler = _root->getHandler(baseUrl);
                if (handler != NULL){
                    _server->send(200, DEFAULT_MIME_TYPE, handler->getValueByTag(prop));
                    return;
                }

                _server->send(500, DEFAULT_MIME_TYPE, "Handler not found");    
                return;    
            });
        }
        
        if ((handler->getTagArray().Array[tagIndex].Type == ttProperty) || (handler->getTagArray().Array[tagIndex].Type == ttWriteOnlyProperty)) {
            _server->on(url, HTTP_POST, [&](){
                char* baseUrl;
                char* prop;  
                HandlerListItem::splitUrl(_server->uri().c_str(), &baseUrl, &prop);

                WebAPIHandler* handler = _root->getHandler(baseUrl);
                if (handler != NULL){
                    if (handler->setValueByTag(prop, _server->arg("plain").c_str())){
                        _server->send(200, DEFAULT_MIME_TYPE, "success");
                        return;
                    }
                    
                    _server->send(500, DEFAULT_MIME_TYPE, handler->getLastError());
                    handler->resetError();
                    return;
                }

                _server->send(500, DEFAULT_MIME_TYPE, "Handler not found");      
            });
        };
        
        if (handler->getTagArray().Array[tagIndex].Type == ttMethod) {
            _server->on(url, HTTP_GET, [&](){
                char* baseUrl;
                char* method;  
                HandlerListItem::splitUrl(_server->uri().c_str(), &baseUrl, &method);

                WebAPIHandler* handler = _root->getHandler(baseUrl);
                if (handler != NULL){
                    if (handler->callMethodByTag(method)){
                        _server->send(200, DEFAULT_MIME_TYPE, "success");
                        return;
                    }
                    
                    _server->send(500, DEFAULT_MIME_TYPE, handler->getLastError());
                    handler->resetError();
                    return;
                }

                _server->send(500, DEFAULT_MIME_TYPE, "Handler not found");      
            });
        }

        tagIndex++;
    }

    _server->on(item->getBaseUrl(), HTTP_GET, [&](){   
            WebAPIHandler* handler = _root->getHandler(_server->uri().c_str());
            if (handler != NULL){                     
                char htmlBuf[DEFAULT_HTML_TAG_LIST_BUFFER_LENGTH];
                htmlBuf[0] = '\0';
                for (int i = 0; i < handler->getTagArray().Array_size; i++){
                    strcat(htmlBuf, handler->getTagArray().Array[i].Name);
                    strcat(htmlBuf, "@");
                    switch (handler->getTagArray().Array[i].Type)
                    {
                        case ttProperty:
                            strcat(htmlBuf, "readwrite");
                        break;
                        case ttReadOnlyProperty:
                            strcat(htmlBuf, "readonly");
                        break;
                        case ttWriteOnlyProperty:
                            strcat(htmlBuf, "writeonly");
                        break;
                        case ttMethod:
                            strcat(htmlBuf, "m");
                        break;
                    }
                    strcat(htmlBuf, ";");
                }

                _server->send(200, DEFAULT_MIME_TYPE, htmlBuf);
                return;
            }
            _server->send(500, DEFAULT_MIME_TYPE, "Handler not found");  
        });
}