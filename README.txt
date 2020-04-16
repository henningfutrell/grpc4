Initially run make. It will install cmake if not installed,
build the "build" directory, create the grpc files and build the binaries. The
machine must have the default password.

$ make

After make a router needs to be started on a machine.

$./build/router -p <router_machine_ip>:<port>

FROM THE PROJECT ROOT DIRECTORY start a master-server pair on the machines.
For each run:

$ ./build/start -h <machine_ip>:<port> -r <router_machine_ip>:<port>

The clients are started with:

$ ./build/client -u <username> -r <router_machine_ip>:<port>

If the port on the client needs to be manually set use "-p <port>".


KNOWN BUGS:

[Timeline]
When the connection is broken and the client reconnects there is a
slight hang for a read.

WHEN THE TIMELINE SERVER SWITCHES just send the intended message
twice and it will succeed on the second send. This is just due to a blocking
call in gRpc.

        [solution] To make this work I would need to convert the stream to
        an non blocking reader, which is too much work for a non production
        assignment that is 99% to spec.

[Slave]
IF THE SLAVE GOES CRAZY, kill it and make sure the right host
information is given. If the host info is wrong it will try and restart the
server very quickly over and over.

[Slave]
IF THE SLAVE DOES NOTHING make sure you run it from the root directory.
