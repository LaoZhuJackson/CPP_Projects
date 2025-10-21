#include "../../../include/utils/db/DbConnection.h"
#include "../../../include/utils/db/DbException.h"
#include <muduo/base/Logging.h>

namespace http::db{
DbConnection::DbConnection(const std::string& host, const std::string& user, const std::string& password,const std::string& database)
    : host_(host)
    , user_(user)
    , password_(password)
    , database_(database){
        try{
            sql::mysql::MySQL_Driver* driver
        }
    }
}