# MCPI-sissy
<br>

<h3><u>MCPI <b>S</b>erver <b>I</b>nventory <b>S</b>ecure <b>S</b>torage, <b>Y</b>es!</u></h3>
<br>
This is a mod for MCPI-reborn, for all those who can't do their own inventory trickery and just get what they want when they want it! Or a mod that makes your MCPI multiplayer work a tiny bit more like the mickerysoft minecraft does :) Depending on your point of view ofc.<br>

The mod file "libmcpisissy.so" is meant to be installed on both client and server instance. If both server and client has the mod installed in their mods dir, the server will take care of storing the inventory data for the client, and will provide inventory data for client upon next connect. If the server appears to not have the sissy-mod installed, the sissy-mod on the client side will know and store the inventory locally instead.


When game instance is a client, your inventory will be saved by intervals of ~30 sec. only upon change, it gets saved either serverside or locally to file. This also means if you die and lose all your stuff, if you exit within the 30 sec. timeframe, you'll get your stuff back :)

When game instance is acting as server, the server will save received inventory data to file by intervals of ~60 sec. unpon changes in the data. This means you should give your clients a fair warning a minute ahead before restarting server, or their most current inventory changes may be lost!<br><br>

<h4>IMPORTANT</h4>
Alongside "libmcpisissy.so" in the mods dir, you also need the (currently) newest version of "libextrapi.so" i.e. mcpi-addons by Bigjango, because the sissy-mod is dependent on a few of it's newest features. Older versions or the currently available v1.2.3 binary of "libextrapi.so" on Bigjango's github wont work, newer versions <i>should</i> work if he don't break backwards compatibility in newer versions ofc. A working compiled "libextrapi.so" is put together with a compiled "libmcpisissy.so" in file "binaries.zip" for you anyway!<br><br>

Unzip both to ~/minecraft-pi/mods/ and start your game! :)<br><br>


<b>If you want to compile by yourself</b>, you should know I'm using an older version 
sdk than the newest, and it is ofc provided in file "sdk_2.4.8.zip", just unzip to 
~/minecraft-pi/ OR wherever you want and edit accordingly in CMakeList.txt!<br><br>

Beware that if you want to compile with a newer sdk, you need to change a lot of the code!<br><br>

<h3>Compile your own and smoke it:</h3>

sudo apt install g++-arm-linux-gnueabihf

sudo apt install gcc-arm-linux-gnueabihf

mkdir build

cd build

cmake ..

make -j$(nproc)

cp libmcpisissy.so ~/.minecraft-pi/mods


The file can also be stripped to a smaller size by doing this:

arm-linux-gnueabihf-strip libmcpisissy.so

<h2>Big thanks to Bigjango, who helped write almost all of it :D :pray: </h2>
