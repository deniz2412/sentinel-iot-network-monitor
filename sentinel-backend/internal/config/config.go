package config

import (
	"log"

	"github.com/spf13/viper"
)

type InfluxDBConfig struct {
	URL    string `mapstructure:"url"`
	Token  string `mapstructure:"token"`
	Org    string `mapstructure:"org"`
	Bucket string `mapstructure:"bucket"`
}

type Config struct {
	RabbitMQURL string         `mapstructure:"rabbitmq_url"`
	QueueName   string         `mapstructure:"queue_name"`
	HTTPPort    string         `mapstructure:"http_port"`
	InfluxDB    InfluxDBConfig `mapstructure:"influxdb"`
}

func LoadConfig() Config {
	viper.SetConfigName("config")
	viper.SetConfigType("yaml")
	viper.AddConfigPath("./configs")
	viper.AddConfigPath(".") // Current directory as fallback

	if err := viper.ReadInConfig(); err != nil {
		log.Printf("Failed to read configuration file, falling back to defaults: %v", err)
	}

	var cfg Config
	if err := viper.Unmarshal(&cfg); err != nil {
		log.Fatalf("Failed to parse configuration: %v", err)
	}

	// Fallback to defaults if config values are missing
	if cfg.RabbitMQURL == "" {
		cfg.RabbitMQURL = "amqp://admin:admin@localhost:5672/"
	}
	
	if cfg.HTTPPort == "" {
		cfg.HTTPPort = "8080"
	}

	log.Printf("Loaded configuration: %+v", cfg)
	return cfg
}
