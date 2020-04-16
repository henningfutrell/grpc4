#include <iostream>
//#include <memory>
#include <thread>
//#include <vector>
#include <string>
#include <unistd.h>
#include <grpc++/grpc++.h>
#include <sstream>
#include <utility>
#include "client.h"
#include "router.pb.h"
#include "router.grpc.pb.h"
#include "timeline.pb.h"
#include "timeline.grpc.pb.h"

using grpc::Channel;
using grpc::Service;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::CompletionQueue;
using grpc::Status;
using timeline::rpc::TimelineService;
using timeline::rpc::TimelineUpdate;
using timeline::rpc::Command;
using timeline::rpc::Result;
using router::rpc::Ack;
using router::rpc::RouterService;
using router::rpc::RouteInfo;

class Client : public IClient
{
    std::string hostname;
    std::string username;
    std::string port;
    std::string router_info;
    std::unique_ptr<TimelineService::Stub> server_stub;
    std::unique_ptr<RouterService::Stub> router_stub;
    ClientContext router_client_context;
    std::string server_info;
    std::unique_ptr<::grpc::ClientReader<::router::rpc::RouteInfo>> router_stream;
    std::thread update_reader;
    bool break_timeline = false;

protected:
    int connectTo() override;
    IReply processCommand(std::string& input) override;
    void processTimeline() override;
    void listenOnTimeline();

public:
    Client(std::string  hostname,
           std::string  username,
           std::string  port,
           std::string  router)
    {
        this->hostname = hostname;
        this->username = username;
        this->port = port;
        this->router_info = router;
        this->server_info = "";
    }

    ~Client() {
        this->update_reader.join();
    }

    int ConnectToServer(std::string server_ip_and_port);
};

int main(int argc, char** argv) {

    std::string hostname = "localhost";
    std::string username = "default";
    std::string port = "3000";
    std::string router;
    int opt = 0;
    while ((opt = getopt(argc, argv, "h:u:p:r:")) != -1){
        switch(opt) {
            case 'h':
                hostname = optarg;break;
            case 'u':
                username = optarg;break;
            case 'p':
                port = optarg;break;
            case 'r':
                router = optarg;break;
            default:
                std::cerr << "Invalid Command Line Argument\n";
        }
    }

    std::stringstream host_and_port;
    host_and_port << hostname << ":" << port;

    Client myc(hostname, username, port, router);

    // You MUST invoke "run_client" function to start business logic
    myc.run_client();

    return 0;
}

int Client::connectTo()
{
    // Need to get information from the routing server
    this->router_stub = std::unique_ptr<RouterService::Stub>(
            RouterService::NewStub(grpc::CreateChannel(this->router_info,
                                                       grpc::InsecureChannelCredentials())));

    this->router_stream = router_stub->SubscribeToRouteInfo(&this->router_client_context,Ack());
    this->update_reader = std::thread([this]() {
        bool good = true;
        while(good) {
            RouteInfo route;
            bool status = this->router_stream->Read(&route);
            if(route.ip_address_and_port() != this->server_info) {
                this->ConnectToServer(route.ip_address_and_port());
                this->break_timeline = true;
            }
            sleep(3);
        }
    });
    this->update_reader.detach();

    return 1; // return 1 if success, otherwise return -1
}

int Client::ConnectToServer(std::string server_ip_and_port) {
    this->server_stub = std::unique_ptr<TimelineService::Stub>(
            TimelineService::NewStub(grpc::CreateChannel(server_ip_and_port,
                                                         grpc::InsecureChannelCredentials())));
}

IReply Client::processCommand(std::string& input)
{
    ClientContext context;
    Result result;

    Command command;
    command.set_command(input);
    command.set_username(username);

    IReply reply;

    reply.grpc_status = server_stub->DoCommand(&context, command, &result);
    reply.comm_status = static_cast<IStatus>(result.status());
    for(const auto& user : result.all_user_list()) {
        reply.all_users.push_back(user);
    }
    for (const auto& follower : result.following_user_list()) {
        reply.followers.push_back(follower);
    }

    return reply;
}

void Client::listenOnTimeline() {
    ClientContext context;
    context.AddMetadata("uname", username);

    this->break_timeline = false;
    std::shared_ptr<ClientReaderWriter<TimelineUpdate, TimelineUpdate>> stream(
            server_stub->GetTimeline(&context));

    std::string uname = username;
    // Thread used to write chat messages to the server
    std::thread writer([this, stream, &context]() {
        std::string msg;
        while (!break_timeline) {
            msg = getPostMessage();
            TimelineUpdate p;
            p.set_user_name(this->username);
            p.set_content(msg);
            stream->Write(p);
        }
        stream->WritesDone();
    });

    // Thread used to read incoming messages from the server
    std::thread reader([this, stream]() {
        TimelineUpdate p;
        TimelineUpdate last_p = p;
        while(!break_timeline) {
            stream->Read(&p);
            // When a stream crashes this will dump reads. Quick fix
            // is to not post the same timed posts over and over.
            if (p.time() != last_p.time()) {
                last_p = p;
                std::time_t t = p.time();
                displayPostMessage(p.user_name(), p.content(), t);
            }
        }
    });

    //Wait for the threads to finish
    writer.join();
    reader.join();
    listenOnTimeline();
}

void Client::processTimeline()
{
    /**
     * This bool is used to switch timeline on servers.
     * Make sure we don't break out immediately so put it up front
     * since setting the server will change this value
     * before timeline ever entered.
     **/

    ClientContext context;
    context.AddMetadata("uname", username);

    std::shared_ptr<ClientReaderWriter<TimelineUpdate, TimelineUpdate>> preamble(
            server_stub->GetTimelinePreamble(&context));

    TimelineUpdate p;
    TimelineUpdate last_p = p;
    while(preamble->Read(&p)) {
        // When a stream crashes this will dump reads. Quick fix
        // is to not post the same timed posts over and over.
        if (p.time() != last_p.time()) {
            last_p = p;
            std::time_t t = p.time();
            displayPostMessage(p.user_name(), p.content(), t);
        }
    }

    listenOnTimeline();
}
