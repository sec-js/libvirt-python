# THIS FILE WAS AUTO-GENERATED
#
#  $ lcitool manifest ci/manifest.yml
#
# https://gitlab.com/libvirt/libvirt-ci

FROM registry.opensuse.org/opensuse/tumbleweed:latest

RUN zypper dist-upgrade -y && \
    zypper install -y \
           ca-certificates \
           ccache \
           gcc \
           git \
           glibc-locale \
           libvirt-devel \
           pkgconfig \
           python3-base \
           python3-build \
           python3-devel \
           python3-lxml \
           python3-pip \
           python3-pytest \
           python3-setuptools \
           python3-wheel \
           rpm-build && \
    zypper clean --all && \
    rm -f /usr/lib*/python3*/EXTERNALLY-MANAGED && \
    rpm -qa | sort > /packages.txt && \
    mkdir -p /usr/libexec/ccache-wrappers && \
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/cc && \
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/gcc

ENV CCACHE_WRAPPERSDIR="/usr/libexec/ccache-wrappers"
ENV LANG="en_US.UTF-8"
ENV PYTHON="/usr/bin/python3"
