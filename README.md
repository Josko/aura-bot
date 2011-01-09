Aura
=============

This is a Warcraft III hosting-bot based on GHost++ by Varlock. It's a complete
overhaul with speed and efficiency in mind and packed with fewer dependencies.

Multi-platform
------------

The bot is guaranteed to run on little-endian Linux and Windows machines. Working flawless on OS X when 
considering a few dependencies (Intel Mac with at least a Core2Duo processor).

Boost
------------

You will need the [Boost](http://www.boost.org/users/download/) libraries. Specifically
boost date-time, system and filesystem.

**Windows**

Windows users can build by:
	
	... cd to the boost directory
	bootstrap
	bjam date_time filesystem system link=static threading=multi variant=release
	
Then move the library files from the bin.v2 folder to `~/aura-bot/aura/boost/lib` (create the folder
if it doesn't exist).

**Linux**

Linux users can usually fetch them from the repo:

* Arch Linux -- `pacman -S boost`
* Debian/Ubuntu -- `apt-get install libboost-dev`

**OS X**

The easiest way to go is by using [Macports](http://www.macports.org/).
After a successful install open (or reopen) Terminal.app located in /Applications/Utilities and execute
	`sudo port install boost`
Building boost will take some time.

Building
------------

**Windows**

Windows users should use VS2010 as there are the necessary .sln and .vcxproj files. Before
building, choose the Release option. The binary shall be generated in the `~\aura-bot\aura\Release` folder.
Run it from `aura-bot` folder.

**Linux**

Linux users will probably need some packages for it to build:

* Debian/Ubuntu -- `apt-get install build-essential m4 libgmp3-dev libncurses5-dev`
* Arch Linux -- `pacman -S base-devel`
	
Also install `g++` if the command above doesn't.

Then the StormLib and BNCSUtil dependencies need to be built:

	cd ~/aura-bot/bncsutil/src/bncsutil/
	make
	sudo make install

Then:

	cd aura/StormLib/stormlib/
	make
	sudo make install
	
Then proceed to building Aura:

	cd ~/aura-bot/aura/
	make
	
This will generate the binary `aura++` in the `~/aura-bot/aura/` directory, move the binary one level
up and run it from `~/aura-bot/`.

** OS X**

Most likely you'll need a more recent version of libgmp cause Apple ships an older one built for i386 only
what would result in problems since we are linking x86_64.

Using [Macports](http://www.macports.org/):
	`sudo port install gmp`
   
When finished type `cd ` (mind the space!) in Terminal and drag&drop your `aura-bot`-folder into the Terminal
to get the folder's path and execute with enter.

For building StormLib execute the following commands (line by line):
	cd StormLib/stormlib_osx/
   make
   sudo make install
   
Continue building bncsutil:
	cd ../../
   cd bncsutil/src/bncsutil
   make
   sudo make install
   
Go on building aura:
	cd ../../../
   cd aura
   make
   make install
   cd ../
   
Now you can run aura by typing './aura++'.

Configuring
-----------

Modify the `aura.cfg` file to configure the bot to your wishes.

Credits
-----

* Varlock -- the author of the GHost++ bot
* Argon- -- for suggestion and ideas
* Joakim -- (a lot of) testing


Contributing
------------

That would be lovely.

1. Fork it.
2. Create a branch (`git checkout -b my_aura`)
3. Commit your changes (`git commit -am "Fixed a crash when using GProxy++"`)
4. Push to the branch (`git push origin my_aura`)
5. Create an [Issue][1] with a link to your branch
6. Enjoy a beer and wait

[1]: https://github.com/Josko/aura-bot/issues
