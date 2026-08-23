# The CI runner's toolchain, reproduced locally.
#
# ubuntu-24.04 ships gcc 13.3.0-6ubuntu2~24.04.1 -- the exact build string
# the GitHub runner reports. The development box is Linux Mint 21.2 (Ubuntu
# 22.04 base) with gcc 11.4 and no clang at all, so it is two gcc releases
# behind CI and has zero local coverage of the two clang jobs.
#
# That gap was not theoretical. An uninitialised read in swevents.c passed
# all 14 gates locally and failed CI, and a stray gcc-only
# __attribute__((aligned(64))) broke the MSVC job while gcc accepted it
# silently. Each cost a full push/wait/read cycle to diagnose.
FROM ubuntu:24.04
RUN apt-get update -qq \
 && DEBIAN_FRONTEND=noninteractive apt-get install -y -qq --no-install-recommends \
      gcc clang make python3 binutils libc6-dev \
 && rm -rf /var/lib/apt/lists/*
WORKDIR /src
