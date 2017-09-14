# An NMOS C++ Implementation

## Introduction

This repository contains an implementation of the [AMWA Networked Media Open Specifications](https://github.com/AMWA-TV/nmos) in C++, [licensed](LICENSE) under the terms of the Apache License 2.0.

- [NMOS Discovery and Registration Specification](https://github.com/AMWA-TV/nmos-discovery-registration) (IS-04)
- [NMOS Connection Management Specification](https://github.com/AMWA-TV/nmos-device-connection-management) (IS-05)

This software is a **work in progress**, tracking the ongoing development of the NMOS specifications in the AMWA Networked Media Incubator. For more information about AMWA, NMOS and the Networked Media Incubator, please refer to http://amwa.tv/.

- The [nmos module](Development/nmos) includes implementations of the NMOS Node, Registration and Query APIs, and the NMOS Connection API.
- The [nmos-cpp-registry application](Development/nmos-cpp-registry) provides a simple but functional instance of an NMOS Registration & Discovery System (RDS), utilising the nmos module.

The [repository structure](Documents/Repository-Structure.md), and the [external dependencies](Documents/Dependencies.md), are outlined in the documentation.

### Getting Started

Windows users with Visual Studio 2013 currently have the easiest means to build and run the nmos-cpp applications.

1. Follow these [Preparation](Documents/Dependencies.md#preparation) instructions
2. Open and build the provided [nmos-cpp Visual Studio Solution](Development/nmos-cpp.sln)

The codebase is intended to be portable, and Mac OS X (Xcode) and Linux (Makefile) projects will be provided *soon*.

## Work In Progress

The implementation is functional and has been used successfully in several Networked Media Incubator workshop "plug-fests", interoperating with other NMOS implementations.

The implementation is incomplete in some areas. Development is ongoing! The NMOS specifications are being continuously developed, as enhancements are proposed and prototyped by the Incubator participants.

Other open-source NMOS implementations:

- [BBC Reference Implementation](https://github.com/bbc/nmos-discovery-registration-ri), in Python
- [Streampunk Ledger](https://github.com/streampunk/ledger), in Node.js

### Active Development

In the next few weeks, we intend to bring to this repository:

- An **nmos-cpp-node** application providing an example implementation of an NMOS Node utilising the [nmos module](Development/nmos)
- The aforementioned project/build files for Mac OS X and Linux
- More of a unit test suite, and automated integration testing (the [AMWA NMOS Automated Testing](https://github.com/AMWA-TV/nmos-automated-testing) Activity is also a work in progress)
- Additional documentation
- Back-end enhancements as part of the NMOS Scalability Activity

And of course, the software will continue to be updated to track the ongoing development of the NMOS specifications, and the work of the Networked Media Incubator.

## Contributing

We welcome bug reports and feature requests for the implementation. These should be submitted as Issues on GitHub:

- [Sign up](https://github.com/join) for GitHub if you haven't already done so.
- Check whether someone has already submitted a similar Issue.
- If necessary, submit a new Issue.

Thank you for your interest!

![This project was formerly known as sea-lion.](Documents/images/sea-lion.png?raw=true)
