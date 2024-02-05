# Cloud-Based File Storage System Project Report

## Introduction

This project aims to address the need for a cloud-based file storage system that enables users to seamlessly upload, download, and delete files while ensuring the privacy of individual storage spaces. In order to not reinvent the wheel and to pursue efficiency, Nextcloud has been selected for its robust features and ease of deployment using Docker containers.

### Choice of Nextcloud

Nextcloud stands out as a suitable choice for this project due to its comprehensive set of functionalities tailored for file management in a cloud environment. It provides a user-friendly interface, robust security measures, and the ability to scale seamlessly. Moreover, the availability of a Docker container for Nextcloud simplifies the deployment process, ensuring a quick and straightforward implementation.

## Nextcloud features

### User Authentication and Authorization with Nextcloud

Nextcloud seamlessly facilitates user authentication and authorization with its built-in features, aligning perfectly with the project requirements:

- **User Registration and Login:** Nextcloud offers a straightforward user registration process, allowing users to sign up, log in, and log out effortlessly. The intuitive interface enhances user experience during these essential interactions.

- **Role-Based Access Control:** Nextcloud inherently supports different user roles, such as regular users and admins. This built-in role-based access control ensures that each user is assigned the appropriate privileges.

- **Private Storage for Regular Users:** Regular users benefit from Nextcloud's default configuration, which allocates a private storage space for each individual, with a limit defined by a Quota.

- **Admin Management Capabilities:** Nextcloud provides an admin dashboard with features to add, remove, and modify user accounts, streamlining the administrative tasks associated with user management.

By default, upon successful authentication, Nextcloud issues an access token that clients will use for all future HTTP requests. This access token should not be stored on any system other than the client requesting it. The user password is also stored encrypted in the Nextcloud database.  

In order to allow users to register by themself using the webinterface the `Registration` app can be installed and enabled, providing the usual email based signup with verification link. In a production setting we would have to setup a server email to provide the verification links and other mail based services.

### Security measures

Nextcloud comes with a set of secure defaults to address the security of the system. In order to implement secure file storage, if the user data is sensible, Nextcloud allows to enable file encryption on the server side. This will reduce the performance of the system but will prevent an intruder that gains access to the data to read it. Enabling file encryption all files can still be shared by a user using the Nextcloud interface but won't be sharable directly from the remote server because they will be encrypted.

To enable encryption from the web interface simply login as admin, search for the Default Encryption module app and enable it. Then to enable file encryption:

`Administration Settings -> Administration Security -> Server-side encryption`

Or from command line (using docker)
```bash
docker exec --user www-data my-nextcloud-container /var/www/html/occ encryption:enable
echo "yes" | docker exec -i --user www-data my-nextcloud-container /var/www/html/occ encryption:encrypt-all
```

The system won't be secure unless our users have non trivial passwords. From the same configuration page we can harden the password policy by requiring numeric characters or symbols. We can also require the password to be changed every few days.
To prevent unauthorized access and secure user authentication we can also enable 2 factor authentication for all user access.

The clients and the server interact through HTTP protocol, enforcing HTTPS protocol is mandatory in production servers to prevent man in the middle attacks and data snooping.

### Database solutions

During development I opted for a simple sqlite database backend for Nextcloud metadata. In a production environment a more powerful backend should be chosen like PostgreSQL or MariaDB, both supported by Nextcloud. Different databases offer different performances but also different replication systems, which are important when we design the scaling and backup solutions of our platform.    

### Storage solutions

By default Nextcloud stores files in the local file system, which is convenient for a small deployment. In more complex environments it offers flexible solutions for both Object based storage like Amazon S3 and network distributed file systems like NFS. I will discuss in the scaling section the most convenient solution based on our requirements.

### Monitoring

In order to monitor the system performances it is convenient to integrate a monitoring system like Grafana backed up by Prometheus for the data collection. These systems are flexible and can be dockerized for easy setup, allowing to monitor the performances also of local disks and network operations.

### Testing

In order to test the performance of the system in terms of load and IO operations it can be convenient to design stress tests and see how our infrastructure reacts. A python package called `locust` can be used to simulate user interactions with the platform, allowing to gather performance metrics.

## Deployment

The deployment is done through the usage of docker and docker-compose. We leverage docker-compose in order to connect locust to our nextcloud instance.

To simply run the docker containers, assuming you are in the folder containing the `docker-compose.yml` file simply run
```bash
docker-compose up -d
```

By default nextcloud will only address requests made from localhost. To make locust container requests to be accepted by nextcloud we need to add it to the trusted_domains. This is only necessary to make load tests run correctly.

```bash
docker exec --user www-data nextcloud /var/www/html/occ config:system:set trusted_domains 1 --value=nextcloud
```

Locust needs a few users to interact with nextcloud. We can 30 locust test users by running the convenient:
```bash
sh add_users_to_nextcloud.sh
```

It might take a minute for nextcloud to reload its configuration.
To perform load tests we can now use locust web ui from `http://localhost:8089/`.

---- TODO
Once the tests are finished we can free up all space used by the test users with
```bash
sh delete_data_test_users.sh
```