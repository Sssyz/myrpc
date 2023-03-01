#include "rpcprovider.h"
#include "mprpcapplication.h"
#include <string>
#include "functional"
#include "rpcheader.pb.h"
#include "logger.h"
#include "zookeeperutil.h"

void RpcProvider::NotifyService(google::protobuf::Service *service){

    ServiceInfo service_info;
    const google::protobuf::ServiceDescriptor *pserviceDesc=service->GetDescriptor();
    //获取服务的名字
    std::string service_name=pserviceDesc->name();
    //获取服务对象service的方法数量
    int methodCnt=pserviceDesc->method_count();
    std::cout<<"service name:"<<service_name<<std::endl;
    for(int i=0;i<methodCnt;++i)
    {
        //获取了服务对象指定下标的服务方法的描述（抽象描述）
        const google::protobuf::MethodDescriptor* pmethodDesc=pserviceDesc->method(i);
        std::string method_name=pmethodDesc->name();
        std::cout<<"method name:"<<method_name<<std::endl;
        //插入服务
        service_info.m_methodMap.insert({method_name,pmethodDesc});
        LOG_INFO("method_name:%s",method_name.c_str());
    }
    //可以使用该表来调用方法
    service_info.m_service=service;
    m_serviceMap.insert({service_name,service_info});
}


void RpcProvider::Run(){
    std::string ip=MprpcApplication::GetInstance().GetConfig().Load("rpcserverip");
    uint16_t port=atoi(MprpcApplication::GetInstance().GetConfig().Load("rpcserverport").c_str());
    muduo::net::InetAddress address(ip,port);
    // 创建TCP server
    muduo::net::TcpServer server(&m_eventLoop,address,"RpcProvider");
    // 绑定连接回调和消息读写回调
    server.setConnectionCallback(std::bind(&RpcProvider::onConnnection,this,std::placeholders::_1));
    server.setMessageCallback(std::bind(&RpcProvider::onMessage,this,
                                std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));
    
    //设置线程数
    server.setThreadNum(4);
    //把当前rpc节点上要发布的服务全部注册在zk上，让rpc client可以从zk上发现服务
    //session的timeout默认为30s，zkclient的网络I/O线程1/3的timeout内不发送心跳则丢弃此节点
    ZkClient zkCli;
    zkCli.Start();//链接zkserver
    for(auto &sp:m_serviceMap)
    {
        //service_name
        std::string service_path ="/"+sp.first;//拼接路径
        zkCli.Create(service_path.c_str(),nullptr,0);//创建临时性节点
        for(auto &mp:sp.second.m_methodMap)
        {
            //service_name/method_name
            std::string method_path=service_path+"/"+mp.first;//拼接服务器路径和方法路径
            char method_path_data[128]={0};
            sprintf(method_path_data,"%s:%d",ip.c_str(),port);//向data中写入路径

            //创建节点,ZOO_EPHEMERAL表示临时节点
            zkCli.Create(method_path.c_str(),method_path_data,strlen(method_path_data),ZOO_EPHEMERAL);
        }
    }
    server.start();
    m_eventLoop.loop();
}
void RpcProvider::onConnnection(const muduo::net::TcpConnectionPtr& conn){
    if(!conn->connected())
    {
        conn->shutdown();
    }
}

void RpcProvider::onMessage(const muduo::net::TcpConnectionPtr& conn ,muduo::net::Buffer* buf,muduo::Timestamp time){

    std::string recv_buf = buf->retrieveAllAsString();
    // 从字符流中读4个字节记录数据头长度
    uint32_t header_size = 0;
    recv_buf.copy((char*)&header_size,4,0);
    // 根据数据头长度读取数据头
    std::string rpc_header_str = recv_buf.substr(4,header_size);
    // 反序列化数据
    mprpc::RpcHeader rpcHeader;
    std::string service_name;
    std::string method_name;
    uint32_t args_size;
    if(rpcHeader.ParseFromString(rpc_header_str))
    {
        // 反序列化成功
        service_name = rpcHeader.service_name();
        method_name = rpcHeader.methond_name();
        args_size = rpcHeader.args_size();
    }
    else
    {
        // 反序列化失败
        std::cout<<"rpc_header_str:"<<rpc_header_str<<"parse error!"<<std::endl;
    }
    // 获取调用参数
    std::string args_str = recv_buf.substr(4+header_size,args_size);

    //获取service和method 对象
    auto it = m_serviceMap.find(service_name);
    if(it == m_serviceMap.end())
    {
        std::cout<<service_name <<"is not exist!"<<std::endl;
        return;
    }

    auto mit = it->second.m_methodMap.find(method_name);
    if(mit == it->second.m_methodMap.end())
    {
        std::cout<<method_name <<"is not exist!"<<std::endl;
        return;
    }
    google::protobuf::Service *service = it->second.m_service;
    const google::protobuf::MethodDescriptor *method = mit->second;
    // 生成rpc调用的响应和请求
    google::protobuf::Message *request = service->GetRequestPrototype(method).New();
    if(!request->ParseFromString(args_str))
    {
        std::cout<<"request parse error!"<<std::endl;
        return;
    }
    google::protobuf::Message *response = service->GetResponsePrototype(method).New();
 
    // 给method方法绑定回调
    google::protobuf::Closure *done = 
     google::protobuf::NewCallback<RpcProvider,const muduo::net::TcpConnectionPtr&,google::protobuf::Message*>(this,&RpcProvider::SendRpcResponse,conn,response);
    service->CallMethod(method,nullptr,request,response,done);
}
void RpcProvider::SendRpcResponse(const muduo::net::TcpConnectionPtr& conn,google::protobuf::Message* response)
{
    std::string response_str;
    if(response->SerializeToString(&response_str))
    {
        conn->send(response_str);
    }else{
        std::cout<<"SerializeToString error"<<std::endl;
    }
    conn->shutdown();


}
