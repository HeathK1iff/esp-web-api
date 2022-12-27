#ifndef WEB_API_VALUE_HANDLER_H 
#define WEB_API_VALUE_HANDLER_H

#include "string.h"

#define WEB_API_ERROR_LENGTH 100

typedef enum TypeOfTag { ttProperty, ttReadOnlyProperty, ttWriteOnlyProperty, ttMethod};

typedef struct Tag
{
    char Name[20]; 
    TypeOfTag Type;
};

typedef struct TagArray
{
   const Tag *Array;
   int Array_size;
} ;

class WebAPIHandler{
    private:
      char _lastError[WEB_API_ERROR_LENGTH];
      bool _error;
    protected:
      void error(const char* message){
          memset(_lastError, '\0', WEB_API_ERROR_LENGTH);
          strcat(_lastError, message);
          _error = true;
      }

    public:
      const char* getLastError() {
          return _lastError;
      }

      void resetError(){
         _lastError[0] = '\0';
         _error = false;
      }

      virtual const TagArray getTagArray() = 0;
      virtual const char* getValueByTag(const char* tag) = 0;
      virtual bool setValueByTag(const char* tag, const char* value) = 0;   
      virtual bool callMethodByTag(const char* tag) = 0;
};

#endif