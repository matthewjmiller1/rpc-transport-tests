# rpc-transport-tests
Basic testing for RPC frameworks

# Dependencies
The Dockerfile shows all the dependencies that need installed.

E.g., to build the container:
```
docker build -t $USER/rpc_transports .
```

To run the container such that you can build/edit the source code:

```
git clone https://github.com/matthewjmiller1/rpc-transport-tests.git
cd rpc-transport-tests
docker run --rm -it --net host -v "$(pwd)"/:/app --user $(id -u):$(id -g) $USER/rpc_transports
```
