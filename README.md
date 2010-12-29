Aura bot
=============

This is a Warcraft III hosting-bot based on GHost++ by Varlock. It's a complete
overhaul with speed and efficiency in mind and packed with fewer dependencies.

Multi-platform
------------

The bot is guaranteed to run on little-endian Linux and Windows machines. Running
it on OS X and big-endian machines is possible with minor-to-large code modifications.

Boost
------------

You will need the [Boost](http://www.boost.org/users/download/) libraries. Specifically
boost date-time, system and filesystem.

**Windows**

Windows users can build by:
	
`... cd to the boost directory...`
`bootstrap`
`bjam date_time filesystem system link=static threading=multi variant=release`
	
Then move the library files from the bin.v2 folder to `~/aura-bot/ghost/boost/lib` (create the folder
if it doesn't exist).

**Linux**

Linux users can usually fetch them from the repo:

* Arch Linux -- `pacman -S boost`
* Debian/Ubuntu -- `apt-get install libboost-dev`

Building
------------

**Windows**

Windows users should use VS2010 as there are the necessary .sln and .vcxproj files. Before
building, choose the Release option. The binary shall be generated in the `~\aura-bot\ghost\Release` folder.
Run it from `aura-bot` folder.

**Linux**

Linux users will probably need some packages for it to build:

* Debian/Ubuntu -- `apt-get install build-essential m4 libgmp3-dev libncurses5-dev`
* Arch Linux -- `pacman -S base-devel`
	
Also install `g++` if the command above doesn't.

Then the StormLib and BNCSUtil dependencies need to be built:

	`cd ~/aura-bot/bncsutil/src/bncsutil/`
	`make`
	`sudo make install`
	
	`cd aura/StormLib/stormlib/`
	`make`
	`sudo make install`
	
Then proceed to building Aura:

	`cd ~/aura-bot/ghost/`
	`make`
	
This will generate the binary `ghost++` in the `~/aura-bot/ghost/` directory, move the binary one level
up and run it from `~/aura-bot/`.

Configuring
-----------

Modify the `default.cfg` file to configure the bot to your wishes.

Credits
-----

* Varlock -- the author of the GHost++ bot
* Argon] -- for suggestion and ideas
* Joakim -- (a lot of) testing


Contributing
------------

That would be lovely

1. Fork it.
2. Create a branch (`git checkout -b my_aura`)
3. Commit your changes (`git commit -am "Fixed a crash when using GProxy++"`)
4. Push to the branch (`git push origin my_aura`)
5. Create an [Issue][1] with a link to your branch
6. Enjoy a beer and wait

[1]: https://github.com/Josko/aura-bot/issues
