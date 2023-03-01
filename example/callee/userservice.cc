#include<iostream>
#include <string>
#include "user.pb.h"
#include "rpcprovider.h"
#include "logger.h"
#include "mprpcapplication.h"
using namespace fixbug;
class UserService:public fixbug::UserServiceRpc
{
public:
    bool Login(std::string name,std::string pwd)
    {
        std::cout<<"do local login"<<std::endl;
        std::cout<<"name:"<<name<<" pwd:"<<pwd<<std::endl;
        return 0;
    }
    void Login(::google::protobuf::RpcController* controller,
                    const ::fixbug::LoginRequest* request,
                    ::fixbug::LoginResponse* response,
                    ::google::protobuf::Closure* done)
    {
         //框架给业务上报了请求参数：LoginRequest，应用程序取出相应的已反序列化的数据来做本地业务
        std::string name=request->name();
        std::string pwd=request->pwd();

        //做本地业务
        bool loginresult=Login(name,pwd);

        //把响应写入,包括错误码，错误信息和运行结果
        ResultCode *Code=response->mutable_result();
        Code->set_errcode(0);
        Code->set_errmsg("Login do success!");
        response->set_success(loginresult);

        //执行回调操作  执行响应对象数据的序列化和网络发送
        done->Run();
    }


};
int main(int argc,char **argv)
{
    
    //先调用框架的初始化操作 provider -i config.conf，从init方法读取配置服务，比如IP地址和端口号
    MprpcApplication::Init(argc,argv);
    //项目提供者，让我们可以发布该服务
    RpcProvider provider;
    //把UserService对象发布到rpc节点上
    provider.NotifyService(new UserService());
    
    //启动一个rpc服务发布节点，run以后，进程进入阻塞状态，等待远程的rpc请求
    provider.Run();
    return 0;
}