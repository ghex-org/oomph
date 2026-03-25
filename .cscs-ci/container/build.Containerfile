ARG DEPS_IMAGE
FROM $DEPS_IMAGE

COPY . /oomph
WORKDIR /oomph

RUN spack -e ci build-env oomph -- cmake -B build -DOOMPH_WITH_TESTING=ON -DMPIEXEC_EXECUTABLE="" -DMPIEXEC_NUMPROC_FLAG="" -DMPIEXEC_PREFLAGS="" -DMPIEXEC_POSTFLAGS="" && \
    spack -e ci build-env oomph -- cmake --build build -j$(nproc)
