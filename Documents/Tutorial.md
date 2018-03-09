# Tutorial

The following instructions describe how to run the NMOS Registry and some example NMOS Nodes provided by nmos-cpp.

## Preparation

Follow the [Getting Started](Getting-Started.md) instructions before proceeding.

Notes:
- On Windows, the correct configuration of the C++ REST SDK library (e.g. cpprestsdk120_2_10.dll or cpprest120d_2_10.dll) needs to be on the ``PATH``.
- The Bonjour service (Windows) or ``mdnsd`` daemon (Linux) must be started to enable NMOS Nodes, and the Registry, to use DNS Service Discovery.
- When running more than one nmos-cpp application on the same host, configuration parameters **must** be used to select unique port numbers, as described below.

## Start an NMOS Registry

Run the **nmos-cpp-registry** application. Configuration parameters may be passed on the command-line as JSON, which is demonstrated by the example Windows batch script, [nmos-cpp-registry.bat](../Development/nmos-cpp-registry.bat), in the repository.

With no configuration parameters, the initial output should appear something like this:

```
2017-11-09 11:48:56.294: info: 67948: Starting nmos-cpp registry
2017-11-09 11:48:56.310: info: 67948: Configuring nmos-cpp registry with its Node API at: 10.0.0.1:3212
2017-11-09 11:48:56.310: info: 67948: Configuring nmos-cpp registry with its Registration API at: 10.0.0.1:3210
2017-11-09 11:48:56.310: info: 67948: Configuring nmos-cpp registry with its Query API at: 10.0.0.1:3211
Press return to quit.
2017-11-09 11:48:56.326: info: 67948: Registered advertisement for: nmos-cpp_query_10.0.0.1:3211._nmos-query._tcp
2017-11-09 11:48:56.326: info: 67948: Registered advertisement for: nmos-cpp_registration_10.0.0.1:3210._nmos-registration._tcp
2017-11-09 11:48:56.326: info: 67948: Registered advertisement for: nmos-cpp_node_10.0.0.1:3212._nmos-node._tcp
2017-11-09 11:48:56.326: info: 67948: Preparing for connections
2017-11-09 11:48:56.341: info: 67948: Ready for connections
```

This shows the nmos-cpp-registry starting up and advertising its APIs via DNS Service Discovery.

## Start several NMOS Nodes

Run the **nmos-cpp-node** application one or more times. Configuration parameters may be passed on the command-line as JSON, which is demonstrated by the example Windows batch script, [nmos-cpp-node.bat](../Development/nmos-cpp-node.bat), in the repository.

When running the nmos-cpp-registry and every nmos-cpp-node on different hosts, no configuration parameters are required.

In this case, each node's initial output should appear something like this:

```
2017-11-09 11:51:45.713: info: 30940: Starting nmos-cpp node
2017-11-09 11:51:49.756: info: 30940: Configuring nmos-cpp node with its Node API at: 10.0.0.2:3212
2017-11-09 11:51:49.756: info: 30940: Registering nmos-cpp node with the Registration API at: 10.0.0.1:3210
Press return to quit.
2017-11-09 11:51:49.772: info: 30940: Registered advertisement for: nmos-cpp_node_10.0.0.2:3212._nmos-node._tcp
2017-11-09 11:51:49.772: info: 30940: Preparing for connections
2017-11-09 11:51:49.787: info: 39480: Sending 6 changes to the Registration API
2017-11-09 11:51:49.787: info: 39480: Requesting registration creation for node: 21848058-2625-43d6-bb34-671e08ed4c84
2017-11-09 11:51:49.787: info: 30940: Ready for connections
2017-11-09 11:51:49.803: info: 39480: Requesting registration creation for device: dcfc538e-6af3-4e94-9bcf-9f6df89d0bc3
2017-11-09 11:51:49.803: info: 39480: Requesting registration creation for source: 555c161b-6e3b-4403-af22-3656d6397412
2017-11-09 11:51:49.818: info: 39480: Requesting registration creation for flow: 28ad8b02-44d3-4a8e-9c36-a14075d48d71
2017-11-09 11:51:49.818: info: 39480: Requesting registration creation for sender: a6dffe48-9def-4291-a182-f013c2b5d583
2017-11-09 11:51:49.818: info: 39480: Requesting registration creation for receiver: df85086a-b283-45e4-9ed6-c1583925a036
```

This shows the nmos-cpp-node starting up and advertising its Node API via DNS Service Discovery.

It selects an NMOS Registry to use, and registers itself, according to the NMOS [Discovery: Registered Operation](https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/docs/3.1.%20Discovery%20-%20Registered%20Operation.md) procedure.

On the other side, this operation should be reflected in the nmos-cpp-registry output something like this:

```
2017-11-09 11:51:49.787: info: 56916: Registration requested for node: 21848058-2625-43d6-bb34-671e08ed4c84
2017-11-09 11:51:49.803: more info: 56916: access: Sending response
2017-11-09 11:51:49.803: info: 66984: Registration requested for device: dcfc538e-6af3-4e94-9bcf-9f6df89d0bc3 on node: 21848058-2625-43d6-bb34-671e08ed4c84
2017-11-09 11:51:49.803: more info: 66984: access: Sending response
2017-11-09 11:51:49.818: info: 9632: Registration requested for source: 555c161b-6e3b-4403-af22-3656d6397412 on device: dcfc538e-6af3-4e94-9bcf-9f6df89d0bc3
2017-11-09 11:51:49.818: more info: 9632: access: Sending response
2017-11-09 11:51:49.818: info: 7220: Registration requested for flow: 28ad8b02-44d3-4a8e-9c36-a14075d48d71 on source: 555c161b-6e3b-4403-af22-3656d6397412
2017-11-09 11:51:49.818: more info: 7220: access: Sending response
2017-11-09 11:51:49.818: info: 9632: Registration requested for sender: a6dffe48-9def-4291-a182-f013c2b5d583 on device: dcfc538e-6af3-4e94-9bcf-9f6df89d0bc3 of flow: 28ad8b02-44d3-4a8e-9c36-a14075d48d71
2017-11-09 11:51:49.818: more info: 9632: access: Sending response
2017-11-09 11:51:49.834: info: 7220: Registration requested for receiver: df85086a-b283-45e4-9ed6-c1583925a036 on device: dcfc538e-6af3-4e94-9bcf-9f6df89d0bc3 subscribed to sender: null
```
