FROM ubuntu:20.04

# Install dependencies
ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y software-properties-common build-essential git sudo cmake g++-9 libgsl-dev clang libssl-dev pkg-config openssl libstdc++-9-dev libboost-all-dev libdouble-conversion-dev

# Install gPRC
RUN git clone --recurse-submodules -b v1.50.2 https://github.com/grpc/grpc
WORKDIR /grpc/cmake
RUN mkdir build
WORKDIR /grpc/cmake/build 
RUN cmake -DgRPC_INSTALL=ON -DgRPC_BUILD_TESTS=OFF ../..
RUN make -j 2
RUN make install
WORKDIR /

# Install CacheLib-Holpaca
RUN git clone -b dev https://github.com/vaavaav/CacheLib-Holpaca.git
CMD /bin/bash
#WORKDIR /CacheLib-Holpaca
# For now, checkout to dev
#RUN git checkout dev
#RUN ./contrib/build.sh -d -j -v
#
##ENV PORT 50000
##EXPOSE ${PORT}
#EXPOSE 5000
#
#CMD build-cachelib/cachelib_holpaca/cachelib_holpaca_run off off 2000000000 localhost:4000 default 0.5 192.168.112.137:5000