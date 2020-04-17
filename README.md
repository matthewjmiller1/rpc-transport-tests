# rpc-transport-tests
Basic testing for RPC frameworks

# Dependencies
The Dockerfile shows all the dependencies that need installed.

# Running the Code
Download the code and go into its directory (following commands assume
you are in this directory).

```
git clone https://github.com/matthewjmiller1/rpc-transport-tests.git
cd rpc-transport-tests
```

## Building the Container
To build the container:
```
docker build -t $USER/rpc_transports .
```

## Working in the Container
### Running the Container
To run the container so you can build the code:
```
docker run --rm -it --name rpc_transports --net host -v "$(pwd)"/:/app --user $(id -u):$(id -g) $USER/rpc_transports
```

### Building in the Container
To build once in the container:
```
mkdir -p /app/build
cd /app/build
cmake .. && make
```

### Running in the Container
Each of these assumes you have two terminals to your container that was run
using the "docker run" command above.

For additional terminals to your contianer, use:
```
docker exec -it rpc_transports /bin/bash
```

### rsocket Hello World
Start the server in the one terminal:
```
cd /app/build
./rsocket_test_server
```

To verify the server is running as expected, on the host machine you should see
port 9898 listening:
```
$ sudo netstat -npatl | grep :9898
tcp6       0      0 :::9898                 :::*                    LISTEN      27569/./rsocket_tes
$
```

Run the client in another terminal:
```
cd /app/build
./rsocket_test_client
```

On the client, you should see:
```
Received >> Hello Jane!
```

On the server, you should see:
```
HelloRequestResponseRequestResponder.handleRequestResponse Metadata(0): <null>, Data(4): 'Jane'
```
