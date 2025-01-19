package influxdb

import (
	"context"
	"log"
	"time"

	"github.com/deniz2412/sentinel-iot-network-monitor/internal/config"
	"github.com/influxdata/influxdb-client-go/v2"
)

type Client struct {
	client influxdb2.Client
	org    string
	bucket string
}

func NewClient(cfg config.InfluxDBConfig) *Client {
	client := influxdb2.NewClient(cfg.URL, cfg.Token)
	return &Client{
		client: client,
		org:    cfg.Org,
		bucket: cfg.Bucket,
	}
}

func (c *Client) WriteData(ctx context.Context, measurement string, tags map[string]string, fields map[string]interface{}, timestamp time.Time) error {
	writeAPI := c.client.WriteAPIBlocking(c.org, c.bucket)

	point := influxdb2.NewPoint(measurement, tags, fields, timestamp)
	err := writeAPI.WritePoint(ctx, point)
	if err != nil {
		log.Printf("Failed to write data to InfluxDB: %v", err)
		return err
	}
	log.Printf("Data written to InfluxDB: measurement=%s tags=%v fields=%v timestamp=%v", measurement, tags, fields, timestamp)
	return nil
}

func (c *Client) Close() {
	c.client.Close()
}
