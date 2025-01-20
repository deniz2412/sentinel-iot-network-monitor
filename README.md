# Sentinel IoT Network Monitor

## Overview
The **Sentinel IoT Network Monitor** is a real-time network monitoring system designed to capture, process, and visualize network activity. It uses an ESP32 device for data collection and a Go backend service for processing and storage. This project is ideal for tracking Wi-Fi packets, devices, and access points in real-time, with visual analytics provided via Grafana dashboards.

---

## Features
1. **ESP32 Monitoring Unit**:
   - Captures Wi-Fi packets in promiscuous mode.
   - Discovers devices on the network.
   - Scans for nearby Wi-Fi access points.
   - Publishes data to an MQTT broker.

2. **Go Backend Service**:
   - Subscribes to MQTT topics (`packets`, `devices`, `access-points`).
   - Processes incoming data and stores it in InfluxDB.
   - Exposes REST APIs for system health and metrics.

3. **Visualization**:
   - Grafana dashboards to visualize trends, packet data, and device activity.

---

## Architecture
### High-Level Architecture
1. **ESP32 IoT Device**:
   - Gathers network data (packets, devices, access points).
   - Publishes JSON payloads to MQTT topics.

2. **Go Backend**:
   - Consumes MQTT messages.
   - Stores time-series data in InfluxDB.
   - Exposes REST endpoints for monitoring and control.

3. **Grafana**:
   - Visualizes network data through interactive dashboards.

### Workflow
1. **Data Collection**:
   - The ESP32 captures network data in promiscuous mode.
   - Devices and access points are discovered and processed.

2. **Data Transmission**:
   - Captured data is published to MQTT topics (`packets`, `devices`, `access-points`).

3. **Data Processing**:
   - The Go backend subscribes to MQTT topics.
   - Data is parsed and stored in InfluxDB.

4. **Visualization**:
   - Grafana fetches data from InfluxDB for real-time visualization.

---

## Project Structure
```
Sentinel-IoT-Network-Monitor/
├── ESP32/
│   ├── main.c                      # Main entry point for the ESP32 firmware.
│   ├── monitoring_v2.c             # Handles packet, device, and access point monitoring.
│   ├── mqtt_manager.c              # Manages MQTT connections and publishing.
│   ├── config_manager.c            # Manages configuration using SPIFFS.
├── Backend/
│   ├── cmd/main.go                 # Entry point for the Go backend.
│   ├── internal/
│   │   ├── mqtt/client.go          # Handles MQTT subscriptions.
│   │   ├── influxdb/client.go      # Writes data to InfluxDB.
│   │   ├── api/server.go           # Sets up HTTP server for REST API.
│   └── pkg/models/                 # Defines data models.
├── configs/
│   └── config.json                 # Configuration file for the ESP32.
├── Docker/
│   ├── docker-compose.yaml         # Local deployment setup with MQTT broker and InfluxDB.
│   ├── Dockerfile                  # Backend containerization.
└── README.md                       # Project documentation.
```

---

## Installation
### Prerequisites
- ESP32 development board.
- Docker and Docker Compose.
- Go (for backend development).
- Grafana (for visualization).

### Steps
1. **Set Up ESP32**:
   - Clone the repository.
   - Configure Wi-Fi and MQTT details in `config.json`.
   - Build and flash the ESP32 firmware.

2. **Deploy Backend**:
   - Use Docker Compose to deploy InfluxDB, MQTT, and the Go backend.
     ```bash
     docker-compose up -d
     ```

3. **Set Up Grafana**:
   - Add InfluxDB as a data source.
   - Import Grafana dashboards from `dashboards/`.

---

## Future Goals and Improvements
1. **Security Enhancements**:
   - Add TLS for secure MQTT communication.
   - Implement API authentication.

2. **Advanced Analytics**:
   - Use machine learning to detect network anomalies.
   - Implement alerting for unusual packet or device activity.

3. **Scalability**:
   - Optimize the Go backend for higher throughput.
   - Support additional data sources and IoT devices.

4. **Improved Device Discovery**:
   - Enhance device monitoring with ARP and ping integrations.
   - Add support for identifying vendor details.

---

## Contributing
We welcome contributions to improve the Sentinel IoT Network Monitor. Please follow these steps:
1. Fork the repository.
2. Create a feature branch.
3. Commit changes with descriptive messages.
4. Submit a pull request.

---

## License
This project is licensed under the MIT License. See `LICENSE` for more details.

---

## Contact
For questions or feedback, please reach out to the project maintainer.
