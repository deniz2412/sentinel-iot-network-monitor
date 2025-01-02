FROM golang:1.23-alpine as builder

WORKDIR /app
COPY . .
RUN go mod tidy
RUN go build -o backend .
FROM alpine:latest
WORKDIR /root/
COPY --from=builder /app/backend .
EXPOSE 8080
CMD ["./backend"]
