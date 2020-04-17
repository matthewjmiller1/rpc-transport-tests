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

## Running the Container for Testing
To run the container so you can test the executables, run a container and exec
to that container so you have two terminals: one for the server and one for the
client.
```
docker run --rm -it --name rpc_transports --net host $USER/rpc_transports
docker exec -it rpc_transports /bin/bash
```

### rsocket Hello World
In the server terminal:
```
rsocket_test_server
```

To verify the server is running as expected, on the host machine you should see
port 9898 listening:
```
$ sudo netstat -npatl | grep :9898
tcp6       0      0 :::9898                 :::*                    LISTEN      5345/rsocket_test_s 
$
```

In the client terminal
```
rsocket_test_client
```

On the client, you should see:
```
Received >> Hello Jane!
```

On the server, you should see:
```
HelloRequestResponseRequestResponder.handleRequestResponse Metadata(0): <null>, Data(4): 'Jane'
```

### grpc Hello World
In the server terminal:
```
greeter_server
```

To verify the server is running as expected, on the host machine you should see
port 50051 listening:
```
$ sudo netstat -npatl | grep :50051
tcp6       0      0 :::50051                :::*                    LISTEN      5363/greeter_server
$
```

In the client terminal
```
greeter_client
```

On the client, you should see:
```
Greeter received: Hello world
```

On the server, you should see:
```
Server listening on 0.0.0.0:50051
```

## Running the Container for Building
To run the container so you can build the code:
```
docker run --rm -it --name rpc_transports --net host -v "$(pwd)"/:/app --user $(id -u):$(id -g) $USER/rpc_transports
```

This mounts /app in the container to your current directory, which is the top
of the git repository and uses your user ID as the container's user to you can
work in the git repo in the container without permission issues due to running
at root in the container.

Note that while your user ID is valid in the container since you're using the
host file system for /app, the container doesn't know about your user name so
`whoami` won't work as expected and the prompt will show:
```
I have no name!@localhost:/$
```

Now, you can build in the container from your git repo. E.g.:
To build once in the container:
```
cd /app/src/rt_client_server
mkdir -p cmake/build
cmake ../..
make -j $(nproc)
```
