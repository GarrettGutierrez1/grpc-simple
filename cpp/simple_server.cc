#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>

#include "simple/simple.grpc.pb.h"

class SimpleServiceImpl final : public simple::Simple::Service {
    grpc::Status SimpleUnary(grpc::ServerContext* context, const simple::messages::SimpleRequest* request, simple::messages::SimpleReply* response) override {
        std::cout << "\t---------- Unary: Received: " << request->message() << std::endl;
        std::string message = "Response";
        std::cout << "\t---------- Unary: Sending: " << message << std::endl;
        response->set_message(message.c_str());
        return grpc::Status::OK;
    }
    grpc::Status SimpleServerStream(grpc::ServerContext* context, const simple::messages::SimpleRequest* request, grpc::ServerWriter<simple::messages::SimpleReply>* writer) {
        std::cout << "\t---------- ServerStream: Received: " << request->message() << std::endl;
        const std::string messages[] = {"Response 1", "Response 2", "Response 3"};
        simple::messages::SimpleReply reply;
        for(const std::string& message : messages) {
            std::cout << "\t---------- ServerStream: Sending: " << message << std::endl;
            reply.set_message(message.c_str());
            if(!writer->Write(reply)) {
                return grpc::Status(grpc::StatusCode::UNKNOWN, "Write failed.");
            }
        }
        return grpc::Status::OK;
    }
    grpc::Status SimpleClientStream(grpc::ServerContext* context, grpc::ServerReader< simple::messages::SimpleRequest>* reader, simple::messages::SimpleReply* response) {
        simple::messages::SimpleRequest request;
        while(reader->Read(&request)) {
            std::cout << "\t---------- ClientStream: Received: " << request.message() << std::endl;
        }
        std::string message = "Response";
        std::cout << "\t---------- ClientStream: Sending: " << message << std::endl;
        response->set_message(message.c_str());
        return grpc::Status::OK;
    }
    grpc::Status SimpleBidirectionalStream(grpc::ServerContext* context, grpc::ServerReaderWriter<simple::messages::SimpleReply, simple::messages::SimpleRequest>* stream) {
        simple::messages::SimpleRequest request;
        simple::messages::SimpleReply reply;
        while(stream->Read(&request)) {
            std::cout << "\t---------- BidirectionalStream: Received: " << request.message() << std::endl;
            std::string prefix = request.message();
            const std::string messages[] = {prefix + " Response 1", prefix + " Response 2", prefix + " Response 3"};
            for(const std::string& message : messages) {
                std::cout << "\t---------- BidirectionalStream: Sending: " << message << std::endl;
                reply.set_message(message.c_str());
                if(!stream->Write(reply)) {
                    return grpc::Status(grpc::StatusCode::UNKNOWN, "Write failed.");
                }
            }
        }
        return grpc::Status::OK;
    }
};

void RunServer() {
    std::string server_address("0.0.0.0:50051");
    SimpleServiceImpl service;
    grpc::EnableDefaultHealthCheckService(true);
    grpc::reflection::InitProtoReflectionServerBuilderPlugin();
    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();
}

int main(int argc, char** argv) {
    RunServer();
    return 0;
}
