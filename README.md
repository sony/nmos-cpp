# An NMOS C++ Implementation [![Build Status](https://github.com/sony/nmos-cpp/workflows/build-test/badge.svg)][build-test]
[build-test]: https://github.com/sony/nmos-cpp/actions?query=workflow%3Abuild-test

## Introduction

This repository contains an implementation of the [AMWA Networked Media Open Specifications](https://amwa-tv.github.io/nmos) in C++, [licensed](LICENSE) under the terms of the Apache License 2.0.

- [AMWA IS-04 NMOS Discovery and Registration Specification](https://amwa-tv.github.io/nmos-discovery-registration)
- [AMWA IS-05 NMOS Connection Management Specification](https://amwa-tv.github.io/nmos-device-connection-management)
- [AMWA IS-07 NMOS Event & Tally Specification](https://amwa-tv.github.io/nmos-event-tally)
- [AMWA IS-09 NMOS System Specification](https://amwa-tv.github.io/nmos-system) (originally defined in JT-NM TR-1001-1:2018 Annex A)
- [AMWA BCP-002-01 NMOS Grouping Recommendations](https://amwa-tv.github.io/nmos-grouping) - Natural Grouping
- [AMWA BCP-003-01 NMOS API Security Recommendations](https://amwa-tv.github.io/nmos-api-security) - Securing Communications

For more information about AMWA, NMOS and the Networked Media Incubator, please refer to http://amwa.tv/.

- The [nmos module](Development/nmos) includes implementations of the NMOS Node, Registration and Query APIs, the NMOS Connection API, and so on.
- The [nmos-cpp-registry application](Development/nmos-cpp-registry) provides a simple but functional instance of an NMOS Registration & Discovery System (RDS), utilising the nmos module.
- The [nmos-cpp-node application](Development/nmos-cpp-node) provides an example NMOS Node, also utilising the nmos module.

The [repository structure](Documents/Repository-Structure.md), and the [external dependencies](Documents/Dependencies.md), are outlined in the documentation.
Some information about the overall design of **nmos-cpp** is also included in the [architecture](Documents/Architecture.md) documentation.

### Getting Started

The codebase is intended to be portable, and the nmos-cpp [CMake project](Development/CMakeLists.txt) can be built on at least Linux, Windows and macOS.

After setting up the dependencies, follow these [instructions](Documents/Getting-Started.md) to build nmos-cpp on your platform, and run the test suite.

Next, try out the registry and node applications in the [tutorial](Documents/Tutorial.md).

## Agile Approach

> Ready for deployment, continual developments

The nmos-cpp applications have been successfully tested in many AMWA Networked Media Incubator workshops, and are used as reference NMOS implementations in the [JT-NM Tested](https://jt-nm.org/jt-nm_tested/) programme.
Several vendors have deployed JT-NM Tested badged products, using nmos-cpp, to their customers.

### Build Status

The following configurations, defined by the [build-test](.github/workflows/src/build-test.yml) jobs, are built and unit tested automatically via continuous integration.

| Platform | Version                  | Configuration Options                  |
|----------|--------------------------|----------------------------------------|
| Linux    | Ubuntu 18.04 (GCC 7.5.0) | Avahi                                  |
| Linux    | Ubuntu 18.04 (GCC 7.5.0) | mDNSResponder                          |
| Linux    | Ubuntu 14.04 (GCC 4.8.4) | mDNSResponder, not using Conan         |
| Windows  | Server 2019 (VS 2019)    | Bonjour (mDNSResponder)                |
| macOS    | 10.15 (AppleClang 11.0)  |                                        |

The [AMWA NMOS API Testing Tool](https://github.com/AMWA-TV/nmos-testing) is automatically run against the APIs of the **nmos-cpp-node** and **nmos-cpp-registry** applications.

**Test Suite/Status:**
[![IS-04-01][IS-04-01-badge]][IS-04-01-sheet]
[![IS-04-02][IS-04-02-badge]][IS-04-02-sheet]
[![IS-04-03][IS-04-03-badge]][IS-04-03-sheet]
[![IS-05-01][IS-05-01-badge]][IS-05-01-sheet]
[![IS-05-02][IS-05-02-badge]][IS-05-02-sheet]
[![IS-07-01][IS-07-01-badge]][IS-07-01-sheet]
[![IS-07-02][IS-07-02-badge]][IS-07-02-sheet]
[![IS-09-01][IS-09-01-badge]][IS-09-01-sheet]
[![IS-09-02][IS-09-02-badge]][IS-09-02-sheet]

[IS-04-01-badge]: https://drive.google.com/open?id=1s4mKwzujyoqS976r-DCa7pwDEQqvSdfP
[IS-04-02-badge]: https://drive.google.com/open?id=1halAIV6n6Kk7lGwBRRuHgpR2jaK2x8GC
[IS-04-03-badge]: https://drive.google.com/open?id=1TGcOAvt1FV64LktC6DcUu5E5OHM-Xvm6
[IS-05-01-badge]: https://drive.google.com/open?id=1x_HtDNds4PaTpB67Gx39AdwPOc0qVbmF
[IS-05-02-badge]: https://drive.google.com/open?id=1WC4rcR-FtIl2bwiXZ7LCt55po3tEz9Fo
[IS-07-01-badge]: https://drive.google.com/open?id=1S4qf7d6056utc8W5Bfg0l2HhiW0sZNmH
[IS-07-02-badge]: https://drive.google.com/open?id=13ib46LMAYzROJmARGKUyfEsvTM-n6L0N
[IS-09-01-badge]: https://drive.google.com/open?id=1_WvtE72R8kvF5x4x7KLdejAWd2eZauUE
[IS-09-02-badge]: https://drive.google.com/open?id=14TgNmeAZVCyfWMlAtqnFGiVq8WwbYhXq
[IS-04-01-sheet]: https://docs.google.com/spreadsheets/d/1UgZoI0lGCMDn9-zssccf2Azil3WN6jogroMT8Wh6H64/edit#gid=0
[IS-04-02-sheet]: https://docs.google.com/spreadsheets/d/1UgZoI0lGCMDn9-zssccf2Azil3WN6jogroMT8Wh6H64/edit#gid=1838684224
[IS-04-03-sheet]: https://docs.google.com/spreadsheets/d/1UgZoI0lGCMDn9-zssccf2Azil3WN6jogroMT8Wh6H64/edit#gid=1174955447
[IS-05-01-sheet]: https://docs.google.com/spreadsheets/d/1UgZoI0lGCMDn9-zssccf2Azil3WN6jogroMT8Wh6H64/edit#gid=517163955
[IS-05-02-sheet]: https://docs.google.com/spreadsheets/d/1UgZoI0lGCMDn9-zssccf2Azil3WN6jogroMT8Wh6H64/edit#gid=205041321
[IS-07-01-sheet]: https://docs.google.com/spreadsheets/d/1UgZoI0lGCMDn9-zssccf2Azil3WN6jogroMT8Wh6H64/edit#gid=828991990
[IS-07-02-sheet]: https://docs.google.com/spreadsheets/d/1UgZoI0lGCMDn9-zssccf2Azil3WN6jogroMT8Wh6H64/edit#gid=367400040
[IS-09-01-sheet]: https://docs.google.com/spreadsheets/d/1UgZoI0lGCMDn9-zssccf2Azil3WN6jogroMT8Wh6H64/edit#gid=919453974
[IS-09-02-sheet]: https://docs.google.com/spreadsheets/d/1UgZoI0lGCMDn9-zssccf2Azil3WN6jogroMT8Wh6H64/edit#gid=2135469955

### Recent Activity

The implementation is incomplete in some areas. Development is ongoing, tracking the evolution of the NMOS specifications in the AMWA Networked Media Incubator.

Recent activity on the project (newest first):

- Switched Continous Integration to GitHub Actions and added Windows and macOS to the tested platforms
- Extended the **nmos-cpp-node** to include mock senders/receivers of audio and ancillary data and offer some additional configuration settings
- Simplified the build process to use Conan by default to download most of the dependencies
- Added support in the Node implementation for discovery of, and interaction with, a System API, as required for compliance with TR-1001-1
- Changed the implementation of `nmos::tai_clock` with the effect that it may no longer be monotonic
- Added a minimum viable LLDP implementation (enabled by a CMake configuration option) to support sending and receiving the IS-04 v1.3 additional network data for Nodes required by IS-06
- Update the IS-05 schemas to correct an unfortunate bug in the IS-05 v1.1 spec (see [AMWA-TV/nmos-device-connection-management#99](https://github.com/AMWA-TV/nmos-device-connection-management/pull/99))
- Attempt to determine the DNS domain name automatically if not explicitly specified, for TR-1001-1
- Travis CI integration
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

## Contributing

We welcome bug reports, feature requests and contributions to the implementation and documentation. Please have a look at the simple [Contribution Guidelines](CONTRIBUTING.md).

Thank you for your interest!

![This project was formerly known as sea-lion.](Documents/images/sea-lion.png?raw=true)
