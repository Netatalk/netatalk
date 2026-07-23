# Performance test image: the netatalk testsuite image plus the Linux
# traffic-control tooling the netns rig needs.
#
# Build (from the repository root):
#   docker build -f distrib/docker/testsuite_alp.Dockerfile -t netatalk-testsuite .
#   docker build -f distrib/docker/perftest/perftest.Dockerfile \
#       --build-arg BASE_IMAGE=netatalk-testsuite -t netatalk-perftest .

ARG BASE_IMAGE=netatalk-testsuite

FROM ${BASE_IMAGE}

RUN apk update \
    && apk add --no-cache \
        ethtool \
        iproute2 \
        iproute2-tc \
    && rm -rf /var/cache/apk/*

COPY distrib/docker/perftest/entrypoint_perftest.sh /entrypoint_perftest.sh

ENTRYPOINT ["/bin/sh", "/entrypoint_perftest.sh"]
