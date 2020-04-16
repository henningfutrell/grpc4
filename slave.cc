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
    std::string hostname_and_port;
    std::string router_info;
    int opt = 0;
    while ((opt = getopt(argc, argv, "h:r:")) != -1){
        switch(opt) {
            case 'h':
                hostname_and_port = optarg;break;
            case 'r':
                router_info = optarg;break;
            default:
                std::cerr << "Invalid Command Line Argument\n";
        }
    }

    if (hostname_and_port.empty() || router_info.empty()) {
        std::cout << "Please enter -h <host ip>:<port> -r <router ip>:<port>" << std::endl;
        exit(1);
    }

    daemonize(argv);
}