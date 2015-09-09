Wangle
======

Coral-Wangle is a fork of facebook wangle, a C++ networking library.

Wangle is a library that makes it easy to build protocols, application
clients, and application servers.

It's like Netty + Finagle smooshed together, but in C++

Building and Installing
-----------------------

First, download and install the coral-folly library from
<https://github.com/yeolar/coral-folly>. You'll also need CMake.
Once coral-folly is installed, run the following to build,
test, and install wangle:

```
$ mkdir build && cd build
$ cmake ..
$ make
$ make test     # optional
# make install
```

Examples
--------

See the examples/ directory for some example Wangle servers and clients.

Documentation
-------------

Wangle interfaces are asynchronous. Interfaces are currently based on
[Futures](https://github.com/yeolar/coral-folly/tree/master/folly/futures),
but we're also exploring how to support fibers.

Client / Server abstraction
---------------------------

You're probably familiar with Java's Netty, or Python's twisted, or
similar libraries.

It is built on top of folly/io/async, so it's one level up the stack
from that (or similar abstractions like boost::asio).

ServerBootstrap - easily manage creation of threadpools and pipelines

ClientBootstrap - the same for clients

Pipeline - set up a series of handlers that modify your socket data

Request / Response abstraction
------------------------------

This is roughly equivalent to the
[Finagle](https://twitter.github.io/finagle/) library.

Aims to provide easy testing, load balancing, client pooling, retry
logic, etc. for any request/response type service - i.e. thrift, http,
etc.

Service - a matched interface between client/server. A server will
implement this interface, and a client will call in to it. These are
protocol-specific

ServiceFilter - a generic filter on a service. Examples: stats, request
timeouts, rate limiting

ServiceFactory - A factory that creates client connections. Any protocol
specific setup code goes here

ServiceFactoryFilter - Generic filters that control how connections are
created. Client examples: load balancing, pooling, idle timeouts,
markdowns, etc.

ServerBootstrap
===============

Easily create a new server

ServerBootstrap does the work to set up one or multiple acceptor
threads, and one or multiple sets of IO threads. The thread pools can be
the same. SO_REUSEPORT is automatically supported for multiple accept
threads. tcp is most common, although udp is also supported.

Methods
-------

**childPipeline(PipelineFactory\<Pipeline\>)**

Sets the pipeline factory for each new connection. One pipeline per
connection will be created.

**group(IOThreadPoolExecutor accept, IOThreadPoolExecutor io)**

Sets the thread pools for accept and io thread pools. If more than one
thread is in the accept group, SO_REUSEPORT is used. Defaults to a
single accept thread, and one io thread per core.

**bind(SocketAddress), bind(port)**

Binds to a port. Automatically starts to accept after bind.

**stop()**

Stops listening on all sockets.

**join()**

Joins all threadpools - all current reads and writes will be completed
before this method returns.

NOTE: however that both accept and io thread pools will be stopped using
this method, so the thread pools can't be shared, or care must be taken
using shared pools during shutdown.

**waitForStop()**

Waits for stop() to be called from another thread.

Other methods
-------------

**channelFactory(ServerSocketFactory)**

Sets up the type of server. Defaults to TCP AsyncServerSocket, but
AsyncUDPServerSocket is also supported to receive udp messages. In
practice, ServerBootstrap is only useful for udp if you need to
multiplex the messages across many threads, or have TCP connections
going on at the same time, etc. Simple usages of AsyncUDPSocket probably
don't need the complexity of ServerBootstrap.

**pipeline(PipelineFactory\<AcceptPipeline\>)**

This pipeline method is used to get the accepted socket (or udp message)
\*before\* it has been handed off to an IO thread. This can be used to
steer the accept thread to a particular thread, or for logging.

See also AcceptRoutingHandler and RoutingDataHandler for additional help
in reading data off of the accepted socket **before** it gets attached
to an IO thread. These can be used to hash incoming sockets to specific
threads.

**childHandler(AcceptorFactory)**

Previously facebook had lots of code that used AcceptorFactories instead
of Pipelines, this is a method to support this code and be backwards
compatible. The AcceptorFactory is responsible for creating acceptors,
setting up pipelines, setting up AsyncSocket read callbacks, etc.

Examples
--------

A simple example:

```
ServerBootstrap<TelnetPipeline> server;
server.childPipeline(std::make_shared<TelnetPipelineFactory>());
server.bind(FLAGS_port);
server.waitForStop();
```

ClientBootstrap
===============

Create clients easily

ClientBootstrap is a thin wrapper around AsyncSocket that provides a
future interface to the connect callback, and a Pipeline interface to
the read callback.

Methods
-------

**group(IOThreadPoolExecutor)**

Sets the thread or group of threads where the IO will take place.
Callbacks are also made on this thread.

**bind(port)**

Optionally bind to a specific port

**Future\<Pipeline\*\> connect(SocketAddress)**

Connect to the selected address. When the future is complete, the
initialized pipeline will be returned.

NOTE: future.cancel() can be called to cancel an outstanding connection
attempt.

**pipelineFactory(PipelineFactory\<Pipeline\>)**

Set the pipeline factory to use after a connection is successful.

Example
-------

```
ClientBootstrap<TelnetPipeline> client;
client.group(std::make_shared<folly::wangle::IOThreadPoolExecutor>(1));
client.pipelineFactory(std::make_shared<TelnetPipelineFactory>());
// synchronously wait for the connect to finish
auto pipeline = client.connect(SocketAddress(FLAGS_host,FLAGS_port)).get();

// close the pipeline when finished
pipeline->close();
```

Pipeline
========

Send your socket data through a series of tubes

A Pipeline is a series of Handlers that intercept inbound or outbound
events, giving full control over how events are handled. Handlers can be
added dynamically to the pipeline.

When events are called, a Context\* object is passed to the Handler -
this means state can be stored in the context object, and a single
instantiation of any individual Handler can be used for the entire
program.

Netty's documentation:
[ChannelHandler](http://netty.io/4.0/api/io/netty/channel/ChannelPipeline.html)

Usually, the bottom of the Pipeline is a wangle::AsyncSocketHandler to
read/write to a socket, but this isn't a requirement.

A pipeline is templated on the input and output types:

```
EventBase base_;
Pipeline<IOBufQueue&, std::unique_ptr<IOBuf>> pipeline;
pipeline.addBack(AsyncSocketHandler(AsyncSocket::newSocket(eventBase)));
```

The above creates a pipeline and adds a single AsyncSocket handler, that
will push read events through the pipeline when the socket gets bytes.
Let's try handling some socket events:

```
class MyHandler : public InboundHandler<folly::IOBufQueue&> {
 public:

  void read(Context* ctx, folly::IOBufQueue& q) override {
    IOBufQueue data;
    if (q.chainLength() >= 4) {
       data.append(q.split(4));
       ctx->fireRead(data);
    }
  }
};
```

This handler only handles read (inbound) data, so we can inherit from
InboundHandler, and ignore the outbound type (so the ordering of
inbound/outbound handlers in your pipeline doesn't matter). It checks if
there are at least 4 bytes of data available, and if so, passes them on
to the next handler. If there aren't yet four bytes of data available,
it does nothing, and waits for more data.

We can add this handler to our pipeline like so:

```
pipeline.addBack(MyHandler());
```

and remove it just as easily:

```
pipeline.remove<MyHandler>();
```

StaticPipeline
--------------

Instantiating all these handlers and pipelines can hit the allocator
pretty hard. There are two ways to try to do fewer allocations.
StaticPipeline allows *all* the handlers, and the pipeline, to be
instantiated all in the same memory block, so we only hit the allocator
once.

The other option is to allocate the handlers once at startup, and reuse
them in many pipelines. This means all state has to be saved in the
HandlerContext object instead of the Handler itself, since each handler
can be in multiple pipelines. There is one context per pipeline to get
around this limitation.

Built-in handlers
=================

The stuff that comes with the box

Byte to byte handlers
---------------------

**AsyncSocketHandler**

This is almost always the first handler in the pipeline for clients and
servers - it connects an AsyncSocket to the pipeline. Having it as a
handler is nice, because mocking it out for tests becomes trivial.

**OutputBufferingHandler**

Output is buffered and only sent once per event loop. This logic is
exactly what is in ThriftServer, and very similar to what exists in
proxygen - it can improve throughput for small writes by up to 300%.

**EventBaseHandler**

Putting this right after an AsyncSocketHandler means that writes can
happen from any thread, and eventBase-\>runInEventBaseThread() will
automatically be called to put them in the correct thread. It doesn't
intrinsically make the pipeline thread-safe though, writes from
different threads may be interleaved, other handler stages must be only
used from one thread or be thread safe, etc.

In addition, reads are still always called on the eventBase thread.

Codecs
------

**FixedLengthFrameDecoder**

A decoder that splits received IOBufs by a fixed number of bytes. Used
for fixed-length protocols

**LengthFieldPrepender**

Prepends a fixed-length field length. Field length is configurable.

**LengthFieldBasedFrameDecoder**

The receiving portion of LengthFieldPrepender - decodes based on a fixed
frame length, with optional header/tailer data sections.

**LineBasedFrameDecoder**

Decodes by line (with optional ending detection types), to be used for
text-based protocols

**StringCodec**

Converts from IOBufs to std::strings and back for text-based protocols.
Must be used after one of the above frame decoders

Services
========

How to add a new protocol

[Finagle's
documentation](https://twitter.github.io/finagle/guide/ServicesAndFilters.html)
on Services is highly recommended

Services
--------

A Pipeline was read() and write() methods - it streams bytes in one or
both directions. write() returns a future, but the future is set when
the bytes are successfully written. Using pipeline there is no easy way
to match up requests and responses for RPC.

A Service is an RPC abstraction - Both clients and servers implement the
interface. Servers implement it by handling the request. Clients
implement it by sending the request to the server to complete.

A Dispatcher is the adapter between the Pipeline and Service that
matches up the requests and responses. There are several built in
Dispatchers, however if you are doing anything advanced, you may need to
write your own.

Because both clients and servers implement the same interface, mocking
either clients or servers is trivially easy.

ServiceFilters
--------------

ServiceFilters provide a way to wrap filters around every request and
response. Things like logging, timeouts, retrying requests, etc. can be
implemented as ServiceFilters.

Existing ServiceFilters include:

-   CloseOnReleaseFilter - rejects requests after connection is closed.
    Often used in conjunction with
-   ExpiringFilter - idle timeout and max connection time (usually used
    for clients)
-   TimeoutFilter - request timeout time. Usually used on servers.
    Clients can use future.within to specify timeouts individually.
-   ExecutorFilter - move requests to a different executor.

ServiceFactories
----------------

For some services, a Factory can help instantiate clients. In Finagle,
these are frequently provided for easy use with specific protocols, i.e.
http, memcache, etc.

ServiceFactoryFilters
---------------------

ServiceFactoryFilters provide filters for getting clients. These include
most connection-oriented things, like connection pooling, selection,
dispatch, load balancing, etc.

Thread pools & Executors
========================

Run your concurrent code in a performant way

All about thread pools
----------------------

**How do I use the thread pools?**

Wangle provides two concrete thread pools (IOThreadPoolExecutor,
CPUThreadPoolExecutor) as well as building them in as part of a complete
async framework. Generally you might want to grab the global executor,
and use it with a future, like this:

```
auto f = someFutureFunction().via(getCPUExecutor()).then(...)
```

Or maybe you need to construct a thrift/memcache client, and need an
event base:

```
auto f = getClient(getIOExecutor()->getEventBase())->callSomeFunction(args...)
         .via(getCPUExecutor())
         .then([](Result r){ .... do something with result});
```

**vs. C++11's std::launch**

The current C++11 std::launch only has two modes: async or deferred. In
a production system, neither is what you want: async will launch a new
thread for every launch without limit, while deferred will defer the
work until it is needed lazily, but then do the work **in the current
thread synchronously** when it is needed.

Wangle's thread pools always launch work as soon as possible, have
limits to the maximum number of tasks / threads allowed, so we will
never use more threads than absolutely needed. See implementation
details below about each type of executor.

**Why do we need yet another set of thread pools?**

Unfortunately none of the existing thread pools had every feature needed
- things based on pipes are too slow. Several older ones didn't support
std::function.

**Why do we need several different types of thread pools?**

If you want epoll support, you need an fd - event_fd is the latest
notification hotness. Unfortunately, an active fd triggers all the epoll
loops it is in, leading to thundering herd - so if you want a fair queue
(one queue total vs. one queue per worker thread), you need to use some
kind of semaphore. Unfortunately semaphores can't be put in epoll loops,
so they are incompatible with IO. Fortunately, you usually want to
separate the IO and CPU bound work anyway to give stronger tail latency
guarantees on IO.

**IOThreadPoolExecutor**

-   Uses event_fd for notification, and waking an epoll loop.
-   There is one queue (NotificationQueue specifically) per
    thread/epoll.
-   If the thread is already running and not waiting on epoll, we don't
    make any additional syscalls to wake up the loop, just put the new
    task in the queue.
-   If any thread has been waiting for more than a few seconds, its
    stack is madvised away. Currently however tasks are scheduled round
    robin on the queues, so unless there is **no** work going on, this
    isn't very effective.
-   ::getEventBase() will return an EventBase you can schedule IO work
    on directly, chosen round-robin.
-   Since there is one queue per thread, there is hardly any contention
    on the queues - so a simple spinlock around an std::deque is used
    for the tasks. There is no max queue size.
-   By default, there is one thread per core - it usually doesn't make
    sense to have more IO threads than this, assuming they don't block.

**CPUThreadPoolExecutor**

-   A single queue backed by folly/LifoSem and folly/MPMC queue. Since
    there is only a single queue, contention can be quite high, since
    all the worker threads and all the producer threads hit the same
    queue. MPMC queue excels in this situation. MPMC queue dictates a
    max queue size.
-   LifoSem wakes up threads in Lifo order - i.e. there are only few
    threads as necessary running, and we always try to reuse the same
    few threads for better cache locality.
-   Inactive threads have their stack madvised away. This works quite
    well in combination with Lifosem - it almost doesn't matter if more
    threads than are necessary are specified at startup.
-   stop() will finish all outstanding tasks at exit
-   Supports priorities - priorities are implemented as multiple queues
    - each worker thread checks the highest priority queue first.
    Threads themselves don't have priorities set, so a series of long
    running low priority tasks could still hog all the threads. (at last
    check pthreads thread priorities didn't work very well)

**ThreadPoolExecutor**

Base class that contains the thread startup/shutdown/stats logic, since
this is pretty disjoint from how tasks are actually run

**Observers**

An observer interface is provided to listen for thread start/stop
events. This is useful to create objects that should be one-per-thread,
but also have them work correctly if threads are added/removed from the
thread pool.

**Stats**

PoolStats are provided to get task count, running time, waiting time,
etc.

