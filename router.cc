//
// Created by henning on 4/13/20.
//

#include <string>
#include <sstream>
#include <grpc++/grpc++.h>
#include <thread>
#include <unistd.h>
#include "router.pb.h"
#include "router.grpc.pb.h"

#include "stringmanipulation.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Service;
using grpc::Status;
using router::rpc::Ack;
using router::rpc::RouterService;
using router::rpc::RouteInfo;

class RouterServiceImpl final : public RouterService::Service {
    std::string master = "undefined";
    std::map<std::string, std::string> servers;

    // As servers come on line they register with the router.
    // The router keeps the list and decides which to use in the list as
    // the active master.
    Status KeepAliveRoute(ServerContext* context,
            ServerReaderWriter<Ack, RouteInfo>* stream
    ) override {
        RouteInfo route;
        stream->Read(&route);
        std::string route_info = route.ip_address_and_port();
        std::cout << "Route " << route_info << " registered. Keeping alive." << std::endl;

        while(true) {
            Ack ack_sent;
            stream->Write(ack_sent);
            stream->Read(&route);
            if (context->IsCancelled()) {
                std::cout << "Route " << route_info << " considered disconnected and dropped." << std::endl;
                servers.erase(route_info);

                return Status::CANCELLED;
            }
        }
    }

    Status SubscribeToRouteInfo(ServerContext* context,
                    const Ack* ack,
                   ServerWriter<RouteInfo>* writerStream
    ) override {
        std::cout << context->peer() << " subscribed to route info." << std::endl;
        std::string id = context->peer();
        std::string previous_master = master;
        while (true) {
            if (previous_master != master) {
                previous_master = master;
                RouteInfo r;
                r.set_ip_address_and_port(master);
                writerStream->Write(r);
            }
            if (context->IsCancelled()) {
                break;
            }
            sleep(1);
        }
        return Status::OK;
    }
};

int main(int argc, char** argv) {
    std::string host_and_port;
    int opt = 0;
    while ((opt = getopt(argc, argv, "p:")) != -1){
        switch(opt) {
            case 'p':
                host_and_port = optarg;break;
            default:
                std::cerr << "Invalid Command Line Argument\n";
        }
    }

    ServerBuilder builder;
    builder.AddListeningPort(host_and_port, grpc::InsecureServerCredentials());

    RouterServiceImpl service;
    builder.RegisterService(&service);
    builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIME_MS, 2000);
    builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 5);
    builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, 1);
    builder.AddChannelArgument(GRPC_ARG_HTTP2_MAX_PINGS_WITHOUT_DATA, 0);

    std::unique_ptr<Server> server(builder.BuildAndStart());

    std::cout << "Router server listening on " << host_and_port << std::endl;

    // Wait for the server to shutdown.
    server->Wait();

    return 0;
}