// Package main implements a server for Greeter service.
package main

import (
	"context"
	"fmt"
	"io"
	"net"

	spb "github.com/GarrettGutierrez1/simple/simple"
	mpb "github.com/GarrettGutierrez1/simple/simple/messages"
	"google.golang.org/grpc"
	"google.golang.org/grpc/reflection"
)

const (
	port = ":50051"
)

type server struct {
	spb.UnimplementedSimpleServer
}

func (s *server) SimpleUnary(ctx context.Context, in *mpb.SimpleRequest) (*mpb.SimpleReply, error) {
	fmt.Printf("\t---------- Unary: Received: %v\n", in.GetMessage())
	message := "Response"
	fmt.Printf("\t---------- Unary: Sending: %v\n", message)
	return &mpb.SimpleReply{Message: message}, nil
}

func (s *server) SimpleServerStream(in *mpb.SimpleRequest, stream spb.Simple_SimpleServerStreamServer) error {
	fmt.Printf("\t---------- ServerStream: Received: %v\n", in.GetMessage())
	messages := [3]string{"Response 1", "Response 2", "Response 3"}
	for _, message := range messages {
		fmt.Printf("\t---------- ServerStream: Sending: %v\n", message)
		if err := stream.Send(&mpb.SimpleReply{Message: message}); err != nil {
			return err
		}
	}
	return nil
}

func (s *server) SimpleClientStream(stream spb.Simple_SimpleClientStreamServer) error {
	for {
		in, err := stream.Recv()
		if err == io.EOF {
			message := "Response"
			fmt.Printf("\t---------- ClientStream: Sending: %v\n", message)
			return stream.SendAndClose(&mpb.SimpleReply{Message: message})
		}
		if err != nil {
			return err
		}
		fmt.Printf("\t---------- ClientStream: Received: %v\n", in.GetMessage())
	}
}

func (s *server) SimpleBidirectionalStream(stream spb.Simple_SimpleBidirectionalStreamServer) error {
	for {
		in, err := stream.Recv()
		if err == io.EOF {
			return nil
		}
		if err != nil {
			return err
		}
		fmt.Printf("\t---------- BidirectionalStream: Received: %v\n", in.GetMessage())
		prefix := in.GetMessage()
		messages := [3]string{prefix + " Response 1", prefix + " Response 2", prefix + " Response 3"}
		for _, message := range messages {
			fmt.Printf("\t---------- BidirectionalStream: Sending: %v\n", message)
			if err := stream.Send(&mpb.SimpleReply{Message: message}); err != nil {
				return err
			}
		}
	}
}

func main() {
	lis, err := net.Listen("tcp", port)
	if err != nil {
		fmt.Printf("\t---------- Main: Failed to listen: %v\n", err)
		panic(err)
	}
	s := grpc.NewServer()
	spb.RegisterSimpleServer(s, &server{})
	reflection.Register(s)
	if err := s.Serve(lis); err != nil {
		fmt.Printf("\t---------- Main: Failed to serve: %v\n", err)
		panic(err)
	}
}
