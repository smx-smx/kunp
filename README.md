Github-CI:
[![Build Status][github_docker_status]][github_docker_link]
[![Build Status][github_linux_status]][github_linux_link]
[![Build Status][github_macos_status]][github_macos_link]
[![Build Status][github_windows_status]][github_windows_link]

[github_docker_status]: https://github.com/Mizux/KalistoUnpacker/workflows/Docker/badge.svg
[github_docker_link]: https://github.com/Mizux/KalistoUnpacker/actions?query=workflow%3ADocker

[github_linux_status]: https://github.com/Mizux/KalistoUnpacker/workflows/Linux/badge.svg
[github_linux_link]: https://github.com/Mizux/KalistoUnpacker/actions?query=workflow%3ALinux

[github_macos_status]: https://github.com/Mizux/KalistoUnpacker/workflows/MacOS/badge.svg
[github_macos_link]: https://github.com/Mizux/KalistoUnpacker/actions?query=workflow%3AMacOS

[github_windows_status]: https://github.com/Mizux/KalistoUnpacker/workflows/Windows/badge.svg
[github_windows_link]: https://github.com/Mizux/KalistoUnpacker/actions?query=workflow%3AWindows


# Introduction
<nav for="project"> |
<a href="#codemap">Codemap</a> |
<a href="#build">Build</a> |
<a href="#test">Test</a> |
<a href="#install">Install</a> |
<a href="ci/README.md">CI</a> |
<a href="#license">License</a> |
</nav>
Unpacker for Kalisto data files (`.KBF`/`.KIX`) found in the "New York Race" (NYR) video game.

NYR is a flying car racing game developed by Kalisto Entertainment and released in 2001.
It is based on [The Fifth Element](https://en.wikipedia.org/wiki/The_Fifth_Element) film.

Based on my discoveries, the KBF extension stands for "Kalisto Binary File" and KIX stands for "Kalisto Index File".
I don't own any other Kalisto-made game to verify if other games also use the same binary format, but you're free to try this tool and see if it works for you

# [Codemap](#codemap)
Thus the project layout is as follow:

* [CMakeLists.txt](CMakeLists.txt) Top-level for [CMake](https://cmake.org/cmake/help/latest/) based build.
* [cmake](cmake) Subsidiary CMake files.

* [ci](ci) Root directory for continuous integration.

* [libkunpacker](libkunpacker) Root directory for `libkunpacker` library.
  * [CMakeLists.txt](libkunpacker/CMakeLists.txt) for `libkunpacker`.
  * [include](libkunpacker/include) public folder.
    * [kunpacker.hpp](libkunpacker/include/kunpacker.hpp)
  * [src](libkunpacker/src) private folder.
    * [kunpacker.cpp](libkunpacker/src/kunpacker.cpp)
* [kunpacker](kunpacker) Root directory for `kunpacker` cli application.
  * [CMakeLists.txt](kunpacker/CMakeLists.txt) for `kunpacker`.
  * [src](kunpacker/src) private folder.
    * [main.cpp](kunpacker/src/main.cpp)

# [C++ Project Build](#build)
To build the C++ project, as usual:
```sh
cmake -S. -Bbuild -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target all -v
```

note: replace `all` by `ALL_BUILD` for non makefile generators.

# [Test C++ Project](#test)
To test the C++ project, as usual:
```sh
cmake --build build --config Release --target test -v
```

note: replace `test` by `RUN_TESTS` for non makefile generators.

# [Install C++ Project](#install)
To install the C++ project, as usual:
```sh
cmake --build build --config Release --target install -v
```

note: replace `install` by `INSTALL` for non makefile generators.

# [License](#license)
Apache 2. See the LICENSE file for details.
