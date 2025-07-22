# Docker

Netatalk comes with a Dockerfile and entry point script for running a containerized AFP server and AppleTalk services.

Out of the box, exactly two users, and two shared volumes are supported in this container.
One of the shared volumes is a backup volume by default.
If you need a different setup, please use the manual configuration option.

Make sure you have a container runtime installed, then build the netatalk container:

```shell
docker build -t netatalk:latest .
```

Alternatively, pull a pre-built Docker image from [Docker Hub](https://hub.docker.com/u/netatalk).

## How to Run

Once you have the netatalk image on your machine run it with `docker`, `podman` or equivalent container runtime.

The easiest way to enable full functionality including Zeroconf service discovery and AppleTalk
is to use the `host` network driver.

For a hardened deployment, use the `bridge` network driver and expose port 548 for AFP.
However, Zeroconf service discovery may not work and you will have to manually specify
the IP address in the client when connecting to the file server.

It is recommended to set up either a bind mount, or a Docker managed volume for persistent storage.
Without this, the shared volume be stored in volatile storage that is lost upon container shutdown.

For Docker Compose, you can use the sample [docker-compose.yml](https://github.com/Netatalk/netatalk/blob/main/docker-compose.yml)
file that is distributed with this source code.

Below follows a sample `docker run` command. Substitute `/path/to/share` with an actual path
on your file system with appropriate permissions, `AFP_USER` and `AFP_PASS` with the appropriate user and password,
and `ATALKD_INTERFACE` with the network interface to use for AppleTalk.

You also need to set the timezone with `TZ` to the [IANA time zone ID](https://nodatime.org/TimeZones)
for your location, in order to get the correct time synchronized with the Timelord time server.

```shell
docker run --rm \
  --network host \
  --cap-add=NET_ADMIN \
  --volume "/path/to/share:/mnt/afpshare" \
  --volume "/path/to/backup:/mnt/afpbackup" \
  --volume "/var/run/dbus:/var/run/dbus" \
  --env AFP_USER= \
  --env AFP_PASS= \
  --env AFP_GROUP=afpusers \
  --env ATALKD_INTERFACE=eth0 \
  --env TZ=Europe/Stockholm \
  --name netatalk netatalk:latest
```

## Constraints

The most straight forward way to enable Zeroconf service discovery as well as the AppleTalk transport layer,
is to use the `host` network driver and `NET_ADMIN` capabilities.

Additionally, we rely on the host's D-Bus for Zeroconf, achieved with a bind mount such as `/var/run/dbus:/var/run/dbus`.
The left hand side of the bind mount is the host machine, and the right hand side is the container.
The host machine path may have to be changed to match the location of D-Bus on the host machine.

On certain host OSes, notably Ubuntu: if the Apparmor security policy restricts D-Bus messages,
enable the `unconfined` security option.

Example for the docker compose yaml configuration file:

```yaml
    security_opt:
      - apparmor=unconfined
```

See the [Docker AppArmor security profiles documentation](https://docs.docker.com/engine/security/apparmor/)
for further details.

The container is hard coded to output `afpd` (the Netatalk file server daemon) logs to the container's stdout,
with default log level `info`. Logs from the AppleTalk daemons are sent to the syslog.

Spotlight compatible search is currently not supported in the production container
due to constraints when running file system indexing on the D-Bus.

## MySQL CNID Backend

The MySQL CNID backend is an alternative to the default Berkeley DB CNID backend which offers better scalability.

Here follows a Docker Compose example to run a container network with MariaDB as the database for the MySQL CNID backend,
plus a web interface to administer the database for good measure.

Set `AFP_CNID_SQL_PASS` and `MARIADB_ROOT_PASSWORD` to the same password.

```yaml
services:
  netatalk:
    image: netatalk:latest
    networks:
      - afp_network
    ports:
      - "548:548"
    volumes:
      - afpshare:/mnt/afpshare
      - afpbackup:/mnt/afpbackup
      - /var/run/dbus:/var/run/dbus
    environment:
      - AFP_USER=atalk1
      - AFP_USER2=atalk2
      - AFP_PASS=
      - AFP_PASS2=
      - AFP_GROUP=afpusers
      - AFP_CNID_BACKEND=mysql
      - AFP_CNID_SQL_HOST=cnid_db
      - AFP_CNID_SQL_PASS=
    depends_on:
      - cnid_db

  cnid_db:
    image: mariadb:latest
    restart: always
    networks:
      - afp_network
    volumes:
      - cnid_db_data:/var/lib/mysql
    environment:
      - MARIADB_ROOT_PASSWORD=

  adminer:
    image: adminer:latest
    restart: always
    networks:
      - afp_network
    ports:
      - 8081:8080

volumes:
  afpshare:
    name: afpshare
  afpbackup:
    name: afpbackup
  cnid_db_data:
    name: cnid_db_data

networks:
  afp_network:
    driver: bridge
```

## Webmin Module

Netatalk's Webmin module is distributed as a separate container image.
The module allows you to administer the Netatalk configuration via a web interface.
Here follows an example Docker Compose configuration to run the module in a container.

Note that since the netatalk daemons run in a separate container, we cannot control
the services directly. Instead, activate polling of changes to the afp.conf
configuration file. Set `AFP_CONFIG_POLLING` to the number of seconds to wait between
polling attempts.

```yaml
services:
  netatalk:
    image: netatalk:latest
    networks:
      - afp_network
    ports:
      - "548:548"
    volumes:
      - afpshare:/mnt/afpshare
      - afpbackup:/mnt/afpbackup
      - afpconf:/etc/netatalk
      - /var/run/dbus:/var/run/dbus
    environment:
      - AFP_USER=atalk1
      - AFP_USER2=atalk2
      - AFP_PASS=
      - AFP_PASS2=
      - AFP_GROUP=afpusers
      - AFP_CONFIG_POLLING=5
      - MANUAL_CONFIG=1

  webmin:
    image: netatalk_webmin_module:latest
    networks:
      - afp_network
    ports:
      - "10000:10000"
    volumes:
      - afpconf:/etc/netatalk
      - /var/run/docker.sock:/var/run/docker.sock
    environment:
      - WEBMIN_USER=admin
      - WEBMIN_PASS=
    depends_on:
      - netatalk

volumes:
  afpshare:
    name: afpshare
  afpbackup:
    name: afpbackup
  afpconf:
    name: afpconf

networks:
  afp_network:
    driver: bridge
```

## Printing

The CUPS administrative web app is running on port 631 in the container,
which is exposed to the host machine by default when using the `host` network driver.
This is used to configure CUPS compatible printers for printing from an old Mac or Apple IIGS.

You may have to restart papd (or the entire container) after adding a CUPS printer
for it to be picked up as an AppleTalk printer.

## Environment Variables

### Mandatory

These are required to set the credentials used to authenticate with the file server.

| Variable  | Description                                    |
|-----------|------------------------------------------------|
| AFP_USER  | Primary user of the shared volumes             |
| AFP_PASS  | Password to authenticate with the primary user |
| AFP_GROUP | Group that owns the shared volume dirs         |

### Mandatory for AppleTalk

| Variable         | Description                                |
|------------------|--------------------------------------------|
| ATALKD_INTERFACE | The network interface to use for AppleTalk |

### Optional

#### String

| Variable          | Description                                                            |
|-------------------|------------------------------------------------------------------------|
| AFP_UID           | Specify user id of AFP_USER                                            |
| AFP_GID           | Specify group id of AFP_GROUP                                          |
| AFP_USER2         | Username for the secondary user                                        |
| AFP_PASS2         | Password for the secondary user                                        |
| SERVER_NAME       | The name of the server (AFP and Zeroconf)                              |
| SHARE_NAME        | The name of the primary shared volume                                  |
| SHARE_NAME2       | The name of the secondary shared (Time Machine) volume                 |
| AFP_LOGLEVEL      | The verbosity of logs; default is "info"                               |
| AFP_MIMIC_MODEL   | Use a custom modern AFP icon, such as `Tower` or `RackMount`           |
| AFP_LEGACY_ICON   | Use a custom legacy AFP icon, such as `daemon` or `sdcard`             |
| ATALKD_OPTIONS    | A string with options to append to atalkd.conf                         |
| AFP_CNID_BACKEND  | The backend to use for the CNID database: `bdb` or `mysql`             |
| AFP_CNID_SQL_HOST | The hostname or IP address of the CNID SQL server                      |
| AFP_CNID_SQL_USER | The username to use when connecting to the CNID SQL server             |
| AFP_CNID_SQL_PASS | The password to use when connecting to the CNID SQL server             |
| AFP_CNID_SQL_DB   | The name of the designated database in the SQL server                  |
| TZ                | The [timezone](https://nodatime.org/TimeZones) to use in the container |

#### Boolean

Set this environment variable to a non-zero value to enable, ex. "1"

| Variable            | Description                                                                                    |
|---------------------|------------------------------------------------------------------------------------------------|
| AFP_DROPBOX         | Enable dropbox mode; secondary user is guest with read only access to the second shared volume |
| AFP_EXTMAP          | Enable mapping of filename extension to Classic Mac OS type/creator                            |
| INSECURE_AUTH       | Enable the "ClearTxt" and "Guest" UAMs                                                         |
| DISABLE_TIMEMACHINE | The secondary shared volume is a regular volume, not a backup volume                           |
| DISABLE_SPOTLIGHT   | Spotlight compatible indexing is disabled (currently not used)                                 |
| MANUAL_CONFIG       | Enable manual management of configurations; overrides most other options                       |
