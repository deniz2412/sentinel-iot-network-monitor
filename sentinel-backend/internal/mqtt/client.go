package mqtt

import (
	"github.com/deniz2412/sentinel-iot-network-monitor/internal/config"
	amqp "github.com/rabbitmq/amqp091-go"
	"log"
)

func StartConsumer(cfg config.Config) {
	conn, err := amqp.Dial(cfg.RabbitMQURL)
	if err != nil {
		log.Fatalf("Failed to connect to RabbitMQ: %v", err)
	}
	defer conn.Close()

	channel, err := conn.Channel()
	if err != nil {
		log.Fatalf("Failed to open a channel: %v", err)
	}
	defer channel.Close()

	queue, err := channel.QueueDeclare(
		"sentinel_queue",
		true,
		false,
		false,
		false,
		nil)

	if err != nil {
		log.Fatalf("Failed to declare a queue: %v", err)
		channel.Close()
		conn.Close()
	}

	routingKeys := []string{"sentinel.devices", "sentinel.packets", "sentinel.access-points"}
	for _, routingKey := range routingKeys {
		err = channel.QueueBind(queue.Name, routingKey, "amq.topic", false, nil)
	}

	msgs, err := channel.Consume(
		queue.Name,
		"",
		true,
		false,
		false,
		false,
		nil)

	go HandleMessages(queue.Name, msgs)

	select {} // Keep the consumer running
}
