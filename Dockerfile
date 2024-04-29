FROM ubuntu:20.04

# Install dependencies
ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y software-properties-common build-essential git sudo cmake g++-9 libgsl-dev clang libssl-dev pkg-config openssl libstdc++-9-dev libboost-all-dev libdouble-conversion-dev

# Install gPRC
RUN git clone --recurse-submodules --depth 1 -b v1.50.2 https://github.com/grpc/grpc
WORKDIR /grpc/cmake
RUN mkdir build
WORKDIR /grpc/cmake/build 
RUN cmake -DgRPC_INSTALL=ON -DgRPC_BUILD_TESTS=OFF ../..
RUN make -j 2
RUN make install
WORKDIR /

# Install CacheLib-Holpaca
RUN git clone --depth 1 -b dev https://github.com/vaavaav/CacheLib-Holpaca
WORKDIR /CacheLib-Holpaca
RUN ./contrib/build.sh -d -j -v

EXPOSE 5000
EXPOSE 6000-6050
ENTRYPOINT [ "/CacheLib-Holpaca/docker-entrypoint.py" ]
