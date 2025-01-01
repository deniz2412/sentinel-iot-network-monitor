# iot-network-monitor
IoT network monitoring project
Sentinel/
├── backend/
│   ├── cmd/
│   │   ├── main.go                   # Entry point for the backend service
│   │   ├── mqtt_subscriber.go        # MQTT subscription logic
│   │   ├── data_processor.go         # Data processing and validation logic
│   │   └── db_writer.go              # Logic for writing to InfluxDB
│   ├── pkg/
│   │   ├── mqtt/
│   │   │   └── mqtt.go               # MQTT client implementation
│   │   ├── db/
│   │   │   └── influxdb.go           # InfluxDB client implementation
│   │   └── api/
│   │       └── api.go                # REST API handlers
│   ├── config/
│   │   └── config.yaml               # Configuration file for backend service
│   ├── Dockerfile                    # Dockerfile for backend service
│   └── go.mod                        # Go module dependencies
│
├── mqtt-broker/
│   ├── mosquitto.conf                # MQTT broker configuration
│   ├── Dockerfile                    # Dockerfile for MQTT broker
│   └── README.md                     # Instructions for configuring the MQTT broker
│
├── grafana/
│   ├── dashboards/
│   │   └── default_dashboard.json    # Default Grafana dashboard JSON
│   ├── datasources/
│   │   └── influxdb.yaml             # Grafana data source configuration
│   ├── Dockerfile                    # Dockerfile for Grafana service
│   └── README.md                     # Instructions for setting up Grafana
│
├── influxdb/
│   ├── init/
│   │   └── influxdb_setup.sh         # Script to initialize InfluxDB
│   ├── Dockerfile                    # Dockerfile for InfluxDB service
│   └── README.md                     # Instructions for InfluxDB configuration
│
├── esp32/
│   ├── src/
│   │   ├── main.cpp                  # Firmware code for ESP32
│   │   ├── mqtt_client.cpp           # MQTT client implementation
│   │   ├── wifi_monitor.cpp          # Wi-Fi metrics collection
│   │   └── config.h                  # Configuration file (MQTT topics, Wi-Fi credentials)
│   ├── docs/
│   │   └── hardware_setup.md         # Instructions for ESP32 setup
│   └── README.md                     # Firmware overview
│
├── k8s/                              # Kubernetes manifests
│   ├── mqtt-deployment.yaml          # Deployment for MQTT broker
│   ├── backend-deployment.yaml       # Deployment for backend service
│   ├── influxdb-deployment.yaml      # Deployment for InfluxDB
│   ├── grafana-deployment.yaml       # Deployment for Grafana
│   └── README.md                     # Kubernetes setup instructions
│
├── docker-compose.yaml               # Docker Compose file for local deployment
├── README.md                         # Project overview and instructions
├── LICENSE                           # License information
└── .gitignore                        # Git ignore rules
