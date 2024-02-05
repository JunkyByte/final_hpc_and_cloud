from locust import HttpUser, task, between
from requests.auth import HTTPBasicAuth

class NextcloudUser(HttpUser):
    auth = None
    wait_time = between(2, 5)

    def on_start(self):
        self.auth = HTTPBasicAuth('admin', 'admin')

    @task(1)
    def propfind(self):
        self.client.request("PROPFIND", "/remote.php/dav/files/admin/", auth=self.auth)

    @task(2)
    def navigate(self):
        self.client.get("/remote.php/dav/files/admin/Readme.md", auth=self.auth)

    @task
    def upload_file(self):
        with open("/test-data/test_locust_file.txt", "rb") as file:
            self.client.put("/remote.php/dav/files/admin/test_locust_file.txt", data=file, auth=self.auth)