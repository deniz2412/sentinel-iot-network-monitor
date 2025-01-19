package models

import "time"

type Device struct {
	MAC      string    `json:"mac"`
	RSSI     float64   `json:"rssi"`
	LastSeen time.Time `json:"lastSeen"`
}
