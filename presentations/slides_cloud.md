---
marp: true
paginate: false
theme: uncover
math: mathjax
---
<!-- size: 4:3 -->
<!-- class: invert -->
<style>
{
  font-size: 35px
}
</style>


# Cloud Final Project
## Cloud-Based File Storage System

- Donninelli Adriano

---

<!-- paginate: true -->

## Task:

Exploring and deploying a solution for a cloud-based file storage system.

#### Requested Features:

- Seamless file storage for users
- User / Admin roles
- Dockerized deployment system
- Monitoring / Testing of the platform
- Discuss Security
- Address Scalability


---

## Nextcloud software

In order to not reinvent the wheel I used the Nextcloud software stack:

#### Builtin Features:

- Flexible storage solutions
  (Object based / NFS)
- Customizable database backend
- User / Admin management
- Dockerized deployment
- Production ready security system

---

## Security Measures

To improve Nextcloud default configuration
I applied some additional security measures:

- Server side data encryption
- Hardened password requirements
- 2 Factor Authentication
- Enforced HTTPS protocol

---

## Monitoring

Builtin Nextcloud monitoring or
Grafana + Prometheus solution:

![width:850px](../CLOUD/figures/grafana_sample.png)

- Deployment using docker-compose
- Easy interface customization
- IO + Network + Nextcloud monitoring

---

## Testing

Locust python package for load / IO testing:

![width:850px](../CLOUD/figures/locust_20user.png)

- Deployment using docker-compose
- Testing with varying number of users
- Exportable report for analysis

---

## Scalability

I discuss 2 production ready
scalable deployment solutions:

- On-Premise Cluster Deployment
- Cloud Deployment with Autoscaling

---

## On-Premise Cluster

- Deployment of nextcloud on multiple nodes
- Shared database backend / NFS for storage

#### Pros and Cons:

- <span style="color:green;">Hardware control</span>
- <span style="color:green;">Data residency</span>
- <span style="color:green;">Predictable costs</span>
- <span style="color:green;">Custom security policies</span>
- <span style="color:red;">Infrastructure costs</span>
- <span style="color:red;">Adhoc backup system</span>
- <span style="color:red;">System availability</span>

---

## Cloud deployment

- Deployment on cloud provided stack
- Shared or cloud provided
  (Amazon RDS) database backend
- Object based storage system (S3)

#### Pros and Cons:

- <span style="color:green;">Scalability</span>
- <span style="color:green;">Security</span>
- <span style="color:green;">System availability</span>
- <span style="color:green;">Usage-b</span><span style="color:red;">ased costs</span>
- <span style="color:red;">Storage costs</span>

---

## Cost considerations

##### On-premise deployment: 
- High upfront infrastructure costs
  possibly lower operational costs.

##### Cloud provider deployment: 
- Costs accumulate based on usage.

#### Cost Optimization:

- Disable server side encryption.
- Use Reserved instances.