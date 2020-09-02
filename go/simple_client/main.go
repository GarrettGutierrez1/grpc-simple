// Package main implements a client for Greeter service.
package main

import (
	"context"
	"fmt"
	"io"
	"time"

	spb "github.com/GarrettGutierrez1/simple/simple"
	mpb "github.com/GarrettGutierrez1/simple/simple/messages"
	"google.golang.org/grpc"
)

const (
	address = "localhost:50051"
)

func unary(c spb.SimpleClient) {
	ctx, cancel := context.WithTimeout(context.Background(), time.Second)
	defer cancel()
	message := "Request"
	fmt.Printf("\t---------- Unary: Sending: %v\n", message)
	r, err := c.SimpleUnary(ctx, &mpb.SimpleRequest{Message: message})
	if err != nil {
		fmt.Printf("\t---------- Unary: Failed: %v\n", err)
		panic(err)
	}
	fmt.Printf("\t---------- Unary: Received: %v\n", r.GetMessage())
}

func serverStream(c spb.SimpleClient) {
	ctx, cancel := context.WithTimeout(context.Background(), time.Second)
	defer cancel()
	message := "Request"
	stream, err := c.SimpleServerStream(ctx, &mpb.SimpleRequest{Message: message})
	if err != nil {
		fmt.Printf("\t---------- ServerStream: Failed: %v\n", err)
		panic(err)
	}
	for {
		in, err := stream.Recv()
		if err == io.EOF {
			break
		}
		if err != nil {
			fmt.Printf("\t---------- ServerStream: Failed: %v\n", err)
			panic(err)
		}
		fmt.Printf("\t---------- ServerStream: Received: %v\n", in.GetMessage())
	}
}

func clientStream(c spb.SimpleClient) {
	ctx, cancel := context.WithTimeout(context.Background(), 10*time.Second)
	defer cancel()
	stream, err := c.SimpleClientStream(ctx)
	if err != nil {
		fmt.Printf("\t---------- ClientStream: Failed: %v\n", err)
		panic(err)
	}
	messages := [3]string{"Request 1", "Request 2", "Request 3"}
	for _, message := range messages {
		fmt.Printf("\t---------- ClientStream: Sending: %v\n", message)
		if err := stream.Send(&mpb.SimpleRequest{Message: message}); err != nil {
			fmt.Printf("\t---------- ClientStream: Failed: %v\n", err)
			panic(err)
		}
		time.Sleep(1 * time.Second)
	}
	in, err := stream.CloseAndRecv()
	if err != nil {
		fmt.Printf("\t---------- ClientStream: Failed: %v\n", err)
		panic(err)
	}
	fmt.Printf("\t---------- ClientStream: Received: %v\n", in.GetMessage())
}

func bidirectionalStream(c spb.SimpleClient) {
	ctx, cancel := context.WithTimeout(context.Background(), time.Second)
	defer cancel()
	stream, err := c.SimpleBidirectionalStream(ctx)
	if err != nil {
		fmt.Printf("\t---------- BidirectionalStream: Failed: %v\n", err)
		panic(err)
	}
	waitc := make(chan struct{})
	go func() {
		for {
			in, err := stream.Recv()
			if err == io.EOF {
				close(waitc)
				return
			}
			if err != nil {
				fmt.Printf("\t---------- BidirectionalStream: Failed: %v\n", err)
				panic(err)
			}
			fmt.Printf("\t---------- BidirectionalStream: Received: %v\n", in.GetMessage())
		}
	}()
	messages := [3]string{"Request 1", "Request 2", "Request 3"}
	for _, message := range messages {
		fmt.Printf("\t---------- BidirectionalStream: Sending: %v\n", message)
		if err := stream.Send(&mpb.SimpleRequest{Message: message}); err != nil {
			fmt.Printf("\t---------- BidirectionalStream: Failed: %v\n", err)
			panic(err)
		}
	}
	stream.CloseSend()
	<-waitc
}

func main() {
	// Set up a connection to the server.
	conn, err := grpc.Dial(address, grpc.WithInsecure(), grpc.WithBlock())
	if err != nil {
		fmt.Printf("\t---------- Main: Failed to connect: %v\n", err)
		panic(err)
	}
	defer conn.Close()
	c := spb.NewSimpleClient(conn)
	unary(c)
	serverStream(c)
	clientStream(c)
	bidirectionalStream(c)
}
