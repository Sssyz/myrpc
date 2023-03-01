#include <iostream>
#include "mprpcapplication.h"
#include "friend.pb.h"
#include "mprpcchannel.h"
int main(int argc,char** argv)
{
    MprpcApplication::Init(argc,argv);
    fixbug::FriendServiceRpc_Stub stub(new MpRpcChannel());
     //rpc方法的请求参数
    fixbug::FriendListRequest request;
    request.set_id(1);
    
     //rpc方法的响应
    fixbug::FriendListResponse response;
    MprpcController controller;
    //发起rpc方法的调用，同步的rpc调用过程，MprpcChannel::callmethod
    stub.GetFriendList(&controller,&request,&response,nullptr);
    if(controller.Failed())
    {
        std::cout<<"do getfriend failed text:"<<controller.ErrorText()<<std::endl;
    }
    else{
        //一次rpc调用完成，读取调用的结果
        if(0==response.result().errcode())
        {
            std::cout<<"rpc getfriend response:"<<response.success()<<std::endl;
        }
        else
        {
            std::cout<<"rpc getfriend response error:"<<response.result().errmsg()<<std::endl;
        }
    }
     
    return 0;
}