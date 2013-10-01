Aura
====

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
* Has a Windows 64-bit build
* Uses SQLite and a different database organization.
* Tested on OS X
* A lot of code removed, about 1 MB smaller binary on Linux
* Updated SQLite library
* Updated zlib
* Connects to and can be controlled via IRC
* Using aggressive optimizations
* No warnings with gcc or clang, cppcheck clean
* Up to 11 fakeplayers can be added.
* Uses DotA stats automagically on maps with 'DotA' in the filename
* Uses fewer boost libraries
* Single-threaded execution
* Auto spoofcheck in private games on PvPGNs
* More commands added either ingame or bnet

Multi-platform
--------------

The bot runs on little-endian Linux (32-bit and 64-bit), Windows (32-bit and 64-bit) and OS X (64-bit Intel CPU) machines.

Building
--------

### Windows

Windows users should use VS2010 as there are the necessary .sln and .vcxproj files. Before
building, choose the Win32 or x64 platform. The binary shall be generated in the `..\aura-bot\aura\Release` folder.
Run it from `aura-bot` folder.

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

### OS X

Most likely you'll need a more recent version of libgmp. Using [Homebrew](http://brew.sh/):

	brew install gmp
   
When finished type `cd ` (mind the space!) in Terminal, drag&drop your `aura-bot`-folder into the Terminal 
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
