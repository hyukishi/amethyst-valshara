FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive \
    TZ=America/Chicago

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
       bash \
       build-essential \
       gdb \
       libsqlite3-dev \
       libxcrypt-dev \
       make \
       netcat-openbsd \
       pkg-config \
       ripgrep \
       sqlite3 \
       telnet \
       tini \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

COPY . /workspace

RUN find src -maxdepth 1 -name '*.o' -delete \
    && rm -f src/smaug \
    && make -C src

EXPOSE 4000

ENTRYPOINT ["/usr/bin/tini", "--"]
CMD ["bash"]
