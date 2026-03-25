FROM ghcr.io/eth-cscs/alps-images:py26.01-alps3-base

ARG SPACK_SHA=v1.1.1
ARG SPACK_PACKAGES_SHA=bc93746ce936d6653271b6e98f6df6ee28f64e84 # develop on 2026-03-25
ARG SPACK_ENV_FILE

ENV DEBIAN_FRONTEND=noninteractive

RUN mkdir -p /opt/spack && \
    curl -Ls "https://api.github.com/repos/spack/spack/tarball/$SPACK_SHA" | tar --strip-components=1 -xz -C /opt/spack

ENV PATH="/opt/spack/bin:$PATH"

RUN mkdir -p /opt/spack-packages && \
    curl -Ls "https://api.github.com/repos/spack/spack-packages/tarball/$SPACK_PACKAGES_SHA" | tar --strip-components=1 -xz -C /opt/spack-packages

RUN spack repo remove --scope defaults:base builtin && \
    spack repo add --scope site /opt/spack-packages/repos/spack_repo/builtin

COPY $SPACK_ENV_FILE /spack_environment/spack.yaml

RUN spack external find --all && \
    spack env create ci /spack_environment/spack.yaml && \
    spack -e ci concretize -f && \
    spack -e ci install --jobs $(nproc) --fail-fast --only=dependencies
