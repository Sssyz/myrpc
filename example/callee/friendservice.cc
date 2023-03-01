#include <iostream>
#include "friend.pb.h"
#include "mprpcapplication.h"
#include "logger.h"
#include "rpcprovider.h"
class FriendService:public fixbug::FriendServiceRpc
{
    std::vector<std::string> GetFriendList(int id)
    {
        std::cout<<"do getFriend"<<"id: "<<id<<std::endl;
        std::vector<std::string> lists{"zhangsan","lisi"};
        return lists;
    }
    void GetFriendList(::google::protobuf::RpcController* controller,
                       const ::fixbug::FriendListRequest* request,
                       ::fixbug::FriendListResponse* response,
                       ::google::protobuf::Closure* done)
   {
        int id = request->id();
        auto list  = GetFriendList(id);
        response->set_success(1);
        fixbug::ResultCode* res= response->mutable_result();
        res->set_errcode(1);
        res->set_errmsg("success");
        for(auto& name:list)
        {
            std::string* p = response->add_friend_();
            *p = name;
        }
        done->Run();
   }                    
};
int main(int argc,char** argv){
    MprpcApplication::Init(argc,argv);
    RpcProvider provider;
    provider.NotifyService(new FriendService());
    provider.Run();
}
