#include <grpc/grpc.h>

static void main2(void)
{
    grpc_init();

    const grpc_completion_queue_factory *factory;
    const grpc_completion_queue_attributes *attributes;
    grpc_completion_queue *cq = grpc_completion_queue_create_for_next(NULL);

    grpc_server *server = grpc_server_create(NULL, NULL);
    char const addr[] = "0.0.0.0:50051";
    grpc_server_register_completion_queue(server, cq, NULL);

    grpc_channel *ch = grpc_insecure_channel_create(addr, NULL, NULL);
    grpc_server_start(server);

    grpc_server_shutdown_and_notify(server, cq, NULL);
    grpc_server_destroy(server);
    grpc_shutdown();
}
