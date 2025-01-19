package models

import "time"

type Packet struct {
	MAC       string    `json:"mac"`
	RSSI      float64   `json:"rssi"`
	Length    int       `json:"length"`
	Timestamp time.Time `json:"timestamp"`
}

type PacketList struct {
	Packets []Packet `json:"packets"`
}
