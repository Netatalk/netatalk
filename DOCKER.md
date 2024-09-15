# Docker

Netatalk comes with a Dockerfile and entry point script for running a containerized AFP server.

For simplicity, exactly one user, one shared volume, and one Time Machine volume is supported. It is hard coded to output afpd logs to the container's stdout, default info log level.

Make sure you have Docker Engine installed, then build the netatalk container:

```
docker build -t netatalk:latest .
```

Alternatively, pull a pre-built Docker container from [Docker Hub](https://hub.docker.com/u/netatalk).

## How to Run

Once the container is ready, run it with `docker run` or `docker compose`.
When running, expose either port 548 for AFP, or use the `host` network driver.
The former option is more secure, but you will have to manually specify the IP address when connecting to the file server.

It is recommended to set up either a bind mount, or a Docker managed volume for persistent storage.
Without this, the shared volume be stored in volatile storage that is lost upon container shutdown.

You can use the sample [docker-compose.yml](https://github.com/Netatalk/netatalk/blob/main/docker-compose.yml) that is distributed with this source code.

Below follows a sample `docker run` command. Substitute `/path/to/share` with an actual path on your file system with appropriate permissions, and `AFP_USER` and `AFP_PASS` with the appropriate user and password, and `ATALKD_INTERFACE` with the network interface to use for AppleTalk.

You also need to set the timezone with `TZ` to the [IANA time zone ID](https://nodatime.org/TimeZones) for your location, in order to get the correct time synchronized with the Timelord time server.

```
docker run --rm \
  --network host \
  --cap-add=NET_ADMIN \
  --volume "/path/to/share:/mnt/afpshare" \
  --volume "/path/to/backup:/mnt/afpbackup" \
  --volume "/var/run/dbus:/var/run/dbus" \
  --env AFP_USER= \
  --env AFP_PASS= \
  --env AFP_GROUP=afpusers \
  --env ATALKD_INTERFACE= \
  --env TZ= \
  --name netatalk netatalk:latest
```

## Constraints

In order to use Zeroconf service discovery and the AppleTalk transport layer, the container requires the "host" network driver and NET_ADMIN capabilities.

Additionally, we rely on the host's DBUS for Zeroconf, achieved with a bind mount for `/var/run/dbus:/var/run/dbus`.

## Printing

The CUPS administrative web app is running on port 631 in the container, which is exposed to the host machine by default when using the `host` network driver. This is used to configure CUPS compatible printers for printing from an old Mac or Apple IIGS.

You may have to restart papd (or the entire container) after adding a CUPS printer for it to be picked up as an AppleTalk printer.

## Environment Variables

### Mandatory

These are required to set the credentials used to authenticate with the file server.

| Variable | Description |
| --- | --- |
| `AFP_USER` | Username to authenticate with the file server |
| `AFP_PASS` | Password to authenticate with the file server |
| `AFP_GROUP` | Group that owns the shared volume dirs |

### Mandatory for AppleTalk

| Variable | Description |
| --- | --- |
| `ATALKD_INTERFACE` | The network interface to use for AppleTalk |
| `TZ` | The timezone to use for the container; must be a [IANA time zone ID](https://nodatime.org/TimeZones) |

### Optional

| Variable        | Description                                                    |
|-----------------|----------------------------------------------------------------|
| `AFP_UID`       | Specify user id of `AFP_USER`                                  |
| `AFP_GID`       | Specify group id of `AFP_GROUP`                                |
| `SERVER_NAME`   | The name of the server reported to Zeroconf                    |
| `SHARE_NAME`    | The name of the file sharing volume                            |
| `AFP_LOGLEVEL`  | The verbosity of logs; default is "info"                       |
| `INSECURE_AUTH` | When non-zero, enable the "ClearTxt" and "Guest" UAMs          |
| `MANUAL_CONFIG` | When non-zero, enable manual management of config files        |

