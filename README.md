Aura
====

Aura is a Warcraft III hosting-bot based on GHost++ by Varlock. It's a complete
overhaul with speed and efficiency in mind and packed with fewer dependencies.

Multi-platform
--------------

The bot is guaranteed to run on little-endian Linux (32-bit and 64-bit), Windows (32-bit and WoW64) and OS X (64-bit Intel CPU) machines.

Boost
-----

You will need  newest [Boost](http://www.boost.org/users/download/) libraries (version 1.46.0 or higher). Specifically
boost date-time, system and filesystem.

### Windows

Windows users can build by:
	
	... cd to the boost directory
	bootstrap
	bjam date_time filesystem system link=static threading=multi variant=release
	
Then move the library files from the bin.v2 folder to `~/aura-bot/aura/boost/lib` (create the folder
if it doesn't exist).

### Linux

Linux users can usually fetch them from the repo:

* Arch Linux -- `pacman -S boost`
* Debian/Ubuntu -- `apt-get install libboost-date-time-dev libboost-system-dev libboost-filesystem-dev`

### OS X

[Xcode](http://developer.apple.com/technologies/xcode.html) is the basic dependency for building anything on OS X.
Apple's development tool is shipped for free with every Mac located on your system CD/DVD.
Without Xcode installed you cannot proceed any further and won't be able to build boost.

The easiest way to go for boost is by using [Macports](http://www.macports.org/).
After a successful install open (or reopen) Terminal.app located in /Applications/Utilities and execute

	sudo port install boost
	
Building boost will take some time.

Building
--------

### Windows

Windows users should use VS2010 as there are the necessary .sln and .vcxproj files. Before
building, choose the Release option. The binary shall be generated in the `..\aura-bot\aura\Release` folder.
Run it from `aura-bot` folder.

note: supplied project settings produce a SSE2-enabled x86 binary.

### Linux

Linux users will probably need some packages for it to build:

* Debian/Ubuntu -- `apt-get install build-essential m4 libgmp3-dev zlib1g-dev libbz2-dev`
* Arch Linux -- `pacman -S base-devel`

For building StormLib execute the following commands (line by line):	
	
	cd aura-bot/StormLib/stormlib
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

note: supplied Makefile settings produce a x64 binary (remove the -m64 from CCFLAGS and CXXFLAGS to make it produce a x86 binary).

### OS X

Most likely you'll need a more recent version of libgmp because Apple ships an older one built for i386 only
what would result in problems since we are linking x86_64.

Using [Macports](http://www.macports.org/):

	sudo port install gmp
   
When finished type `cd ` (append a space after!) in Terminal, drag&drop your `aura-bot`-folder into the Terminal 
window to get the folder's path and execute with enter.

For building StormLib execute the following commands (line by line):

	cd StormLib/stormlib_osx
	make
	sudo make install
   
Continue building bncsutil:

	cd ../..
	cd bncsutil/src/bncsutil
	make
	sudo make install
   
Go on building aura:

	cd ../../..
	cd aura
	make && make install
	cd ..
   
Now you can run aura by executing `./aura++` in Terminal.

note: supplied Makefile settings produce a x64 binary which is the only one supported.

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
