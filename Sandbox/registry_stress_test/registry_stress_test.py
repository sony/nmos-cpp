import argparse
import requests
import uuid
from concurrent.futures import ThreadPoolExecutor, as_completed

# Configuration
# API_URL = "http://172.22.128.1:20000/x-nmos/registration/v1.3/resource"
HEADERS = {
    "Content-Type": "application/json"
    #"Authorization": "Bearer YOUR_API_TOKEN"  # Optional
}

# Payload Parameters
# NUM_CLIENTS = 5000  # Number of simulated clients (concurrent threads)

def parse_arguments():
    parser = argparse.ArgumentParser(description='NMOS RDS Registration Stress Test')
    parser.add_argument('--host', default="127.0.0.1",
                        help="host or IPs of the API under test")
    parser.add_argument('--port', default=7000, type=int,
                        help="port of the API under test")
    parser.add_argument('--num_clients', default=5000, type=int,
                        help="number of simulated clients")

    return parser.parse_args()

# Generate a single registration payload
def generate_payload(index):
    return {
        "type": "node",
        "data": {
            "api": {
                "endpoints": [
                    {
                        "authorization": False,
                        "host": "172.22.143.162",
                        "port": 10000,
                        "protocol": "http"
                    }
                ],
                "versions": [
                    "v1.0",
                    "v1.1",
                    "v1.2",
                    "v1.3"
                ]
            },
            "caps": {},
            "clocks": [
                {
                    "name": "clk0",
                    "ref_type": "internal"
                }
            ],
            "description": "",
            "hostname": "172.22.143.162",
            "href": "http://172.22.143.162:10000/",
            "id": str(uuid.uuid4()), # generate a new UUID for each payload
            "interfaces": [
                {
                    "chassis_id": "00-15-5d-d4-36-9b",
                    "name": "eth0",
                    "port_id": "00-15-5d-d4-36-9b"
                }
            ],
            "label": f"Test Node {index}",
            "services": [],
            "tags": {},
            "version": "1752755727:587283463"
        }
    }

# Function to simulate a client POSTing a payload
def simulate_client(index, api_url):
    payload = generate_payload(index)
    try:
        response = requests.post(api_url, headers=HEADERS, json=payload)
        print(f"Client {index} - Status {response.status_code} - Response: {response.text}")
    except requests.exceptions.RequestException as e:
        print(f"Client {index} - Request failed: {e}")

# Simulate multiple clients in parallel
def simulate_multiple_clients(host, port, num_clients):
    api_url = f"http://{host}:{port}/x-nmos/registration/v1.3/resource"
    with ThreadPoolExecutor(max_workers=num_clients) as executor:
        futures = [executor.submit(simulate_client, i+1, api_url) for i in range(num_clients)]
        for future in as_completed(futures):
            pass

# Run the simulation
if __name__ == "__main__":
    print("*** Registry stress test ***")
    args = parse_arguments();
    simulate_multiple_clients(args.host, args.port, args.num_clients)