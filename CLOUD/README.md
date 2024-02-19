# Cloud Basic - Final Assignment
# Cloud-Based File Storage System

The `Donninelli_report.pdf` contains all the details for deployment. Please refer to it. 

I paste the `deployment` section here.

## Deployment

The deployment is done through the usage of docker and docker-compose. We leverage docker-compose in order to connect locust / grafana and prometheus to our nextcloud instance. By simple changes to the `docker-compose.yml` it is possible to switch the Database backend and modify the configuration of Netxcloud.

To run the docker containers, `cd` to the folder containing the `docker-compose.yml` file and run
```bash
docker-compose up -d
```

### Monitoring with Grafana

In order to monitor the performance of our system I setup 3 additional containers, one for Grafana, a common monitoring visualization tool, one for prometheus which works as a data source for Grafana and last an open source library which acts as a bridge between the Nextcloud instance and prometheus, called `nextcloud-exporter` (https://github.com/xperimental/nextcloud-exporter).

The docker compose I provide contains a working setup required to connect these containers. Some customization (for example changing the passwords to non default ones) can be applied in order to secure the system.

The only thing we need to setup manually is the Grafana data source and dashboard. In order to do so go to grafana web interface `http://localhost:3000/` and add a new data source for Prometheus.

Set the Prometheus server url to `http://prometheus:9090` (the name of the docker container and the port exposed). Save and test if everything works correctly. Last step is to add a new dashboard. I provide a sample one, `dashboard_sample.json` which can be used as a starting point, simply click on `Create dashboard`, import the json file and select the data source previously created.

The dashboard is now connected and various metrics are visibile. Many more metrics are exported by prometheus and the dashboard can be further customized.

![grafana](./figures/grafana_sample.png)

### Testing with locust

As a security measure by default nextcloud will only authorize requests made from localhost. In order to make the requests from the locust container be accepted we need to add it to the `trusted_domains`. This is only necessary in order to run load tests correctly.

```bash
docker exec --user www-data nextcloud /var/www/html/occ config:system:set trusted_domains 1 --value=nextcloud
```

Locust needs a few users to simulate interactions with nextcloud. We can spawn 30 test users for it by using the convenient:
```bash
sh add_users_to_nextcloud.sh
```

It might take a minute for nextcloud to reload its configuration and accept requests.

To perform load tests we can now use the locust web ui from `http://localhost:8089/`.

I designed a few tasks, contained in `tasks.py`. With these tasks Locust will, for instance, create new files of different sizes (1kb, 1mb and 1gb), read a user file content, upload a text file and request a list of all the files owned by the user.

> NOTE: The 1 gb file is not included (because is 1gb!) and should be created with
`dd if=/dev/zero of=./test-data/file_1gb bs=1M count=1024`

Once the locust tests are finished it might be convenient to free up all the storage space used by the test users. This can be done with the provided bash script:
```bash
sh delete_data_test_users.sh
```