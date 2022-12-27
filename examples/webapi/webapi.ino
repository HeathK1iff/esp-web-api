
#include <WebAPI.h>
#include <WebAPIHandler.h>

#define SSID "XXX"
#define SSID_PASS "YYYY"

WebServer webServer(80);
WebAPI webApi(&webServer);

class Setting : public WebAPIHandler{
private:
    int _relayPin;
    bool _state;

    const Tag ARRAY_TAGS[2] = {
        {"relay_pin", ttProperty},
        {"relay_state", ttProperty}
    };

    const TagArray getTagArray(){
        return {ARRAY_TAGS, 2};
    }
      
    const char* getValueByTag(const char* tag){
         if (strcmp(tag, "relay_pin") == 0){
            char buf[20];
            return itoa(_relayPin, buf, 10);
        } else if (strcmp(tag, "relay_state") == 0)
        {
            if (_state){
                return "on";
            } else {
                return "off";
            }
        }
    }

    bool setValueByTag(const char* tag, const char* value){
       if (strcmp(tag, "relay_pin") == 0){
            _relayPin = atoi(value);
       } else if (strcmp(tag, "relay_state") == 0) {
            if (strcmp(value, "on") == 0){
                _state = true;
            } else 
            if (strcmp(value, "off") == 0) {
                _state = false;
            } else {
                error("Invalid value");
                return false;
            }

       }
       
       return true;
    } 

    bool callMethodByTag(const char* tag){
        
    }
public:
    

} settings;


void setup(){
    Serial.begin(115200);
    Serial.println("Connecting...");

    WiFi.begin(SSID, SSID_PASS);

    Serial.println("Connected");

    while (!WiFi.isConnected()){
        delay(100);
    }    

    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    webApi.begin();
    webApi.addHandler("/settings", &settings);
    
    webServer.begin();
}

void loop(){
    webServer.handleClient();
}