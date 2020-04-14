CMAKE=$(sh which cmake)

all: clean do_cmake build_grpc repeat_cmake build_exe

do_cmake:
	sh -e install_cmake.sh
	mkdir -p build 
	cd build && cmake ..

build_grpc:
	cd build && make generate_rpc_files

repeat_cmake:
	cd build && cmake ..

build_exe:
	cd build && make all

clean:
	rm -rf build/*
