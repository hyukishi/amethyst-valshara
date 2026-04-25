FROM rockylinux:9

ENV TZ=America/Chicago

RUN dnf -y update \
    && dnf -y install \
       bash \
       gcc \
       gcc-c++ \
       gdb \
       glibc-devel \
       libxcrypt-devel \
       make \
       nc \
       pkgconf-pkg-config \
       sqlite \
       sqlite-devel \
       telnet \
    && dnf clean all \
    && rm -rf /var/cache/dnf

WORKDIR /workspace

COPY . /workspace

RUN find src -maxdepth 1 -name '*.o' -delete \
    && rm -f src/smaug \
    && make -C src

EXPOSE 4000

CMD ["bash"]
