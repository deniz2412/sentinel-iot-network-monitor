package models

import "time"

type AccessPoint struct {
	SSID      string    `json:"ssid"`
	MAC       string    `json:"mac"`
	RSSI      float64   `json:"rssi"`
	AuthMode  string    `json:"authMode"`
	Channel   int       `json:"channel"`
	Timestamp time.Time `json:"timestamp"`
}
