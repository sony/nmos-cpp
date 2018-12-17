# Tutorial

The following instructions describe how to run the NMOS Registry and some example NMOS Nodes provided by nmos-cpp.

## Preparation

Follow the [Getting Started](Getting-Started.md) instructions before proceeding.

Notes:
- On Windows, the correct configuration of the C++ REST SDK library (e.g. cpprestsdk140_2_10.dll or cpprest140d_2_10.dll) needs to be on the ``PATH``.
- The Bonjour service (Windows) or ``mdnsd`` daemon (Linux) must be started to enable NMOS Nodes, and the Registry, to use DNS Service Discovery.
- When running more than one nmos-cpp application on the same host, configuration parameters **must** be used to select unique port numbers, as described below.

## Start an NMOS Registry

Run the **nmos-cpp-registry** application. Configuration parameters may be passed on the command-line, or in a configuration file, as JSON. Each named parameter is specified as a field in a JSON object.
The provided [nmos-cpp-registry/config.json](../Development/nmos-cpp-registry/config.json) may be used as a starting point, or refer directly to the source code in [nmos/settings.h](../Development/nmos/settings.h).
Note: C++/JavaScript-style single and multi-line comments are permitted and ignored in nmos-cpp config files.

For example, to launch the application without verbose logging, try ``./nmos-cpp-registry "{\"logging_level\":0}"``, or ``./nmos-cpp-registry config.json`` with a file config.json:

```json
{
    "logging_level": 0
}
```

The initial output should appear something like this:

```
2018-06-25 16:24:06.536: info: 10460: Starting nmos-cpp registry
2018-06-25 16:24:06.571: info: 10460: Process ID: 11776
2018-06-25 16:24:06.571: info: 10460: Initial settings: {"host_address":"10.0.0.1","host_addresses":["10.0.0.1"],"host_name":"h1","logging_level":0,"seed_id":"91688498-560c-4f3c-b4d4-2718f83b227f"}
2018-06-25 16:24:06.571: info: 10460: Configuring nmos-cpp registry with its primary Node API at: 10.0.0.1:3212
2018-06-25 16:24:06.571: info: 10460: Configuring nmos-cpp registry with its primary Registration API at: 10.0.0.1:3210
2018-06-25 16:24:06.571: info: 10460: Configuring nmos-cpp registry with its primary Query API at: 10.0.0.1:3211
2018-06-25 16:24:07.484: info: 10460: Registered advertisement for: nmos-cpp_query_10.0.0.1:3211._nmos-query._tcp
2018-06-25 16:24:07.486: info: 10460: Registered advertisement for: nmos-cpp_registration_10.0.0.1:3210._nmos-registration._tcp
2018-06-25 16:24:07.487: info: 10460: Registered advertisement for: nmos-cpp_node_10.0.0.1:3212._nmos-node._tcp
2018-06-25 16:24:07.487: info: 10460: Preparing for connections
2018-06-25 16:24:07.496: info: 10460: Ready for connections
```

This shows the nmos-cpp-registry starting up and advertising its APIs via DNS Service Discovery.

## Start several NMOS Nodes

Run the **nmos-cpp-node** application one or more times. Like the registry application, configuration parameters may be passed on the command-line as JSON.
The provided [nmos-cpp-node/config.json](../Development/nmos-cpp-node/config.json) may be used as a starting point, or refer directly to the source code in [nmos/settings.h](../Development/nmos/settings.h).
Note: C++/JavaScript-style single and multi-line comments are permitted and ignored in nmos-cpp config files.

When running more than one nmos-cpp application on the same host, configuration parameters **must** be used to make the port numbers of each instance unique.
In the case of the node application, there are four APIs to consider;
``"node_port"``, ``"connection_port"``, ``"settings_port"`` and ``"logging_port"`` must be configured for each instance.
For brevity, the default port for _all_ the APIs can be overridden by using the ``"http_port"`` parameter.

Otherwise, the command may be as simple as ``./nmos-cpp-node "{\"logging_level\":0}"``, or ``./nmos-cpp-node config.json`` with a file config.json:

```json
{
    "logging_level": 0
}
```

Each node's initial output should appear something like this:

```
2018-06-25 16:25:23.387: info: 6096: Starting nmos-cpp node
2018-06-25 16:25:23.415: info: 6096: Process ID: 8340
2018-06-25 16:25:23.415: info: 6096: Initial settings: {"host_address":"10.0.0.2","host_addresses":["10.0.0.2"],"host_name":"h2","logging_level":0,"seed_id":"1befd731-e454-41d7-b198-9c736b2aaa6d"}
2018-06-25 16:25:23.415: info: 6096: Configuring nmos-cpp node with its primary Node API at: 10.0.0.2:3212
2018-06-25 16:25:23.428: info: 6096: Preparing for connections
2018-06-25 16:25:23.435: info: 6096: Ready for connections
2018-06-25 16:25:23.437: info: 4656: Registered advertisement for: nmos-cpp_node_10.0.0.2:3212._nmos-node._tcp
2018-06-25 16:25:23.439: info: 4656: Attempting discovery of a Registration API
2018-06-25 16:25:23.455: info: 4656: Discovered 2 Registration API(s)
2018-06-25 16:25:23.455: info: 4656: Attempting initial registration
2018-06-25 16:25:23.460: info: 4656: Registering nmos-cpp node with the Registration API at: 10.0.0.1:3210
2018-06-25 16:25:23.461: info: 4656: Requesting registration creation for node: 172bbbe0-a2da-5179-8a24-f2f288eeff75
2018-06-25 16:25:23.585: info: 4656: Adopting registered operation
2018-06-25 16:25:23.593: info: 4656: Requesting registration creation for device: b89caa85-556f-52e4-aec3-2c625a314bb5
2018-06-25 16:25:23.682: info: 4656: Requesting registration creation for source: baffbc06-cf83-59a5-9969-6494c6bc1b2e
2018-06-25 16:25:23.780: info: 4656: Requesting registration creation for flow: b8b8ab2b-aab8-50a3-aea3-87a3f848f40b
2018-06-25 16:25:23.892: info: 4656: Requesting registration creation for sender: 4214b159-58d5-5484-9d08-3c2553240f09
2018-06-25 16:25:23.986: info: 4656: Requesting registration creation for receiver: 45a18912-db55-5953-a9f8-b87f4d70d386
```

This shows the nmos-cpp-node starting up and advertising its Node API via DNS Service Discovery.

It selects an NMOS Registry to use, and registers itself, according to the NMOS [Discovery: Registered Operation](https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/docs/3.1.%20Discovery%20-%20Registered%20Operation.md) procedure.

On the other side, this operation should be reflected in the nmos-cpp-registry output something like this:

```
2018-06-25 16:25:23.580: info: 11976: Registration requested for node: 172bbbe0-a2da-5179-8a24-f2f288eeff75
2018-06-25 16:25:23.676: info: 11948: Registration requested for device: b89caa85-556f-52e4-aec3-2c625a314bb5 on node: 172bbbe0-a2da-5179-8a24-f2f288eeff75
2018-06-25 16:25:23.777: info: 11836: Registration requested for source: baffbc06-cf83-59a5-9969-6494c6bc1b2e on device: b89caa85-556f-52e4-aec3-2c625a314bb5
2018-06-25 16:25:23.888: info: 8344: Registration requested for flow: b8b8ab2b-aab8-50a3-aea3-87a3f848f40b on source: baffbc06-cf83-59a5-9969-6494c6bc1b2e
2018-06-25 16:25:23.982: info: 11200: Registration requested for sender: 4214b159-58d5-5484-9d08-3c2553240f09 on device: b89caa85-556f-52e4-aec3-2c625a314bb5
2018-06-25 16:25:24.089: info: 11836: Registration requested for receiver: 45a18912-db55-5953-a9f8-b87f4d70d386 on device: b89caa85-556f-52e4-aec3-2c625a314bb5
```

## Adjust settings of a running nmos-cpp application

A few settings may be changed dynamically by PATCH to /settings/all on the experimental Settings API.

For example:

```
curl -X PATCH -H "Content-Type: application/json" http://localhost:3209/settings/all -d "{\"logging_level\":-40}"
curl -X PATCH -H "Content-Type: application/json" http://localhost:3209/settings/all -T config.json
```
