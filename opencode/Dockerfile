FROM ghcr.io/anomalyco/opencode:latest

USER root

RUN apk add --no-cache \
    git \
    git-lfs \
    openssh-client \
    curl \
    wget \
    ca-certificates \
    shadow \
    bash \
    python3 \
    py3-pip \
    udev \
    gcompat

RUN pip3 install --break-system-packages platformio

WORKDIR /docker
ENV PATH=$PATH:/docker

COPY ./*.sh .
RUN chmod +x *.sh

CMD [ "/bin/bash", "-i" ]
ENTRYPOINT [ "entrypoint.sh" ]
