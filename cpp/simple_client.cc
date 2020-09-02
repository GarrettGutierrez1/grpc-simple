#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <grpcpp/grpcpp.h>

#include "simple/simple.grpc.pb.h"

class SimpleClient {
private:
    std::unique_ptr<simple::Simple::Stub> stub_;
public:
    SimpleClient(std::shared_ptr<grpc::Channel> channel) : stub_(simple::Simple::NewStub(channel)) {}
    void unary() {
        grpc::ClientContext context;
        simple::messages::SimpleRequest request;
        simple::messages::SimpleReply reply;
        std::string message = "Request";
        request.set_message(message.c_str());
        std::cout << "\t---------- Unary: Sending : " << message << std::endl;
        grpc::Status status = stub_->SimpleUnary(&context, request, &reply);
        if (!status.ok()) {
            std::cout << "\t---------- Unary: Failed: " << status.error_message() << std::endl;
            return;
        }
        std::cout << "\t---------- Unary: Received: " << reply.message() << std::endl;
    }
    void serverStream() {
        grpc::ClientContext context;
        simple::messages::SimpleRequest request;
        simple::messages::SimpleReply reply;
        std::string message = "Request";
        request.set_message(message.c_str());
        std::cout << "\t---------- ServerStream: Sending : " << message << std::endl;
        std::unique_ptr<grpc::ClientReader<simple::messages::SimpleReply>> reader(stub_->SimpleServerStream(&context, request));
        while(reader->Read(&reply)) {
            std::cout << "\t---------- ServerStream: Received: " << reply.message() << std::endl;
        }
        grpc::Status status = reader->Finish();
        if(!status.ok()) {
            std::cout << "\t---------- ServerStream: Failed: " << status.error_message() << std::endl;
        }
    }
    void clientStream() {
        grpc::ClientContext context;
        simple::messages::SimpleRequest request;
        simple::messages::SimpleReply reply;
        std::unique_ptr<grpc::ClientWriter<simple::messages::SimpleRequest> > writer(stub_->SimpleClientStream(&context, &reply));
        const std::string messages[] = {"Response 1", "Response 2", "Response 3"};
        for(const std::string& message : messages) {
            request.set_message(message.c_str());
            std::cout << "\t---------- ClientStream: Sending : " << message << std::endl;
            if (!writer->Write(request)) {
                break;
            }
        }
        grpc::Status status = writer->Finish();
        if(!status.ok()) {
            std::cout << "\t---------- ClientStream: Failed: " << status.error_message() << std::endl;
        } else {
            std::cout << "\t---------- ClientStream: Received: " << reply.message() << std::endl;
        }
    }
    void bidirectionalStream() {
        grpc::ClientContext context;
        std::shared_ptr<grpc::ClientReaderWriter<simple::messages::SimpleRequest, simple::messages::SimpleReply>> stream(stub_->SimpleBidirectionalStream(&context));
        std::thread writer([stream]() {
            simple::messages::SimpleRequest request;
            const std::string messages[] = {"Response 1", "Response 2", "Response 3"};
            for(const std::string& message : messages) {
                std::cout << "\t---------- BidirectionalStream: Sending : " << message << std::endl;
                request.set_message(message.c_str());
                stream->Write(request);
            }
            stream->WritesDone();
        });
        simple::messages::SimpleReply reply;
        while(stream->Read(&reply)) {
            std::cout << "\t---------- BidirectionalStream: Received: " << reply.message() << std::endl;
        }
        writer.join();
        grpc::Status status = stream->Finish();
        if(!status.ok()) {
            std::cout << "\t---------- BidirectionalStream: Failed: " << status.error_message() << std::endl;
        }
    }
};

int main(int argc, char** argv) {
    std::string target_str;
    std::string arg_str("--target");
    if (argc > 1) {
        std::string arg_val = argv[1];
        size_t start_pos = arg_val.find(arg_str);
        if (start_pos != std::string::npos) {
            start_pos += arg_str.size();
            if (arg_val[start_pos] == '=') {
                target_str = arg_val.substr(start_pos + 1);
            } else {
                std::cout << "The only correct argument syntax is --target=" << std::endl;
                return 0;
            }
        } else {
            std::cout << "The only acceptable argument is --target=" << std::endl;
            return 0;
        }
    } else {
        target_str = "localhost:50051";
    }
    SimpleClient simple(grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
    simple.unary();
    simple.serverStream();
    simple.clientStream();
    simple.bidirectionalStream();
    return 0;
}
