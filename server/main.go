package main

import (
	"log"
	"./server/internal/server"
)

func main() {
	log.Println("Starting listening on port 8080")
	srv := server.NewHTTPServer(":8080")
	log.Fatal(srv.ListenAndServe())
}
