# Architecture of nmos-cpp

The [nmos](../Development/nmos/) module fundamentally provides three things. 

1. A C++ data model for the AMWA IS-04 and IS-05 NMOS resources which represent the logical functionality of a Node, or equally, for the resources of many Nodes held by a Registry.
2. An implementation of each of the REST APIs defined by the AMWA IS-04 and IS-05 NMOS specifications, in terms of the data model.
3. An implementation of the Node and Registry "active behaviours" defined by the specifications.  

The module also provides the concept of a server which combines the REST APIs and behaviours into a single object for simplicity.

## Data Model

> [nmos/model.h](../Development/nmos/model.h)

The top-level data structures for an NMOS Node and Registry are ``nmos::node_model`` and ``nmos::registry_model`` respectively.

A ``node_model`` has three member variables which are containers, of IS-04 resources, IS-05 resources and IS-07 resources, respectively:

```C++
nmos::resources node_resources;
nmos::resources connection_resources;
nmos::resources events_resources;
```

A ``registry_model`` has two containers, this time for the Registry's own Node API "self" resource, and for the resources that have been registered with the Registration API:

```C++
nmos::resources node_resources;
nmos::resources registry_resources;
```

It also has a single resource for the IS-09 System API global configuration resource for when the registry is also serving this purpose:

```C++
nmos::resource system_global_resource;
```

In addition, both models, by inheritance from ``nmos::base_model``, include member variables to hold application-wide settings and shutdown state, and to perform synchronisation between multiple threads that need shared access to the data model.

### Resources Container

>  [nmos/resources.h](../Development/nmos/resources.h)

The ``nmos::resources`` container can be viewed similarly to a C++ Standard Library container class template like a ``std::map``, but it provides multiple indices required for the IS-04 APIs, namely:

* by id
* by type (node, device, source, flow, sender, receiver)
* by creation timestamp
* by update timestamp

Each index has the necessary constraints to maintain the invariants required by the NMOS specifications, while providing the required ordering and look-up operations.

The [Boost Multi-index Containers Library](https://www.boost.org/doc/libs/1_68_0/libs/multi_index/doc/index.html) is used to implement the ``resources`` container, but a higher-level API is provided by free functions including:

* ``nmos::insert_resource``
* ``nmos::modify_resource``
* ``nmos::erase_resource``
* ``nmos::find_resource``

Unlike a ``std::map``, each of the indices of the ``resources`` container are based on the relevant member variables of the contained elements, that is the resources themselves, as represented by the ``nmos::resource`` data structure.

### Resource

> [nmos/resource.h](../Development/nmos/resource.h)

An ``nmos::resource`` is primarily designed to represent the resources defined by the AMWA IS-04 specification.
A ``resource`` is constructed from a triplet of its API specification version, its resource type and the JSON representation of the resource data (which includes its unique id).

In addition to these data, a ``resource`` maintains several items of metadata, including the unique id of each of its sub-resources (for performance), the created and updated timestamps (for Query API ordering) and the resource health (for Registration API expiry).

### Synchronisation

> [nmos/model.h](../Development/nmos/model.h),
> [nmos/mutex.h](../Development/nmos/mutex.h)

The implementation of the REST APIs utilises the C++ REST SDK, which provides task-based concurrency for HTTP request handling via a thread pool.
In addition, the "active behaviours" of a Node or Registry are implemented by long-running threads.

Protecting the shared data model from being simultaneously accessed by multiple threads is thus critical. The ``base_model`` contains a mutex, which offers two levels of access:

* shared - several threads can share ownership of the same mutex
* exclusive - only one thread can own the mutex

Shared access is used to allow multiple threads to simultaneously _read_ the data model, provided no thread currently has exclusive access.
Exclusive access is used to allow a single thread to _write_ to the data model, preventing any threads from reading while the write operation is being performed.
Convenience member functions provide simple scoped locking of the mutex as follows:

```C++
auto lock = model.read_lock(); // takes a shared lock, blocks until the mutex is available
auto lock = model.write_lock(); // takes an exclusive lock, blocks until the mutex is available
```

In order to provide simple synchronisation between multiple threads, the ``base_model`` also contains a condition variable that any thread may use to block until another thread makes a change to the data model.

> **NOTE:** It is of course critical that any thread that modifies the data model does so only after gaining exclusive access via a write lock _and_ that it then uses the condition variable to notify waiting threads once its write operation is complete.

A convenience member function ``notify`` is provided to notify all threads waiting on the model's condition variable.
Convenience ``wait`` member function templates provide a simple means to block the current thread until the condition variable is notified or a spurious wakeup occurs, optionally looping until some predicate is satisfied.
For example:

```C++
auto lock = model.read_lock();
auto most_recent_update = nmos::tai_min();
for (;;)
{
    // wait for the thread to be interrupted because there have been updates to the model
    // or because the server is being shut down
    model.wait(lock, [&] { return model.shutdown || most_recent_update < nmos::most_recent_update(model.connection_resources); });
    if (model.shutdown) break;
    ...
}
```

## REST APIs

> [nmos/node_api.cpp](../Development/nmos/node_api.cpp),
> [nmos/connection_api.cpp](../Development/nmos/connection_api.cpp),
> [nmos/events_api.cpp](../Development/nmos/events_api.cpp),
> [nmos/registration_api.cpp](../Development/nmos/registration_api.cpp),
> [nmos/query_api.cpp](../Development/nmos/query_api.cpp),
> [nmos/system_api.cpp](../Development/nmos/system_api.cpp)

The ``nmos`` module also provides the implementation of each of the REST APIs defined by AMWA IS-04, IS-05, IS-07 and IS-09.

The C++ REST SDK provides a general purpose HTTP listener, that accepts requests at a particular base URL and passes them to a user-specified request handler for processing.
Therefore the ``nmos`` module implements each API as a request handler which reads and/or writes the relevant parts of the NMOS data model, and provides a convenience function, ``nmos::support_api``, for associating the API request handler with the HTTP listener.

For example, a simple configuration of the Connection API would look like:

```C++
auto connection_api = nmos::make_connection_api(node_model, gate);
web::http::experimental::listener::http_listener connection_listener(connection_uri);
nmos::support_api(connection_listener, connection_api);
```

As mentioned previously, the C++ REST SDK ``http_listener`` handles requests using task-based concurrency via a thread pool. The listener must be opened before any request will be accepted; the RAII idiom may be used to open and close the listener:

```C++
web::http::experimental::listener::http_listener_guard connection_guard(connection_listener);
```

Each specific endpoint, or route, defined by the RAML definition of the API in the NMOS specification, is implemented as a function object within the relevant C++ factory function, such as ``nmos::make_connection_api`` in the example above.
Each endpoint function takes the request and response objects, and returns a ``task`` object that becomes ready when the handler is finished, and the response is ready to send, or the next handler should be called.
This allows requests to be processed in parallel and without blocking the thread pool.

(The ``gate`` seen in the above example is the Logging Gateway through which log messages are sent for processing. Logging is discussed below.)

## Active Behaviour

The REST APIs of the ``nmos`` module can be viewed as "passive" or "reactive" in the sense that they are driven only by HTTP requests.
The AMWA IS-04 and IS-05 specifications also define the "active" or "proactive" behaviours that are required of Node and the Registry implementations.

The ``nmos`` module implements these behaviours as long-running threads that interact with the NMOS data model.

(Both the **nmos-cpp-node** simulated Node, and the **nmos-cpp-registry** Registry application, of course also have a ``main`` thread. After set-up this simply blocks waiting for a shutdown signal from the user.)

### Node Behaviour

> [nmos/node_behaviour.cpp](../Development/nmos/node_behaviour.cpp),
> [nmos/connection_activation.cpp](../Development/nmos/connection_activation.cpp)

The required Node behaviour includes:

- discovery and registration of the Node's resources with a Registry
- persistence of its resources in the registry by issuing heartbeats
- updating of registrations for existing resources in response to changes in the data model

The state machine implemented by the ``nmos::node_behaviour_thread`` is shown below:

![NMOS Node Behaviour](images/node-behaviour.png)  

<details>
<summary>More details...</summary>

Notes:

- in **Registered Operation**, "POST sub-resources" is short-hand for queuing up resource-added events for all the sub-resources on the transition from the **Initial Registration** state
- the exponential backoff is not described
- "(5xx) Error" also covers connectivity issues, timeouts
- HTTP response status codes that are not explicitly mentioned do not cause a state transition

</details>

In addition, the ``nmos::connection_activation_thread`` manages scheduling of Connection API activation requests. It supports callbacks that allow vendor-specific customization of the behaviour.

#### Vendor-specific Node Behaviour

> [nmos-cpp-node/node_implementation.cpp](../Development/nmos-cpp-node/node_implementation.cpp)

It is a principle of both AMWA IS-04 and IS-05 that the resources exposed by the APIs should reflect the current state of the underlying implementation.

It is therefore expected that a Node implementation using **nmos-cpp** will contain one or more additional threads that synchronise the NMOS data model with the underlying state.
For example, such a thread may wait to be notified of changes in the data model as a result of Connection API activation requests.
At any time, it may make changes to the model to reflect other events in the underlying implementation, provided it correctly locks the model mutex and notifies the condition variable.
The example ``node_implementation_thread`` demonstrates how such a thread might behave.

### Registry Behaviour

> [nmos/query_ws_api.cpp](../Development/nmos/query_ws_api.cpp),
> [nmos/registration_api.cpp](../Development/nmos/registration_api.cpp)

The required Registry behaviour includes:

- push notification of changes in the Registry via Query API WebSockets
- expiry of a Node's resources after the specified garbage collection interval if no heartbeats are received from that Node

These functions are implemented by ``nmos::send_query_ws_events_thread`` and ``nmos::erase_expired_resources_thread`` respectively.

The diagram below shows a sequence of events within and between an **nmos-cpp** Node, the **nmos-cpp-registry** Registry and a Client.
Resource events initiated in a resource-scheduling thread in the Node are propagated via the Registration API to the Registry model.
Events in the Registry model are sent in WebSocket messages to each Client with a matching Query API subscription.

![Sequence Diagram](images/node-registry-sequence.png)  

## Servers

> [nmos/node_server.cpp](../Development/nmos/node_server.cpp),
> [nmos/registry_server.cpp](../Development/nmos/registry_server.cpp)

The ``nmos`` module also provides factory functions to create NMOS Node servers and NMOS Registry servers, which combine the REST APIs and behaviours into a single object for simplicity.

For example, a simple configuration of a Node supporting the Node API, Connection API, Events API, etc. would look like:

```C++
auto node_server = nmos::experimental::make_node_server(node_model, node_implementation_resolve_auto, node_implementation_set_transportfile, log_model, gate);
nmos::server_guard node_server_guard(node_server);
```

## Logging

> [slog/all_in_one.h](../Development/slog/all_in_one.h)

Logging is a mundane but important feature of production software. Slog is a library for logging or tracing in C++. Most logging statements in **nmos-cpp** look a bit like this:

```C++
slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Beware the Jabberwock, my son!";
slog::log<slog::severities::info>(gate, SLOG_FLF) << "A picture is worth " << 1000 << " words.";
```

These logging statements can be easily disabled at compile-time (via the ``SLOG_LOGGING_SEVERITY`` preprocessor symbol) or at run-time, controlled by the specified logging gateway.

(The compile-time level used by the nmos-cpp apps can be configured via CMake.
Set the value of the CMake variable ``SLOG_LOGGING_SEVERITY`` to ``slog::never_log_severity`` to strip all logging at compile-time, or ``slog::max_verbosity`` for full control at run-time.)

Most top-level functions in the ``nmos`` module have a Logging Gateway as a final parameter. All log messages will be sent through the specified gateway.

(Both the **nmos-cpp-node** simulated Node, and the **nmos-cpp-registry** Registry application, have a ``main_gate`` logging gateway.
This can be configured to write error messages in a plain text file format, to also write an access log in the [Common Log Format](https://httpd.apache.org/docs/2.4/logs.html#accesslog)
also used by the Apache HTTP Server and others, and to expose recent log messages via a JSON-based REST API in the same style as the NMOS Query API.
Both applications also allow the logging verbosity to be changed at run-time - up to the compile-time level.
See the [Tutorial](Tutorial.md) and the provided [nmos-cpp-node/config.json](../Development/nmos-cpp-node/config.json) and [nmos-cpp-registry/config.json](../Development/nmos-cpp-registry/config.json) files for more information.)

<details>
<summary>More details...</summary>

In full, a ``slog`` logging statement may specify:

- the logging gateway that it uses
- its severity/verbosity level
  - this enables the logging statement to be almost entirely disabled at compile-time ("compiled out") if necessary
- any conditions that would make the log message irrelevant, such as intermittent ("1-in-N") logging statements
- a minimal set of logging attributes whose values can always be captured cheaply and only be captured right there (source filename, line number, function name/signature)
- the message itself, using stream-based or ``printf``-style message preparation

A logging statement at run-time:

- checks conditions that would make the log message irrelevant, and if not:
  - uses the logging gateway to decide if the log message will be pertinent, and if so:
    - constructs the log message with the logging attributes
    - prepares the message itself, including any additional statement-specific attributes (e.g. event id/error code, category)
    - sends the prepared message through the logging gateway
  - terminates the application if its severity level is fatal

</details>
