#include "iostream"
#include "test.pb.h"

int main()
{
    fixbug::LoginRequest req;
    req.set_name("zhangsan");
    req.set_pwd("123456");
    return 0;
}