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


I create a custom version of the docker image
build with  `docker build -t custom-nextcloud:latest .`

Run with `docker run -d -p 8080:80 --name my-custom-nextcloud custom-nextcloud:latest`