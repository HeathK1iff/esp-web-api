#ifndef WebAPI_h
#define WebAPI_h 

#include "WebAPIHandler.h"
#include "Time.h"

#ifdef ESP8266
#include <ESP8266WebServer.h>
#elif ESP32
#include <WebServer.h>
#endif

#define URL_BASE_LENGTH 128

class WebAPI
{
    private:
        enum TypeOfTag { ttProperty, ttReadOnlyProperty, ttWriteOnlyProperty, ttMethod};

        class HandlerListItem{
            private:
                char _baseUrl[URL_BASE_LENGTH];
                WebAPIHandler* _handler;
                HandlerListItem* _child;
            public:
                HandlerListItem(const char *baseUrl,  WebAPIHandler *handler);
                const char* getBaseUrl();

                const char* _getUrlWithTag(char* buf, const char* tag);
                static void splitUrl(const char* url, char **baseUrl, char **tag);

                WebAPIHandler* getHandler(const char *baseUrl);
                void setChild(HandlerListItem *child);
                HandlerListItem *getLastChild();
        };

        HandlerListItem* _root;
        bool _updateError;
        #ifdef ESP8266
        ESP8266WebServer* _server;
        #elif ESP32
        WebServer *_server;
        #endif
       
        void _splitUrl(char *url, char **baseUrl, char **prop);
    public:
        #ifdef ESP8266
        WebAPI(ESP8266WebServer* server);
        #elif ESP32
        WebAPI(WebServer* server);
        #endif

        void begin();
        void addHandler(const char* baseUrl, WebAPIHandler *handler);
};

#endif