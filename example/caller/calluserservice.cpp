#include <iostream>
#include "mprpcapplication.h"
#include "user.pb.h"
#include "mprpcchannel.h"
int main(int argc,char** argv)
{
    MprpcApplication::Init(argc,argv);
    fixbug::UserServiceRpc_Stub stub(new MpRpcChannel());
     //rpc方法的请求参数
    fixbug::LoginRequest request;
    request.set_name("zhangsan");
    request.set_pwd("123");
     //rpc方法的响应
    fixbug::LoginResponse response;
    MprpcController controller;
    //发起rpc方法的调用，同步的rpc调用过程，MprpcChannel::callmethod
    stub.Login(&controller,&request,&response,nullptr);
    if(controller.Failed())
    {
        std::cout<<"do login failed text:"<<controller.ErrorText()<<std::endl;
    }
    else{
        //一次rpc调用完成，读取调用的结果
        if(0==response.result().errcode())
        {
            std::cout<<"rpc login response:"<<response.success()<<std::endl;
        }
        else
        {
            std::cout<<"rpc login response error:"<<response.result().errmsg()<<std::endl;
        }
    }
     
    return 0;
}