FROM ubuntu:18.04

ENV DEBIAN_FRONTEND=noninteractive
ENV INSTALL_DIR=/usr/local
ENV GRPC_VERSION=v1.28.1
ENV CMAKE_VERSION=3.17.0

RUN apt-get update && \
    apt-get -y install \
    build-essential \
    autoconf \
    libtool \
    pkg-config \
    vim \
    clang \
    wget \
    libboost-all-dev \
    libevent-dev \
    libdouble-conversion-dev \
    libgoogle-glog-dev \
    libgflags-dev \
    libiberty-dev \
    liblz4-dev \
    liblzma-dev \
    libsnappy-dev \
    make \
    zlib1g-dev \
    binutils-dev \
    libjemalloc-dev \
    libssl-dev \
    libunwind-dev \
    net-tools \
    iproute2 \
    tcpdump \
    netcat \
    nmap \
    strace \
    git-all \
    && rm -rf /var/lib/apt/lists/*

SHELL ["/bin/bash", "-c"]

# Get/install the version of cmake grpc requires
RUN wget -q -O cmake-linux.sh https://github.com/Kitware/CMake/releases/download/v$CMAKE_VERSION/cmake-$CMAKE_VERSION-Linux-x86_64.sh
RUN sh cmake-linux.sh -- --skip-license --prefix=/usr/local
RUN rm cmake-linux.sh

WORKDIR /git_repos

RUN git clone --recurse-submodules -b $GRPC_VERSION https://github.com/grpc/grpc
RUN git clone https://github.com/fmtlib/fmt.git
RUN git clone https://github.com/facebook/folly.git
RUN git clone https://github.com/rsocket/rsocket-cpp.git
RUN git clone https://github.com/google/glog.git

# Build/install grpc and all its dependencies based on:
# https://grpc.io/docs/quickstart/cpp/
WORKDIR /git_repos/grpc/cmake/build
RUN cmake -DgRPC_INSTALL=ON \
    -DgRPC_BUILD_TESTS=OFF \
    -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR \
    ../..
RUN make -j $(nproc)
RUN make install

# Build/install fmt required by folly based on:
# https://github.com/facebook/folly
WORKDIR /git_repos/fmt/_build
RUN cmake -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR ..
RUN make -j $(nproc)
RUN make install

# Build/install folly required by rsocket-cpp based on:
# https://github.com/facebook/folly
WORKDIR /git_repos/folly/_build
RUN cmake -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR ..
RUN make -j $(nproc)
RUN make install

# Build/install glog required by rsocket-cpp based on:
# https://github.com/google/glog/blob/master/cmake/INSTALL.md
WORKDIR /git_repos/glog
RUN cmake -H. -Bbuild -G "Unix Makefiles"
RUN cmake -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR --build build
RUN cmake --build build --target install

# Build/install rsocket-cpp based on:
# https://github.com/rsocket/rsocket-cpp
WORKDIR /git_repos/rsocket-cpp/build
RUN cmake -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR ..
RUN make -j $(nproc)
RUN make install

RUN mkdir -p /rpc_transport/bin
RUN mkdir -p /rpc_transport/rt_client_server
ENV PATH="${PATH}:/rpc_transport/bin"

# Build grpc hello world
COPY src/transport_hello_world/grpc /rpc_transport/hello_world/grpc
RUN rm -rf /rpc_transport/hello_world/grpc/cmake/
WORKDIR /rpc_transport/hello_world/grpc/cmake/build
RUN cmake ../..
RUN make -j $(nproc)
RUN cp greeter_{client,server} /rpc_transport/bin/

# Build rsockets hello world
COPY src/transport_hello_world/rsocket /rpc_transport/hello_world/rsocket
RUN rm -rf /rpc_transport/hello_world/rsocket/cmake/
WORKDIR /rpc_transport/hello_world/rsocket/cmake/build
RUN cmake ../..
RUN make -j $(nproc)
RUN cp rsocket_test_{client,server} /rpc_transport/bin/

# Build rt_client_server
COPY src/rt_client_server /rpc_transport/rt_client_server
RUN rm -rf /rpc_transport/rt_client_server/cmake
WORKDIR /rpc_transport/rt_client_server/cmake/build
RUN cmake ../..
RUN make -j $(nproc)
RUN cp rt_{client,server} /rpc_transport/bin/

# Set clang to the default compiler for development
ENV CC=/usr/bin/clang
ENV CXX=/usr/bin/clang++

WORKDIR /

CMD ["/bin/bash"]
