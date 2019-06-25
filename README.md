# An NMOS C++ Implementation

## Introduction

This repository contains an implementation of the [AMWA Networked Media Open Specifications](https://github.com/AMWA-TV/nmos) in C++, [licensed](LICENSE) under the terms of the Apache License 2.0.

- [AMWA IS-04 NMOS Discovery and Registration Specification](https://github.com/AMWA-TV/nmos-discovery-registration)
- [AMWA IS-05 NMOS Connection Management Specification](https://github.com/AMWA-TV/nmos-device-connection-management)
- [AMWA IS-07 NMOS Event & Tally Specification](https://github.com/AMWA-TV/nmos-event-tally)
- [AMWA IS-09 NMOS System Specification](https://github.com/AMWA-TV/nmos-system) (originally defined in JT-NM TR-1001-1:2018 Annex A)
- [AMWA BCP-002-01 NMOS Grouping Recommendations](https://github.com/AMWA-TV/nmos-grouping) - Natural Grouping
- [AMWA BCP-003-01 NMOS API Security Recommendations](https://github.com/AMWA-TV/nmos-api-security) - Securing Communications

This software is a **work in progress**, tracking the ongoing development of the NMOS specifications in the AMWA Networked Media Incubator. For more information about AMWA, NMOS and the Networked Media Incubator, please refer to http://amwa.tv/.

- The [nmos module](Development/nmos) includes implementations of the NMOS Node, Registration and Query APIs, and the NMOS Connection API.
- The [nmos-cpp-registry application](Development/nmos-cpp-registry) provides a simple but functional instance of an NMOS Registration & Discovery System (RDS), utilising the nmos module.
- The [nmos-cpp-node application](Development/nmos-cpp-node) provides an example NMOS Node, also utilising the nmos module.

The [repository structure](Documents/Repository-Structure.md), and the [external dependencies](Documents/Dependencies.md), are outlined in the documentation.
Some information about the overall design of **nmos-cpp** is also included in the [architecture](Documents/Architecture.md) documentation.

### Getting Started

The codebase is intended to be portable, and the nmos-cpp [CMake project](Development/CMakeLists.txt) can be built on at least Windows and Linux.

After setting up the dependencies, follow these [instructions](Documents/Getting-Started.md) to build nmos-cpp on your platform, and run the test suite.

Next, try out the registry and node applications in the [tutorial](Documents/Tutorial.md).

## Work In Progress

The implementation is functional and has been used successfully in several Networked Media Incubator workshop "plug-fests", interoperating with other NMOS implementations, and is being used to build NMOS support for several products.

The implementation is incomplete in some areas. Development is ongoing! The NMOS specifications are being continuously developed, as enhancements are proposed and prototyped by the Incubator participants.

Other open-source NMOS implementations:

- [BBC Reference Implementation](https://github.com/bbc/nmos-discovery-registration-ri), in Python
- [Streampunk Ledger](https://github.com/streampunk/ledger), in Node.js

### Active Development

Recent activity on the project (newest first):

- Updates for resolutions of specification issues in IS-04 v1.3 and IS-05 v1.1 final drafts
- Experimental support for human-readable HTML rendering of NMOS responses
- Experimental support for the rehomed (work in progress) IS-09 System API (originally defined in JT-NM TR-1001-1:2018 Annex A)
- IS-07 Events API and Events WebSocket API implementation and updated nmos-cpp-node example
- Experimental support for secure communications (HTTPS, WSS)
- Bug fixes (with test cases added to the [AMWA NMOS API Testing Tool](https://github.com/AMWA-TV/nmos-testing))
- Support for running nmos-cpp applications with forward/reverse proxies
- Experimental support for JT-NM TR-1001-1 System API
- Instructions for cross-compiling for the Raspberry Pi
- Instructions for running the official AMWA NMOS API Testing Tool
- Updates to build instructions and required dependencies
- Simpler creation/processing of the types of SDP files required to support ST 2110 and ST 2022-7
- Simpler run-time configuration of the **nmos-cpp-node** and **nmos-cpp-registry** settings
- Some documentation about the overall design of **nmos-cpp** for developers
- An implementation of the Connection API
- A fix for a potential memory leak
- An SDP parser/generator (to/from a JSON representation)
- JSON Schema validation in the Registration API and the Query API
- Cross-platform build support using CMake
- An initial release of the **nmos-cpp-node** example application
- Back-end enhancements as part of the NMOS Scalability Activity

And of course, the software will continue to be updated to track the ongoing development of the NMOS specifications, and the work of the Networked Media Incubator.

## Contributing

We welcome bug reports, feature requests and contributions to the implementation and documentation. Please have a look at the simple [Contribution Guidelines](CONTRIBUTING.md).

Thank you for your interest!

![This project was formerly known as sea-lion.](Documents/images/sea-lion.png?raw=true)
