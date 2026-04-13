#/usr/bin/env bash
#
# Helper script to mount a separate directory for the Testing/Temporary
# directory for each process when running ctest within slurm.

set -x
mkdir -p "/tmp/Testing/Temporary-${SLURM_PROCID}"
unshare --mount --map-root-user \
    bash -c \
    "mount --bind /tmp/Testing/Temporary-${SLURM_PROCID} $PWD/Testing/Temporary && $@"
