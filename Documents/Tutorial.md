# Tutorial

The following instructions describe how to run the NMOS Registry and some example NMOS Nodes provided by nmos-cpp.

## Preparation

Follow the [Getting Started](Getting-Started.md) instructions before proceeding.

Notes:
- On Windows, the correct configuration of the C++ REST SDK library (e.g. cpprestsdk140_2_10.dll or cpprest140d_2_10.dll) needs to be on the ``PATH``.
- On Windows, the nmos-cpp apps may need to be run with administrator privileges on Windows. This is explained in the [build instructions for the C++ REST SDK](Dependencies.md#c-rest-sdk).
- The Bonjour service (Windows) or ``mdnsd`` daemon (Linux) must be started to enable NMOS Nodes, and the Registry, to use DNS Service Discovery.
- When running more than one nmos-cpp application on the same host, configuration parameters **must** be used to select unique port numbers, as described below.

## Start an NMOS Registry

Run the **nmos-cpp-registry** application. Configuration parameters may be passed on the command-line, or in a configuration file, as JSON. Each named parameter is specified as a field in a JSON object.
The provided [nmos-cpp-registry/config.json](../Development/nmos-cpp-registry/config.json) may be used as a starting point, or refer directly to the source code in [nmos/settings.h](../Development/nmos/settings.h).
Note: C++/JavaScript-style single and multi-line comments are permitted and ignored in nmos-cpp config files.

For example, to launch the application without verbose logging, try ``nmos-cpp-registry config.json`` with a file config.json like:

```json
{
    "logging_level": 0,
    "domain": "local."
}
```

By default, when the nmos-cpp applications discover a DNS domain, they adopt unicast DNS-SD, but this requires the necessary service configuration in the DNS server.
For this initial tutorial, forcing mDNS operation may be simpler, by specifying ``"domain": "local."`` as above.

The initial output should appear something like this:

```
2020-04-07 11:06:58.777: info: 22984: Starting nmos-cpp registry
2020-04-07 11:06:58.807: info: 22984: Process ID: 12232
2020-04-07 11:06:58.807: info: 22984: Build settings: cpprestsdk/2.10.15 (listener=httpsys; client=winhttp); WebSocket++/0.8.1; Boost 1.72.0; OpenSSL 1.1.1e  17 Mar 2020
2020-04-07 11:06:58.807: info: 22984: Initial settings: {"domain":"local.","host_address":"10.0.0.1","host_addresses":["10.0.0.1"],"logging_level":0,"seed_id":"bb8f8efb-f05b-4cb4-ba77-deb8fd750d48"}
2020-04-07 11:06:58.807: info: 22984: Configuring nmos-cpp registry with its primary Node API at: 10.0.0.1:3212
2020-04-07 11:06:58.807: info: 22984: Configuring nmos-cpp registry with its primary Registration API at: 10.0.0.1:3210
2020-04-07 11:06:58.807: info: 22984: Configuring nmos-cpp registry with its primary Query API at: 10.0.0.1:3211
2020-04-07 11:07:00.395: info: 22984: Preparing for connections
2020-04-07 11:07:00.421: info: 22984: Ready for connections
2020-04-07 11:07:00.422: info: 28092: Registered advertisement for: nmos-cpp_query_10-0-0-1:3211._nmos-query._tcp.local.
2020-04-07 11:07:00.425: info: 28092: Registered advertisement for: nmos-cpp_registration_10-0-0-1:3210._nmos-registration._tcp.local.
2020-04-07 11:07:00.428: info: 28092: Registered advertisement for: nmos-cpp_registration_10-0-0-1:3210._nmos-register._tcp.local.
2020-04-07 11:07:00.432: info: 28092: Registered advertisement for: nmos-cpp_node_10-0-0-1:3212._nmos-node._tcp.local.
2020-04-07 11:07:00.435: info: 28092: Registered advertisement for: nmos-cpp_system_10-0-0-1:10641._nmos-system._tcp.local.
```

This shows the nmos-cpp-registry starting up and advertising its APIs via DNS Service Discovery.

## Start several NMOS Nodes

Run the **nmos-cpp-node** application one or more times. Like the registry application, configuration parameters may be passed on the command-line as JSON.
The provided [nmos-cpp-node/config.json](../Development/nmos-cpp-node/config.json) may be used as a starting point, or refer directly to the source code in [nmos/settings.h](../Development/nmos/settings.h).
Note: C++/JavaScript-style single and multi-line comments are permitted and ignored in nmos-cpp config files.

When running more than one nmos-cpp application on the same host, configuration parameters **must** be used to make the port numbers of each instance unique.
There are several APIs to consider, so for brevity, the default port for _all_ the APIs can be overridden by using the ``"http_port"`` parameter.
(WebSocket APIs are served separately so ``"events_ws_port"`` must also be specified when running multiple nmos-cpp-node instances.)

For example, for the first Node, try ``nmos-cpp-node config.json`` with a file config.json like:

```json
{
    "logging_level": 0,
    "domain": "local.",
    "http_port": 8080,
    "events_ws_port": 8081
}
```

Each additional Node should be configured with different values of `"http_port"` and `"events_ws_port"`.

Note that some optional APIs can be disabled by specifying port -1, including ``"system_port": -1`` on the Registry and ``"events_port": -1`` on the Nodes.

The initial output of each Node instance should appear something like this:

```
2020-04-07 11:07:08.754: info: 13380: Starting nmos-cpp node
2020-04-07 11:07:08.780: info: 13380: Process ID: 28312
2020-04-07 11:07:08.780: info: 13380: Build settings: cpprestsdk/2.10.15 (listener=httpsys; client=winhttp); WebSocket++/0.8.1; Boost 1.72.0; OpenSSL 1.1.1e  17 Mar 2020
2020-04-07 11:07:08.780: info: 13380: Initial settings: {"connection_port":8080,"domain":"local.","events_port":8080,"events_ws_port":8081,"host_address":"10.0.0.2","host_addresses":["10.0.0.2"],"http_port":8080,"logging_level":0,"logging_port":8080,"manifest_port":8080,"node_port":8080,"seed_id":"b4e2edf5-b093-4a53-807c-ac5a7a327cef","settings_port":8080}
2020-04-07 11:07:08.781: info: 13380: Configuring nmos-cpp node with its primary Node API at: 10.0.0.2:8080
2020-04-07 11:07:08.867: info: 13380: Preparing for connections
2020-04-07 11:07:08.875: info: 10636: Registered advertisement for: nmos-cpp_node_10-0-0-2:8080._nmos-node._tcp.local.
2020-04-07 11:07:08.880: info: 13380: Ready for connections
2020-04-07 11:07:08.892: info: 16596: Attempting discovery of a System API in domain: local.
2020-04-07 11:07:08.892: info: 26840: Attempting discovery of a Registration API in domain: local.
2020-04-07 11:07:08.903: info: 23876: Updated model with node: 9b52923d-25b7-51a6-90b7-a9ef62036067
2020-04-07 11:07:08.915: info: 23876: Updated model with device: 36065208-7d6f-5c49-a5c9-5486ccb37259
2020-04-07 11:07:08.927: info: 23876: Updated model with source: a10d1228-ed9f-5cbe-9187-25c34a021f93
2020-04-07 11:07:08.938: info: 23876: Updated model with flow: 0dfb9a66-6119-5138-9da2-46724c3eaff2
2020-04-07 11:07:08.956: info: 23876: Updated model with sender: 812ad403-db09-5000-9278-2146b98acf17
2020-04-07 11:07:08.967: info: 23876: Updated model with sender: 812ad403-db09-5000-9278-2146b98acf17
2020-04-07 11:07:08.979: info: 23876: Updated model with source: e766e18c-ccdb-5f1e-90ba-1afbacef4a2d
2020-04-07 11:07:08.991: info: 23876: Updated model with flow: cf387616-192a-58d6-8990-42d37a14e136
2020-04-07 11:07:09.008: info: 23876: Updated model with sender: 86453077-e4ba-554a-9427-d00b5f57d89d
2020-04-07 11:07:09.020: info: 23876: Updated model with sender: 86453077-e4ba-554a-9427-d00b5f57d89d
2020-04-07 11:07:09.032: info: 23876: Updated model with source: 55ae34e5-64cc-5dac-aa1e-981bcec08429
2020-04-07 11:07:09.044: info: 23876: Updated model with flow: 0b4b99d4-6d96-56a7-a5e9-6095c976a502
2020-04-07 11:07:09.058: info: 23876: Updated model with sender: fb135720-ffee-563f-a20a-1b5a08c3972a
2020-04-07 11:07:09.069: info: 23876: Updated model with sender: fb135720-ffee-563f-a20a-1b5a08c3972a
2020-04-07 11:07:09.083: info: 23876: Updated model with receiver: 1064ae55-3a55-5dd1-9df9-12dc139db93a
2020-04-07 11:07:09.093: info: 23876: Updated model with receiver: 1064ae55-3a55-5dd1-9df9-12dc139db93a
2020-04-07 11:07:09.105: info: 23876: Updated model with receiver: d3f52261-7315-5649-ae83-cfa4af764164
2020-04-07 11:07:09.116: info: 23876: Updated model with receiver: d3f52261-7315-5649-ae83-cfa4af764164
2020-04-07 11:07:09.128: info: 23876: Updated model with receiver: 2e59b264-1301-57ed-b5ed-bd9035b1f94c
2020-04-07 11:07:09.139: info: 23876: Updated model with receiver: 2e59b264-1301-57ed-b5ed-bd9035b1f94c
2020-04-07 11:07:09.152: info: 23876: Updated model with source: 86b92b24-b891-54ec-989d-37937f36ff4f
2020-04-07 11:07:09.163: info: 23876: Updated model with flow: e66f0cd1-7bbc-5564-898d-7a1504cc8cee
2020-04-07 11:07:09.174: info: 23876: Updated model with sender: 01959fab-1282-57ac-80b6-9ae9615f40d7
2020-04-07 11:07:09.185: info: 23876: Updated model with sender: 01959fab-1282-57ac-80b6-9ae9615f40d7
2020-04-07 11:07:09.197: info: 23876: Updated model with source: 86b92b24-b891-54ec-989d-37937f36ff4f
2020-04-07 11:07:09.210: info: 23876: Updated model with receiver: bca2736d-7f36-5e9e-8962-00a06278a61f
2020-04-07 11:07:09.221: info: 23876: Updated model with receiver: bca2736d-7f36-5e9e-8962-00a06278a61f
2020-04-07 11:07:09.892: info: 16596: Discovered 1 System API(s)
2020-04-07 11:07:09.892: info: 16596: Attempting System API node behaviour
2020-04-07 11:07:09.892: info: 26840: Discovered 1 Registration API(s)
2020-04-07 11:07:09.893: info: 26840: Attempting initial registration
2020-04-07 11:07:09.907: info: 26840: Registering nmos-cpp node with the Registration API at: 10.0.0.1:3210
2020-04-07 11:07:09.907: info: 26840: Requesting registration creation for node: 9b52923d-25b7-51a6-90b7-a9ef62036067
2020-04-07 11:07:09.950: info: 13684: New system global configuration discovered from the System API at: http://10.0.0.1:10641/x-nmos/system/v1.0
2020-04-07 11:07:09.950: info: 26840: Adopting registered operation
2020-04-07 11:07:09.951: info: 26840: Attempting registration heartbeats with the Registration API at: 10.0.0.1:3210
2020-04-07 11:07:09.957: info: 26644: Started registered operation with Registration API at: http://10.0.0.1:3210/x-nmos/registration/v1.3
2020-04-07 11:07:09.958: info: 26840: Requesting registration creation for device: 36065208-7d6f-5c49-a5c9-5486ccb37259
2020-04-07 11:07:09.980: info: 26840: Requesting registration creation for source: a10d1228-ed9f-5cbe-9187-25c34a021f93
2020-04-07 11:07:09.997: info: 26840: Requesting registration creation for source: e766e18c-ccdb-5f1e-90ba-1afbacef4a2d
2020-04-07 11:07:10.016: info: 26840: Requesting registration creation for source: 55ae34e5-64cc-5dac-aa1e-981bcec08429
2020-04-07 11:07:10.032: info: 26840: Requesting registration creation for source: 86b92b24-b891-54ec-989d-37937f36ff4f
2020-04-07 11:07:10.048: info: 26840: Requesting registration creation for flow: 0dfb9a66-6119-5138-9da2-46724c3eaff2
2020-04-07 11:07:10.063: info: 26840: Requesting registration creation for flow: cf387616-192a-58d6-8990-42d37a14e136
2020-04-07 11:07:10.078: info: 26840: Requesting registration creation for flow: 0b4b99d4-6d96-56a7-a5e9-6095c976a502
2020-04-07 11:07:10.093: info: 26840: Requesting registration creation for flow: e66f0cd1-7bbc-5564-898d-7a1504cc8cee
2020-04-07 11:07:10.109: info: 26840: Requesting registration creation for sender: 812ad403-db09-5000-9278-2146b98acf17
2020-04-07 11:07:10.126: info: 26840: Requesting registration creation for sender: 86453077-e4ba-554a-9427-d00b5f57d89d
2020-04-07 11:07:10.144: info: 26840: Requesting registration creation for sender: fb135720-ffee-563f-a20a-1b5a08c3972a
2020-04-07 11:07:10.162: info: 26840: Requesting registration creation for sender: 01959fab-1282-57ac-80b6-9ae9615f40d7
2020-04-07 11:07:10.179: info: 26840: Requesting registration creation for receiver: 1064ae55-3a55-5dd1-9df9-12dc139db93a
2020-04-07 11:07:10.197: info: 26840: Requesting registration creation for receiver: d3f52261-7315-5649-ae83-cfa4af764164
2020-04-07 11:07:10.214: info: 26840: Requesting registration creation for receiver: 2e59b264-1301-57ed-b5ed-bd9035b1f94c
2020-04-07 11:07:10.231: info: 26840: Requesting registration creation for receiver: bca2736d-7f36-5e9e-8962-00a06278a61f
```

This shows the nmos-cpp-node starting up and advertising its Node API via DNS Service Discovery.

It selects an NMOS Registry to use, and registers itself, according to the NMOS [Discovery: Registered Operation](https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/docs/3.1.%20Discovery%20-%20Registered%20Operation.md) procedure.

On the other side, this operation should be reflected in the nmos-cpp-registry output something like this:

```
2020-04-07 11:07:09.931: info: 22476: Registration requested for node: 9b52923d-25b7-51a6-90b7-a9ef62036067
2020-04-07 11:07:09.975: info: 17100: Registration requested for device: 36065208-7d6f-5c49-a5c9-5486ccb37259 on node: 9b52923d-25b7-51a6-90b7-a9ef62036067
2020-04-07 11:07:09.994: info: 22476: Registration requested for source: a10d1228-ed9f-5cbe-9187-25c34a021f93 on device: 36065208-7d6f-5c49-a5c9-5486ccb37259
2020-04-07 11:07:10.013: info: 26032: Registration requested for source: e766e18c-ccdb-5f1e-90ba-1afbacef4a2d on device: 36065208-7d6f-5c49-a5c9-5486ccb37259
2020-04-07 11:07:10.028: info: 17100: Registration requested for source: 55ae34e5-64cc-5dac-aa1e-981bcec08429 on device: 36065208-7d6f-5c49-a5c9-5486ccb37259
2020-04-07 11:07:10.045: info: 17100: Registration requested for source: 86b92b24-b891-54ec-989d-37937f36ff4f on device: 36065208-7d6f-5c49-a5c9-5486ccb37259
2020-04-07 11:07:10.060: info: 26032: Registration requested for flow: 0dfb9a66-6119-5138-9da2-46724c3eaff2 on device: 36065208-7d6f-5c49-a5c9-5486ccb37259
2020-04-07 11:07:10.075: info: 26032: Registration requested for flow: cf387616-192a-58d6-8990-42d37a14e136 on device: 36065208-7d6f-5c49-a5c9-5486ccb37259
2020-04-07 11:07:10.090: info: 26032: Registration requested for flow: 0b4b99d4-6d96-56a7-a5e9-6095c976a502 on device: 36065208-7d6f-5c49-a5c9-5486ccb37259
2020-04-07 11:07:10.106: info: 28092: Registration requested for flow: e66f0cd1-7bbc-5564-898d-7a1504cc8cee on device: 36065208-7d6f-5c49-a5c9-5486ccb37259
2020-04-07 11:07:10.123: info: 26520: Registration requested for sender: 812ad403-db09-5000-9278-2146b98acf17 on device: 36065208-7d6f-5c49-a5c9-5486ccb37259
2020-04-07 11:07:10.141: info: 26032: Registration requested for sender: 86453077-e4ba-554a-9427-d00b5f57d89d on device: 36065208-7d6f-5c49-a5c9-5486ccb37259
2020-04-07 11:07:10.159: info: 28092: Registration requested for sender: fb135720-ffee-563f-a20a-1b5a08c3972a on device: 36065208-7d6f-5c49-a5c9-5486ccb37259
2020-04-07 11:07:10.176: info: 28092: Registration requested for sender: 01959fab-1282-57ac-80b6-9ae9615f40d7 on device: 36065208-7d6f-5c49-a5c9-5486ccb37259
2020-04-07 11:07:10.194: info: 17100: Registration requested for receiver: 1064ae55-3a55-5dd1-9df9-12dc139db93a on device: 36065208-7d6f-5c49-a5c9-5486ccb37259
2020-04-07 11:07:10.211: info: 17100: Registration requested for receiver: d3f52261-7315-5649-ae83-cfa4af764164 on device: 36065208-7d6f-5c49-a5c9-5486ccb37259
2020-04-07 11:07:10.229: info: 17100: Registration requested for receiver: 2e59b264-1301-57ed-b5ed-bd9035b1f94c on device: 36065208-7d6f-5c49-a5c9-5486ccb37259
2020-04-07 11:07:10.246: info: 26520: Registration requested for receiver: bca2736d-7f36-5e9e-8962-00a06278a61f on device: 36065208-7d6f-5c49-a5c9-5486ccb37259
```

If there are other Registries and Nodes in the network, it is possible to configure the priority of the nmos-cpp instances, using the `"pri"` setting on the Registry, and the `"highest_pri"` and `"lowest_pri"` settings on the Nodes.

## Logging to file

Logging can be redirected to text file:

```json
{
    "error_log": "log.txt"
}
```

The nmos-cpp applications open these files in a manner that allows cooperation with an external log rotation tool to prevent log files growing too large.
For example, [logrotate](https://linux.die.net/man/8/logrotate) on Linux can be used with its "copytruncate" directive.
There is a port of logrotate for Windows called [logrotatewin](https://github.com/plecos/logrotatewin).

## Adjust settings of a running nmos-cpp application

A few settings may be changed dynamically by PATCH to **/settings/all** on the experimental Settings API.

For example:

```
curl -X PATCH -H "Content-Type: application/json" http://localhost:3209/settings/all -d "{\"logging_level\":-40}"
curl -X PATCH -H "Content-Type: application/json" http://localhost:3209/settings/all -T config.json
```
