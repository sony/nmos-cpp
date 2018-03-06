# An NMOS C++ Implementation

## Introduction

This repository contains an implementation of the [AMWA Networked Media Open Specifications](https://github.com/AMWA-TV/nmos) in C++, [licensed](LICENSE) under the terms of the Apache License 2.0.

- [NMOS Discovery and Registration Specification](https://github.com/AMWA-TV/nmos-discovery-registration) (IS-04)
- [NMOS Connection Management Specification](https://github.com/AMWA-TV/nmos-device-connection-management) (IS-05)

This software is a **work in progress**, tracking the ongoing development of the NMOS specifications in the AMWA Networked Media Incubator. For more information about AMWA, NMOS and the Networked Media Incubator, please refer to http://amwa.tv/.

- The [nmos module](Development/nmos) includes implementations of the NMOS Node, Registration and Query APIs, and the NMOS Connection API.
- The [nmos-cpp-registry application](Development/nmos-cpp-registry) provides a simple but functional instance of an NMOS Registration & Discovery System (RDS), utilising the nmos module.
- The [nmos-cpp-node application](Development/nmos-cpp-node) provides an example NMOS Node, also utilising the nmos module.

The [repository structure](Documents/Repository-Structure.md), and the [external dependencies](Documents/Dependencies.md), are outlined in the documentation.

### Getting Started

The codebase is intended to be portable, and the nmos-cpp [CMake project](Development/CMakeLists.txt) can be built on at least Windows and Linux.

After setting up the dependencies, follow these [instructions](Documents/Getting-Started.md) to build nmos-cpp on your platform, and run the test suite.

Next, try out the registry and node applications in the [tutorial](Documents/Tutorial.md).

## Work In Progress

The implementation is functional and has been used successfully in several Networked Media Incubator workshop "plug-fests", interoperating with other NMOS implementations.

The implementation is incomplete in some areas. Development is ongoing! The NMOS specifications are being continuously developed, as enhancements are proposed and prototyped by the Incubator participants.

Other open-source NMOS implementations:

- [BBC Reference Implementation](https://github.com/bbc/nmos-discovery-registration-ri), in Python
- [Streampunk Ledger](https://github.com/streampunk/ledger), in Node.js

### Active Development

Recent activity on the project:

- Cross-platform build support using CMake
- An initial release of the **nmos-cpp-node** example application
- Back-end enhancements as part of the NMOS Scalability Activity

In the next few weeks, we intend to bring to this repository:

- Instructions for configuring a CMake toolchain for cross-compiling for embedded Linux on ARM
- More of a unit test suite, and automated integration testing (the [AMWA NMOS Automated Testing](https://github.com/AMWA-TV/nmos-automated-testing) Activity is also a work in progress)
- Additional documentation

And of course, the software will continue to be updated to track the ongoing development of the NMOS specifications, and the work of the Networked Media Incubator.

## Contributing

We welcome bug reports, feature requests and contributions to the implementation and documentation. Please have a look at the simple [Contribution Guidelines](CONTRIBUTING.md).

Thank you for your interest!

![This project was formerly known as sea-lion.](Documents/images/sea-lion.png?raw=true)
