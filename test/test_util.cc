#include"../HCServer/HCServer.h"
#include<assert.h>

HCServer::Logger::ptr g_logger=HCSERVER_LOG_ROOT();
void test_util()
{   
    HCSERVER_LOG_INFO(g_logger)<<HCServer::BacktraceToString(10);
    //HCSERVER_ASSERT1(false);
    HCSERVER_ASSERT2(false,"abcdef xx");
}
int main(){
    test_util();
    return 0;
}
