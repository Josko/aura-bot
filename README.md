Aura
====
[![Build Status](https://secure.travis-ci.org/Josko/aura-bot.png?branch=master)](http://travis-ci.org/Josko/aura-bot)
[![Build Status](https://ci.appveyor.com/api/projects/status/u67db01q5nbt172l/branch/master?svg=true)](https://ci.appveyor.com/project/Josko/aura-bot/branch/master)
[![Build Status](https://scan.coverity.com/projects/1748/badge.svg)](https://scan.coverity.com/projets/josko-aura-bot)

Overview
--------

Aura is a Warcraft III hosting-bot based on GHost++ by Varlock. It's a complete
overhaul with speed and efficiency in mind and packed with fewer dependencies.

Removed features from GHost++:
* No MySQL support
* No autohost
* No admin game
* No language.cfg
* No W3MMD support
* No replay saving
* No save/load games
* No BNLS support
* No boost required

Other changes:
* Uses C++14
* Single-threaded
* Has a Windows 64-bit build
* Uses SQLite and a different database organization.
* Tested on OS X (see [Building -> OS X](#os-x) for detailed requirements)
* A lot of code removed, about 1 MB smaller binary on Linux
* Updated libraries: StormLib, SQLite, zlib
* Connects to and can be controlled via IRC
* Using aggressive optimizations
* Up to 11 fakeplayers can be added.
* Uses DotA stats automagically on maps with 'DotA' in the filename
* Auto spoofcheck in private games on PvPGNs
* More commands added either ingame or bnet
* Checked with various tools such as clang-analyzer and cppcheck

Multi-platform
--------------

The bot runs on little-endian Linux (32-bit and 64-bit), Windows (32-bit and 64-bit) and OS X (64-bit Intel CPU) machines.

Building
--------

### Windows

Windows users must use VS2015 or later. Visual Studio 2015 Community edition works.
Neccessary .sln and .vcxproj files are provided. Before building, choose the Release configuration and Win32 or x64 as the platform.
The binary shall be generated in the `..\aura-bot\aura\Release` folder.

### Linux

Linux users will probably need some packages for it to build:

* Debian/Ubuntu -- `apt-get install build-essential m4 libgmp3-dev cmake`
* Arch Linux -- `pacman -S base-devel cmake`

#### Steps

For building StormLib execute the following commands (line by line):

	cd aura-bot/StormLib
	mkdir build
	cd build
	cmake -DCMAKE_BUILD_TYPE=Release ..
	make
	sudo make install

Continue building bncsutil:

	cd ../..
	cd bncsutil/src/bncsutil
	make
	sudo make install

Then proceed to build Aura:

	cd ../../..
	make

Now you can run Aura by executing `./aura++` or install it to your path using `sudo make install`.

**Note**: gcc version needs to be 5 or higher along with a compatible libc.

**Note**: clang needs to be 3.6 or higher along with ld gold linker (ie. package binutils-gold for ubuntu)

**Note**: StormLib installs itself in `/usr/local/lib` which isn't in PATH by default
on some distros such as Arch or CentOS.

### OS X

#### Requirements

* OSX ≥10.9, possibly even higher for necessary C++14 support. It is verified to work and tested on OSX 10.11.
* Latest available Xcode for your platform and/or the Xcode Command Line Tools.
One of these might suffice, if not just get both.
* A recent `libgmp`.

You can use [Homebrew](http://brew.sh/) to get `libgmp`. When you are at it, you can also use it to install StormLib instead of compiling it on your own:

	brew install gmp
	brew install stormlib   # optional

Now proceed by following the [steps for Linux users](#Steps) and omit StormLib in case you installed it using `brew`.


Configuring
-----------

Modify the `aura.cfg` file to configure the bot to your wishes.

Credits
-------

* Varlock -- the author of the GHost++ bot
* Argon- -- suggestions, code, bug fixes, testing and OS X support
* Joakim -- testing and bug reports
* PhillyPhong -- testing and bug reports

Contributing
------------

That would be lovely.

1. Fork it.
2. Create a branch (`git checkout -b my_aura`)
3. Commit your changes (`git commit -am "Fixed a crash when using GProxy++"`)
4. Push to the branch (`git push origin my_aura`)
5. Create an [Issue][1] with a link to your branch or make Pull Request
6. Enjoy a beer and wait

[1]: https://github.com/Josko/aura-bot/issues
