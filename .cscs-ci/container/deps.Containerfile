ARG BASE_IMAGE
FROM BASE_IMAGE

ARG SPACK_SHA
RUN mkdir -p /opt/spack && \
    curl -Ls "https://api.github.com/repos/spack/spack/tarball/$SPACK_SHA" | tar --strip-components=1 -xz -C /opt/spack

ENV PATH="/opt/spack/bin:$PATH"

ARG SPACK_PACKAGES_SHA
RUN mkdir -p /opt/spack-packages && \
    curl -Ls "https://api.github.com/repos/spack/spack-packages/tarball/$SPACK_PACKAGES_SHA" | tar --strip-components=1 -xz -C /opt/spack-packages

RUN spack repo remove --scope defaults:base builtin && \
    spack repo add --scope site /opt/spack-packages/repos/spack_repo/builtin

ARG SPACK_ENV_FILE
COPY $SPACK_ENV_FILE /spack_environment/spack.yaml

RUN spack external find --all && \
    spack env create ci /spack_environment/spack.yaml && \
    spack -e ci concretize -f && \
    spack -e ci install --jobs $(nproc) --fail-fast --only=dependencies
