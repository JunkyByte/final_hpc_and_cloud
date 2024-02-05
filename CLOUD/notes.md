# Notes

Using nextcloud, started from official docker

`docker pull nextcloud`

Launch with

```docker run -d -p 8080:80 nextcloud```
Which exposes port 8080.


```
# I get a warning at first launch

Performance warning
You chose SQLite as database.
SQLite should only be used for minimal and development instances. For production we recommend a different database backend.
If you use clients for file syncing, the use of SQLite is highly discouraged.
```

To skip the setup

```bash
docker run -d \
  -p 8080:80 \
  --name my-nextcloud-container \
  -e SQLITE_DATABASE=nextcloud \
  -e NEXTCLOUD_ADMIN_USER=admin \
  -e NEXTCLOUD_ADMIN_PASSWORD=your_admin_password \
  nextcloud:latest
```


```bash
curl -u admin:your_admin_password -T ~/Downloads/Lecture01.pdf "http://localhost:8080/remote.php/dav/files/admin/"
```