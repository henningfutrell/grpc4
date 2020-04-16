#include <string>
#include <getopt.h>
#include <iostream>
#include <unistd.h>
#include <wait.h>

int daemonize(char** argv) {
    pid_t pid = fork();
    if (pid == 0) {
        execv("fbsd", argv);
    } else {
        int status;
        waitpid(pid, &status, 0);
        if (status != 0) {
            // restart
            std::cout << "Server failed. Restarting..." << std::endl;
            daemonize(argv);
        }
    }
    return 0;
}

int main(int argc, char** argv) {
    std::string hostname_and_port = "localhost:3000";
    std::string router_info = "localhost:3005";
    int opt = 0;
    while ((opt = getopt(argc, argv, "h:u:p:r")) != -1){
        switch(opt) {
            case 'h':
                hostname_and_port = optarg;break;
            case 'r':
                router_info = optarg;break;
            default:
                std::cerr << "Invalid Command Line Argument\n";
        }
    }

    daemonize(argv);
}