Initially run make. It will install cmake if not installed,
build the "build" directory, create the grpc files and build the binaries. The
machine must have the default password.

run:
$ make

After this, a router needs to be started. There is logic to search for a router,
but it works much better to just start a router with:
$./build/router -p <router_machine_ip>:<port>

To start a master-server pair on a machine run:
$ ./build/start -h <machine_ip>:<port> -r <router_machine_ip>:<port>

The clients are started with:
$ ./build/client -u <username> -r <router_machine_ip>:<port>

After all are connected they will transparently maintain connections.

KNOWN BUGS:
[Timeline] When the connection is broken and the client reconnects there is a
slight hang for a read. To make this work I would need to convert the stream to
an asynchronous reader writer, which is too much work for a non production
assignment that is 99% to spec.

    [-solution]: When the timeline server switches just send the intended message
    twice and it will succeed on the second send. This is just due to a blocking
    call in gRpc.

[Slave] If the slave is given bad information it will try to restart the server
very quickly. <b>If the slave goes crazy<b>, kill it and make sure the right host
information is given.

If the slave does nothing, make sure you are running it from the root directory.
