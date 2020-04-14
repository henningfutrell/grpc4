To run initially simply run:

$ make

This should install cmake, build the "build" directory, create the grpc files and build the binaries.

After this the server can be run with

$ ./bin/fbsd

and the clients can be run with

$ ./bin/fbc -u <username>
