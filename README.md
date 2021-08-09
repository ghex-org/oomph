# OOMPH

**Oomph** is a library for enabling high performance point-to-point, asynchronous communication over different fabrics.
It leverages
the ubiquitos MPI library as well as UCX (and will support Libfabric).
Both device and host memory are supported. Under the hood it uses hwmalloc for memory registration.

**selling points**
- lightweight, fast
- ABI stable
- variety of backends
- can be used in threaded applications

**current capabilites**
- tagged asynchronous send/recv
- callback based interface

**future plans**
- channels
- libfabric backend

# Overview

Below a few important features of the library are highlighted and illustrated with code snippets.
Note, that this library
expects that at least an MPI implementation is available.

## Context
In order to use **oomph** a context object needs to be created.  The context object manages the
lifetime of the transport layer specific infrastructure.

```cpp
oomph::context ctxt{MPI_WORLD};

```
The context must therefore be created in a serial part of the code. All of its member functions
are thread-safe.

## Message Buffer

Messages must sent through a message buffer which can be created from the context or the communicator (see also below).
```cpp
oomph::message_buffer<int> msg = ctxt.make_buffer<int>(100);
```
Device memory can be requested using
```cpp
oomph::message_buffer<int> msg = ctxt.make_device_buffer<int>(100);
```
Note, that the underlying memory manager (hwmalloc) will always allocate on the host, and will mirror
memory on the host. This does not imply that communications will always go through the host, however.
GPU aware transport layer functionality is fully supported.

## Communicator

The communicator allows to schedule send and receive operations.
A communicator object is thread compatible. Usually one communicator is created per thread, however, multiple instances
per thread are possible. One must not use another thread's communicator object.

```cpp
oomph::communicator comm = ctxt.get_communicator();
```

### Send/Recv

Send and receive operations return request objects which can be used to monitor completion of the scheduled operation.
```cpp
// send a message to rank 1 with tag 42
oomph::send_request req = comm.send(msg, 1, 42);
// check whether communication has finished
if (req.is_ready()) { /* do something */ }
// check whether communication has finished and progress transport layer
if (req.test()) { /* do something */ }
// wait until communication has finished
req.wait();
```
The receive requests additionally expose a member funcion to cancel the scheduled operation if possible.
```cpp
// receive a message from rank 1 with tag 42
oomph::recv_request req = comm.recv(msg, 1, 42);
// cancel receive request
req.cancel();
```

Every send and receive operation can be used in conjunction with a callback which is invoked once the operation completes.
```cpp
oomph::message_buffer<int> msg = ctxt.make_buffer<int>(100);
oomph::send_request req = comm.send(msg, 1, 42,
    [](message_buffer<int> & msg, int rank, int tag)
    {
        // do something with the message
    });
```
Note, that the signature of the callback depends on the function it is used with. For example, the communicator will
take over a message's lifetime management when the message is passed by r-value reference:
```cpp
oomph::message_buffer<int> msg = ctxt.make_buffer<int>(100);
oomph::send_request req = comm.send(std::move(msg), 1, 42,
    [](message_buffer<int> msg, int rank, int tag)
    {
        // do something with the message
    });
// At this point we can forget the message or assign some new message to it
msg = ctxt.make_buffer<int>(10);
```
Here, the callback will need to receive the message argument by value.

Note: recursive calls to the communicator from within a callback are explicitely allowed.

