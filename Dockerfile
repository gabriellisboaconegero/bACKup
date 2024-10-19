FROM archlinux:base-devel

RUN mkdir -p /bACKup
WORKDIR /bACKup
ENTRYPOINT ["/bin/bash"]
