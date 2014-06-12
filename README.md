Aura
====
[![Build Status](https://secure.travis-ci.org/Josko/aura-bot.png?branch=master)](http://travis-ci.org/Josko/aura-bot)

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
* Uses C++11
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

Windows users must use VS2013 or later. Visual Studio 2013 Express for Windows Desktop works.
Neccessary .sln and .vcxproj files are provided. Before building, choose the Release configuration and Win32 or x64 as the platform.
The binary shall be generated in the `..\aura-bot\aura\Release` folder.

### Linux

Linux users will probably need some packages for it to build:

* Debian/Ubuntu -- `apt-get install build-essential m4 libgmp3-dev cmake`
* Arch Linux -- `pacman -S base-devel cmake`

For building StormLib execute the following commands (line by line):	
	
	cd aura-bot/StormLib
	mkdir build
	cd build
	cmake ..
	make
	sudo make install

Continue building bncsutil:

	cd ../..
	cd bncsutil/src/bncsutil
	make
	sudo make install	
	
Then proceed to build Aura:

	cd ../../..
	cd aura
	make && make install
	cd ..
	
Now you can run Aura by executing `./aura++`.

**Note**: gcc version needs to be 4.7 or higher along with a compatible libc.

**Note**: StormLib installs itself in `/usr/local/lib` which isn't in PATH by default
on some distros such as Arch or CentOS.

### OS X

Building `aura++` is verified to work with Xcode 5 (on 10.9) but it should
be possible with Xcode 4 as well 
(as long as your OSX version is not below 10.7). 

To build `aura++` you need at least 
[Xcode](https://developer.apple.com/xcode/), downloadable from the Mac App 
Store. In case you don't want to use the Xcode projects to compile `aura++` you 
also have to download the Command Line Tools, available in Apple's Developer 
Area (a free membership is sufficient). 

As a further prerequisite you need a more recent version of `libgmp`.
The easiest way to get one is by using a package manager like 
[Homebrew](http://brew.sh/).
After a successful install open (or reopen) `Terminal.app` located in 
`/Applications/Utilities` and execute 


	brew install gmp
   
No matter how you acquire `libgmp`, the makefiles as well as
Xcode projects expect it in `/usr/lib` or `/usr/local/lib`.
You might want to adjust these files in case you choose a different path.

You can now build `aura++` (and the dependencies StormLib & bncsutil)
by either using the respective makefiles (see the Linux instructions above) or
by opening `aura++.xcodeproj` in Xcode. Press 'Build' to compile StormLib, 
bncsutil and `aura++` in one go. 
Now you can run aura by executing `./aura++` in Terminal.

**Note**: StormLib and bncsutil are linked dynamically, thus you should
refrain from deleting their build results.

**Note**: the Xcode project file in `StormLib/` is *not* the one produced by
`cmake`. 
Please keep this in mind in case you generate a new StormLib Xcode project with 
`cmake` — it might break `aura++.xcodeproj`.

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
