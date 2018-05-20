# http-server

A very minimal multi-threaded HTTP server using the thread pool pattern.

Did this project for learning purposes and out of curiosity.

## Notes

This HTTP server can only handle GET requests...

There are many features that can be added and changes that can be made to this HTTP server. Details can be found in [TODO](TODO).

This project was built and tested on Ubuntu 16.04. Will most likely work in other Linux distros.


## Build

First clone repo:

```
$ git clone https://github.com/nujhedtozu/http-server.git
$ cd http-server
```

In the project root directory (`http-server/`):
```
$ make
```

To remove object files:

```
$ make clean
```

## Run

In the project root directory:
```
$ sudo ./http-server
```

To gracefully terminate server and free resources, send SIGINT by pressing Ctrl-C.
