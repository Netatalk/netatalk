#
# To test using this Dockerfile
# Build Container Image: docker build -t netatalk-test:latest .

# Init Container and Netatalk: docker run -d --network host --cap-add=NET_ADMIN --volume "<path to local folder>:/mnt/afpshare" --volume "<path to local folder>:/mnt/afpbackup" --env AFP_USER=test --env AFP_PASS=test --env SHARE_NAME='File Sharing' --env AFP_VERSION=7 --env AFP_GROUP=afpusers --env TZ=Europe/London --env INSECURE_AUTH=true --name netatalk-test netatalk-test:latest
# Read Container Logs: docker logs netatalk-test
# Run Netatalk Test: docker exec -it netatalk-test /usr/local/bin/afp_lantest -7 -h localhost -p 548 -u test -w test -s "File Sharing" -n 3
# Stop Container: docker stop netatalk-test
# Start Container: docker start netatalk-test netatalk-test

# Init Container, Run Test, Shutdown: docker run --rm -it --network host --cap-add=NET_ADMIN --volume "<path to local folder>:/mnt/afpshare" --volume "<path to local folder>:/mnt/afpbackup" --env TESTSUITE=lan --env TEST_FLAGS="-n 2" --env AFP_USER=test --env AFP_PASS=test --env SHARE_NAME='File Sharing' --env AFP_VERSION=7 --env AFP_GROUP=afpusers --env TZ=Europe/London --env INSECURE_AUTH=true --name netatalk-test netatalk-test:latest

# List Containers: docker ps -a
# Delete Container Instance: docker rm netatalk-test
# Delete Container Image: docker image rm netatalk-test

ARG RUN_DEPS="\
    acl \
    avahi \
    cups \
    db \
    dbus \
    glib \
    iniparser \
    krb5 \
    libevent \
    libgcrypt \
    linux-pam \
    localsearch \
    mariadb-client \
    mariadb-connector-c \
    openldap \
    sqlite \
    talloc \
    tinysparql \
    tzdata \
    "
ARG BUILD_DEPS="\
    acl-dev \
    avahi-dev \
    bison \
    cups-dev \
    db-dev \
    dbus-dev \
    build-base \
    flex \
    gcc \
    iniparser-dev \
    krb5-dev \
    libevent-dev \
    libgcrypt-dev \
    linux-pam-dev \
    mariadb-dev \
    meson \
    ninja \
    openldap-dev \
    pkgconfig \
    sqlite-dev \
    talloc-dev \
    tinysparql-dev \
    "

FROM alpine:3.22@sha256:4bcff63911fcb4448bd4fdacec207030997caf25e9bea4045fa6c8c44de311d1 AS build

ARG RUN_DEPS
ARG BUILD_DEPS
ENV RUN_DEPS=$RUN_DEPS
ENV BUILD_DEPS=$BUILD_DEPS

RUN apk update \
&&  apk add --no-cache \
    $RUN_DEPS \
    $BUILD_DEPS

WORKDIR /netatalk-code
COPY . .
RUN rm -rf build

RUN meson setup build \
    -Dbuildtype=release \
    -Dwith-afpstats=false \
    -Dwith-appletalk=true \
    -Dwith-dbus-daemon-path=/usr/bin/dbus-daemon \
    -Dwith-dbus-sysconf-path=/etc \
    -Dwith-docs= \
    -Dwith-dtrace=false \
    -Dwith-init-style=none \
    -Dwith-pkgconfdir-path=/etc/netatalk \
    -Dwith-quota=false \
    -Dwith-spotlight=true \
    -Dwith-tcp-wrappers=false \
    -Dwith-testsuite=true \
&&  meson compile -C build

RUN meson install --destdir=/staging/ -C build

FROM alpine:3.22@sha256:4bcff63911fcb4448bd4fdacec207030997caf25e9bea4045fa6c8c44de311d1 AS deploy

ARG RUN_DEPS
ENV RUN_DEPS=$RUN_DEPS

COPY --from=build /staging/ /

RUN apk update \
&&  apk add --no-cache $RUN_DEPS

COPY /contrib/scripts/netatalk_container_entrypoint.sh /entrypoint.sh

WORKDIR /mnt
EXPOSE 548
VOLUME ["/mnt/afpshare", "/mnt/afpbackup"]
CMD ["/entrypoint.sh"]
