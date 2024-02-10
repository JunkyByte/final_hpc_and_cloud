# Cloud Basic - Final Assignment
# Cloud-Based File Storage System

The `report.pdf` contains all the details for deployment. Please refer to it. 

I paste the `deployment` section here.

## Deployment

The deployment is done through the usage of docker and docker-compose. We leverage docker-compose in order to connect locust to our nextcloud instance. By simple changes to the `docker-compose.yml` it is possible to switch the Database backend and modify the configuration of the Netxcloud instance.

To simply run the docker containers, `cd` to the folder containing the `docker-compose.yml` file and run
```bash
docker-compose up -d
```

As a security measure by default nextcloud will only authorize requests made from localhost. To make locust container requests to be accepted by nextcloud we need to add it to the trusted_domains. This is only necessary to make the load tests run correctly.

```bash
docker exec --user www-data nextcloud /var/www/html/occ config:system:set trusted_domains 1 --value=nextcloud
```

Locust needs a few users to simulate interactions with nextcloud. We can spawn 30 test users for locust by using the convenient:
```bash
sh add_users_to_nextcloud.sh
```

It might take a minute for nextcloud to reload its configuration.

To perform load tests we can now use the locust web ui from `http://localhost:8089/`. I designed a few tasks, contained in `tasks.py`, locust will, for instance, create new files of different sizes (1kb, 1mb and 1gb), read a user file content, upload a text file and request a list of all the files of the user.
> NOTE: The 1 gb file is not included (because is 1gb!) and should be created with
`dd if=/dev/zero of=./test-data/file_1gb bs=1M count=1024`

Once the locust tests are finished it might be convenient to free up all storage space used by the test users with
```bash
sh delete_data_test_users.sh
```