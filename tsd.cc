//
// Created by henning on 2/4/20.
//

#include <algorithm>
#include <chrono>
#include <experimental/filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <thread>

#include <grpc++/grpc++.h>
#include <unistd.h>
#include "timeline.pb.h"
#include "timeline.grpc.pb.h"
#include "router.pb.h"
#include "router.grpc.pb.h"

#include "client.h"
#include "stringmanipulation.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Service;
using grpc::Status;
using grpc::Status;
using router::rpc::Ack;
using router::rpc::RouterService;
using router::rpc::RouteInfo;
using timeline::rpc::Command;
using timeline::rpc::CommandStatus;
using timeline::rpc::Result;
using timeline::rpc::TimelineService;
using timeline::rpc::TimelineUpdate;

namespace fs = std::experimental::filesystem;

struct Post {
    std::time_t time;
    std::string msg;
    std::string username;
    std::string to_string() {
        std::stringstream ss;
        ss << (int) time << " " << username << " " << msg;
        return ss.str();
    }

    static Post from_string(std::string str) {
        Post new_post{};
        if (!str.empty()) {
            std::vector<std::string> words = split(str);

            new_post.time = (std::time_t) atoi(words[0].c_str());
            new_post.username = words[1];
            new_post.msg = words[2];
        }
        return new_post;
    }

    bool operator < (const Post& rhs) {
        return this->time < rhs.time;
    }

    bool operator == (const Post& rhs) {
        return this->time == rhs.time;
    }

    bool operator > (const Post& rhs) {
        return this->time > rhs.time;
    }
};

struct User {

    std::string name;
    std::map<std::string, std::string> followers{};
    std::map<std::string, std::string> following{};

    std::vector<Post> timeline;
    std::string dir_prefix = "db";

    User() = default;
    explicit User(std::string name) {
        this->name = name;
        this->followers = {};
        this->following = {};
    }

    void serialize() {
        std::stringstream ss(fs::path().string());
        ss <<"db/" << this->name << ".db";

        std::string path = ss.str();
        std::ofstream file(path);
        file << name << std::endl;
        for (const auto& follower : followers) {
            file << follower.first << " ";
        }
        file << std::endl;

        for (const auto& followee : following) {
            file << followee.first << " ";
        }
        file << std::endl;

        for (auto& post : timeline) {
            file << post.to_string() << std::endl;
        }
        file.close();
    }

    static User deserialize(std::fstream& file) {
        User deserialized_user{};
        std::string uname, followers_raw, following_raw;
        getline(file, deserialized_user.name); // get users name (line 1)

        getline(file, followers_raw); // get follower list (line 2)
        for (const auto& follower : split(followers_raw)) {
            deserialized_user.followers.emplace(follower, follower);
        }

        getline(file, following_raw);
        for (const auto& following : split(followers_raw)) {
            deserialized_user.following.emplace(following, following);
        }

        std::string post;
        while (getline(file, post)) { // get posts (line 3+)
            if (!post.empty()) {
                deserialized_user.timeline.push_back(Post::from_string(post));
            }
        }
        file.close();

        return deserialized_user;
    }
};


typedef std::map<std::string, User> UserMap;

class UserPool {
    UserMap users{};

public:

    UserPool() {
        try{
            for (const auto& file : fs::directory_iterator("db")) {
                std::fstream f(file.path());
                deserialize_file(f);
            }
        } catch(fs::filesystem_error& e) {
            fs::create_directory("db");
        }
    }

    bool has_user(std::string name) {
        return users.find(name) != users.end();
    }

    std::vector<std::string> get_all_usernames() {
        std::vector<std::string> all_users;
        for (const auto& index : users) {
            all_users.push_back(index.first);
        }
        return all_users;
    }

    std::vector<std::string> get_users_followers(const std::string& name) {
        std::vector<std::string> names;
        for (const auto& follower : users[name].followers)
            names.push_back(follower.first);
        return names;
    }

    std::vector<std::string> get_users_following(const std::string& name) {
        std::vector<std::string> names;
        for (const auto& followee : users[name].following)
            names.push_back(followee.first);
        return names;
    }

    IStatus add_user(const std::string& name) {
        if (has_user(name)) {
            return FAILURE_ALREADY_EXISTS;
        }

        // else put the user in the pool
        User user(name);
        users.emplace(name, user);
        serialize_user(name);
        return SUCCESS;
    }

    IStatus add_user_follower(const std::string& user, const std::string& follower) {
        if (!has_user(user)) {
            return FAILURE_NOT_EXISTS;
        }
        users[user].followers.emplace(follower, follower);
        users[follower].following.emplace(user, user);
        serialize_user(user);
        serialize_user(follower);
        return SUCCESS;
    }

    IStatus remove_user_follower(const std::string& user, const std::string& follower) {
        if (!has_user(user)) {
            return FAILURE_INVALID_USERNAME;
        }
        users[user].followers.erase(follower);
        users[follower].following.erase(user);
        serialize_user(user);
        serialize_user(follower);
        return SUCCESS;
    }

    void serialize_user(std::string name) {
        users[name].serialize();
    }

    void deserialize_file(std::fstream& file) {
        User user = User::deserialize(file);
        users.emplace(user.name, user);
    }

    void update_timeline(std::string name, Post post) {
        users[name].timeline.push_back(post);
        serialize_user(name);
    }

    std::vector<Post> get_users_timeline(std::string name) {
        return users[name].timeline;
    }
};


class PoolDelegate {
    UserPool pool{};

public:
    void list(Result* result, const std::string& username) {
        if (pool.has_user(username)) {
            std::vector<std::string> usernames = pool.get_all_usernames();
            std::vector<std::string> followers = pool.get_users_followers(username);
            for (int i = 0; i < usernames.size(); i++) {
                result->mutable_all_user_list()->Add();
                result->set_all_user_list(i, usernames[i]);
            }
            for (int i = 0; i < followers.size(); i++) {
                result->mutable_following_user_list()->Add();
                result->set_following_user_list(i, followers[i]);
            }
            result->set_status(CommandStatus::SUCCESS);
        } else {
            result->set_status(CommandStatus::FAILURE_NOT_EXISTS);
        }
    }

    void add_user(const std::string& username) {
        pool.add_user(username);
    }

    void follow_self(const std::string& username) {
        pool.add_user_follower(username, username);
    }

    void add_follower(Result* result, const std::string& username, const std::string& target) {
        if(username == target) {
            result->set_status(CommandStatus::FAILURE_INVALID);
        } else {
            result->set_status((CommandStatus) pool.add_user_follower(target, username));
        }
    }

    void remove_follower(Result* result, const std::string& username, const std::string& target) {
        result->set_status((CommandStatus)pool.remove_user_follower(target, username));
    }

    std::vector<std::string> get_followers(std::string name) {
        return pool.get_users_followers(name);
    }

    std::vector<std::string> get_following(std::string name) {
        return pool.get_users_following(name);
    }

    void serialize_user(std::string name) {
        pool.serialize_user(name);
    }

    void update_timeline(std::string name, Post post) {
        pool.update_timeline(name, post);
    }

    std::vector<Post> get_timeline(std::string name) {
        return pool.get_users_timeline(name);
    }
};

class TimelineServiceImpl final : public TimelineService::Service {
    PoolDelegate pool_delegate;
    std::map<std::string, ServerReaderWriter<TimelineUpdate, TimelineUpdate>*> listeners;

    Status DoCommand(ServerContext* context,
                     const Command* command,
                     Result* result
    ) override {
        const std::string& cmd_string = command->command();
        const std::string& username = command->username();

        // create and follow self
        pool_delegate.add_user(username);
        pool_delegate.follow_self(username);
        pool_delegate.serialize_user(username);

        std::stringstream ss(cmd_string);
        std::string cmd, target;
        ss >> cmd >> target;

        // try to record the user, in case not yet listed

        if (up_case_str(cmd) == "LIST") {
            pool_delegate.list(result, username);
        } else if (up_case_str(cmd) == "FOLLOW") {
            pool_delegate.add_follower(result, username, target);
            // update the user
            pool_delegate.serialize_user(target);
        } else if (up_case_str(cmd) == "UNFOLLOW") {
            pool_delegate.remove_follower(result, username, target);
            pool_delegate.serialize_user(target);
        }


        return Status::OK;
    }

    Status GetTimeline(ServerContext* context,
                       ServerReaderWriter<TimelineUpdate, TimelineUpdate>* stream
    ) override  {
        std::string username = context->client_metadata().find("uname")->second.data();
        username = username.substr(0, username.find('\303'));
        listeners.emplace(username, stream);
        std::vector<Post> all_posts;

        // get the users following last 20 posts and place on all post pool
        for (const auto& followee : pool_delegate.get_following(username)) {
            std::vector<Post> posts = pool_delegate.get_timeline(followee);
            int offset = posts.size() > 20 ? 20 : posts.size();
            all_posts.insert(all_posts.end(), posts.end() - offset, posts.end());
        }

        // sort the array to order all posts
        std::sort(all_posts.begin(), all_posts.end());
        for (const auto& post : all_posts) {
            TimelineUpdate new_posting;
            new_posting.set_user_name(post.username);
            new_posting.set_content(post.msg);
            new_posting.set_time((int)post.time);
            stream->Write(new_posting);
        }

        TimelineUpdate p;
        while(stream->Read(&p)) {
            std::time_t time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            const std::string& msg = p.content();
            username = p.user_name();

            // update the users db
            std::sort(all_posts.begin(), all_posts.end());
            Post post{};
            post.time = time;
            post.username = username;
            post.msg = msg;
            pool_delegate.update_timeline(username, post);

            TimelineUpdate new_posting;
            new_posting.set_user_name(post.username);
            new_posting.set_content(post.msg);
            new_posting.set_time((int)post.time);
            for(auto listener : listeners) {
                auto followers = pool_delegate.get_followers(username);
                if (std::find(followers.begin(), followers.end(), listener.first) != followers.end()) {
                    listener.second->Write(new_posting);
                }
            }
        }
        return Status::OK;
    }
};

int main(int argc, char** argv) {
    std::string hostname_and_port = "localhost:3000";
    std::string router_info = "localhost:3005";
    int opt = 0;
    while ((opt = getopt(argc, argv, "h:u:p:r:")) != -1){
        switch(opt) {
            case 'h':
                hostname_and_port = optarg;break;
            case 'r':
                router_info = optarg;break;
            default:
                std::cerr << "Invalid Command Line Argument\n";
        }
    }

    ServerBuilder builder;
    builder.AddListeningPort(hostname_and_port, grpc::InsecureServerCredentials());

    TimelineServiceImpl service;
    builder.RegisterService(&service);

    std::unique_ptr<Server> server(builder.BuildAndStart());

    // register with router server
    bool connected = false;
    std::shared_ptr<RouterService::Stub> router_stub;

    std::cout << "Connecting to router at " << router_info << std::endl;
    router_stub = std::unique_ptr<RouterService::Stub>(
            RouterService::NewStub(grpc::CreateChannel(router_info,
                                                           grpc::InsecureChannelCredentials())));

    std::thread keep_alive_thread([hostname_and_port, router_stub]() {
        ClientContext context;
        auto stream = std::shared_ptr<ClientReaderWriter<RouteInfo,
        Ack>>(router_stub->KeepAliveRoute(&context));
        RouteInfo r;
        Ack ack;
        r.set_ip_address_and_port(hostname_and_port);
        while (true) {
            Ack ack;
            stream->Write(r);
            stream->Read(&ack);
            sleep(1);
        }
    });
    keep_alive_thread.detach();

    std::cout << "Server listening on " << hostname_and_port << std::endl;
    // Wait for the server to shutdown.
    server->Wait();

    return 0;
}
