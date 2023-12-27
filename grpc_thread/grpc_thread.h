// grpc_thread.h
#ifndef GRPC_THREAD_H
#define GRPC_THREAD_H

#include <grpcpp/grpcpp.h>
#include "grpc/input_service.grpc.pb.h"
#include "../logger_thread/logger_thread.h"

class InputServiceImpl final : public Input::Service {

public:
    LoggerThread* loggerThread;

    grpc::Status StartRecording(grpc::ServerContext* context, const StartRequest* request,
                                StatusResponse* response) override;
    grpc::Status StopRecording(grpc::ServerContext* context, const StopRequest* request,
                               StatusResponse* response) override;
};

#endif // GRPC_THREAD_H
