FROM fedora:24
MAINTAINER Marcelo Jara Almeyda

RUN dnf install -y  llvm-devel \
					bison \
					flex \
					gcc-c++ \
					make \
					redhat-rpm-config \
					findutils
