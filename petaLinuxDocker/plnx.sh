#!/usr/bin/env bash
# Helper for the PetaLinux 2024.1 container.
#   ./plnx.sh build              build the image (installer must be in this dir)
#   ./plnx.sh run [project_dir]  interactive shell, project_dir mounted at /work
set -euo pipefail

IMAGE=petalinux:2024.1
HERE="$(cd "$(dirname "$0")" && pwd)"

# If run via sudo, use the invoking user's IDs; never pass root (0) into the image
HOST_UID="${SUDO_UID:-$(id -u)}"
HOST_GID="${SUDO_GID:-$(id -g)}"
[ "$HOST_UID" -eq 0 ] && HOST_UID=1000
[ "$HOST_GID" -eq 0 ] && HOST_GID=1000

case "${1:-run}" in
  build)
    DOCKER_BUILDKIT=1 docker build --progress=plain \
      --build-arg UID="$HOST_UID" --build-arg GID="$HOST_GID" \
      -t "$IMAGE" "$HERE"
    ;;
  run)
    PROJ="$(realpath "${2:-$PWD}")"
    # Persistent Yocto download/sstate cache across containers -> much faster rebuilds
    CACHE="${PLNX_CACHE:-$HOME/.cache/petalinux-2024.1}"
    mkdir -p "$CACHE"/{downloads,sstate}
    # must be writable by the container user (UID $HOST_UID), even if run via sudo
    if [ "$(stat -c %u "$CACHE")" != "$HOST_UID" ]; then
      chown -R "$HOST_UID:$HOST_GID" "$CACHE" 2>/dev/null \
        || sudo chown -R "$HOST_UID:$HOST_GID" "$CACHE"
    fi
    docker run --rm -it \
      -v "$PROJ":/work \
      -v "$CACHE":/cache \
      --network host \
      -e COLUMNS="$(tput cols 2>/dev/null || echo 160)" \
      -e LINES="$(tput lines 2>/dev/null || echo 50)" \
      "$IMAGE"
    ;;
  *)
    echo "usage: $0 {build|run [project_dir]}"; exit 1 ;;
esac
