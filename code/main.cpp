#include "server/webserver.h"
#include "config/config.h"

int main(){

    string filename = "config.ini";

    Config config(filename);

    WebServer server(config);

    server.Start();
    
    return 0;
}