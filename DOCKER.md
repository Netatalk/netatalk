# Docker

Netatalk comes with a Dockerfile and entry point script for running a containerized AFP server.

For simplicity, exactly one user, one shared volume, and one Time Machine volume is supported. It is hard coded to output afpd logs to the container's stdout, default info log level.

Make sure you have Docker Engine installed, then build the netatalk container:

```
docker build -t netatalk3 .
```

Alternatively, fetch a pre-built docker container from [Docker Hub](https://hub.docker.com/u/netatalk).

## How to Run

Once the container is ready, run it with `docker run` or `docker compose`.
When running, expose either port 548 for AFP, or use the `host` network driver.
The former option is more secure, but you will have to manually specify the IP address when connecting to the file server.

It is recommended to set up either a bind mount, or a Docker managed volume for persistent storage.
Without this, the shared volume be stored in volatile storage that is lost upon container shutdown.

Sample `docker-compose.yml` with docker managed volume and Zeroconf

```
services:
  netatalk:
    image: netatalk3:latest
    network_mode: "host"
    cap_add:
      - NET_ADMIN
    volumes:
      - afpshare:/mnt/afpshare
      - afpbackup:/mnt/afpbackup
      - /var/run/dbus:/var/run/dbus
    environment:
      - "AFP_USER=atalk"
      - "AFP_PASS=atalk"
volumes:
  afpshare:
  afpbackup:
```

Sample `docker run` command. Substitute `/path/to/share` with an actual path on your file system with appropriate permissions.

```
docker run --rm --network host --cap-add=NET_ADMIN --volume "/path/to/share:/mnt/afpshare" --volume "/path/to/backup:/mnt/afpbackup" --volume "/var/run/dbus:/var/run/dbus" --env AFP_USER=atalk --env AFP_PASS=atalk --name netatalk netatalk3:latest
```

## Constraints

In order to use Zeroconf service discovery, the container requires the "host" network driver and NET_ADMIN capabilities.

Additionally, we rely on the host's DBUS for Zeroconf, achieved with a bind mount for `/var/run/dbus:/var/run/dbus`.

Note that the Dockerfile currently only supports Avahi for Zeroconf; no mDNS support at present.

## Environment Variables

### Mandatory

These are required to set the credentials used to authenticate with the file server.

- `AFP_USER`
- `AFP_PASS`

### Optional

- `AFP_GROUP` <- group that owns the shared volume, and that AFP_USER gets assigned to
- `AFP_UID` <- specify user id of AFP_USER
- `AFP_GID` <- specify group id of AFP_GROUP
- `SERVER_NAME` <- the name of the server reported to Zeroconf
- `SHARE_NAME` <- the name of the file sharing volume
- `AFP_LOGLEVEL` <- the verbosity of logs; default is "info"
- `INSECURE_AUTH` <- when non-zero, enable the "Clear Text" and "Guest" UAMs
- `MANUAL_CONFIG` <- when non-zero, skip netatalk config file modification, allowing you to manually manage them
