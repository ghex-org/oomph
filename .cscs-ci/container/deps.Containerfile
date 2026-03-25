FROM ghcr.io/eth-cscs/alps-images:py26.01-alps3-base

ARG SPACK_SHA=develop
ARG SPACK_PACKAGES_SHA=main
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

RUN spack env create ci /spack_environment/spack.yaml && \
    spack -e ci concretize -f && \
    spack -e ci install --jobs $(nproc) --fail-fast --only=dependencies
