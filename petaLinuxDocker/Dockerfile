# syntax=docker/dockerfile:1.4
#
# PetaLinux 2024.1 build environment
# Host OS: Ubuntu 22.04 LTS (officially supported for 2024.1)
#
# The PetaLinux installer is NOT redistributable, so download it from AMD
# yourself and place it next to this Dockerfile before building:
#     petalinux-v2024.1-05202009-installer.run
#
# Build (BuildKit required for the bind mount, so the ~2.5 GB installer
# never ends up in an image layer):
#   DOCKER_BUILDKIT=1 docker build \
#       --build-arg UID=$(id -u) --build-arg GID=$(id -g) \
#       -t petalinux:2024.1 .

FROM ubuntu:22.04

ARG DEBIAN_FRONTEND=noninteractive
ARG PLNX_INSTALLER=petalinux-v2024.1-05202009-installer.run
# Space-separated subset of: arm aarch64 microblaze
# (arm = Zynq-7000, aarch64 = ZynqMP/Versal). Fewer platforms = smaller image.
ARG PLNX_PLATFORMS="arm aarch64"
ARG PLNX_DIR=/opt/petalinux/2024.1
ARG USERNAME=plnx
ARG UID=1000
ARG GID=1000

# 32-bit libs are still needed by some PetaLinux tools
RUN dpkg --add-architecture i386 && apt-get update && apt-get install -y --no-install-recommends \
        autoconf automake bc bison build-essential ca-certificates chrpath cpio \
        debianutils diffstat dnsutils file flex gawk gcc-multilib git git-core gnupg gzip \
        iproute2 iputils-ping less libegl1-mesa libglib2.0-0 libncurses5-dev \
        libncurses5 libtinfo5 libsdl1.2-dev libselinux1 libssl-dev libtool \
        locales lsb-release lz4 make net-tools pax pylint python3 python3-git \
        python3-jinja2 python3-pexpect python3-pip python3-subunit python-is-python3 \
        rsync screen socat sudo tar texinfo tftpd-hpa unzip vim-tiny wget \
        xterm xxd xz-utils zlib1g-dev zlib1g:i386 zstd \
    && rm -rf /var/lib/apt/lists/*

# PetaLinux requires /bin/sh -> bash (not dash) and a UTF-8 locale
RUN echo "dash dash/sh boolean false" | debconf-set-selections \
    && dpkg-reconfigure -f noninteractive dash \
    && locale-gen en_US.UTF-8 \
    && update-locale LANG=en_US.UTF-8
ENV LANG=en_US.UTF-8 LC_ALL=en_US.UTF-8 TERM=xterm-256color

# PetaLinux refuses to install or run as root -> unprivileged user whose
# UID/GID match the host user so files in mounted project dirs stay yours
# (reuses an existing group if the host GID is already taken, e.g. GID 20 =
#  macOS 'staff' = Ubuntu 'dialout'; renames an existing user with that UID)
RUN set -eux; \
    if [ "${UID}" -eq 0 ] || [ "${GID}" -eq 0 ]; then \
        echo "ERROR: UID/GID 0 (root) passed in. PetaLinux cannot run as root;" \
             "build without sudo or pass --build-arg UID=1000 --build-arg GID=1000" >&2; \
        exit 1; \
    fi; \
    if ! getent group ${GID} > /dev/null; then groupadd -g ${GID} ${USERNAME}; fi; \
    existing_user="$(getent passwd ${UID} | cut -d: -f1 || true)"; \
    if [ -n "$existing_user" ]; then \
        usermod -l ${USERNAME} -d /home/${USERNAME} -m -g ${GID} -s /bin/bash "$existing_user"; \
    else \
        useradd -m -u ${UID} -g ${GID} -s /bin/bash ${USERNAME}; \
    fi; \
    echo "${USERNAME} ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/${USERNAME} \
    && mkdir -p ${PLNX_DIR} && chown ${UID}:${GID} ${PLNX_DIR}

USER ${USERNAME}
WORKDIR /home/${USERNAME}

# Install PetaLinux (installer bind-mounted from the build context, not copied)
RUN --mount=type=bind,source=${PLNX_INSTALLER},target=/tmp/plnx-installer.run \
    echo "arch=$(uname -m) user=$(id -un) sh=$(readlink -f /bin/sh)"; df -h /tmp ${PLNX_DIR}; \
    cp /tmp/plnx-installer.run /tmp/installer.run && chmod +x /tmp/installer.run \
    && /tmp/installer.run -y --log /tmp/plnx_install.log -d ${PLNX_DIR} --platform "${PLNX_PLATFORMS}" < /dev/null \
    || { echo "===== PetaLinux install FAILED, log tail: ====="; \
         tail -n 80 /tmp/plnx_install.log 2>/dev/null; \
         exit 1; }; \
    rm -f /tmp/installer.run

# Source the PetaLinux environment in every interactive shell
RUN echo "source ${PLNX_DIR}/settings.sh > /dev/null" >> ~/.bashrc
ENV PETALINUX=${PLNX_DIR}

WORKDIR /work
CMD ["/bin/bash", "-l"]
