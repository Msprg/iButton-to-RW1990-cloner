# iButton-to-RW1990-cloner
This is the version of sketch by [**Ella Jameson,**](https://www.youtube.com/watch?v=gkU5b4hSm94) with some improvements of mine. Thanks a bunch for the video - it was the only fast, easy & reliable sketch I've found for this purpose!


Most of the added funcionallity is ability to control the program through serial console.

Recomended enviroment for building this project is using PlatformIO & IDE of your choice - ideally one that supports PlatformIO integration. I'm using VS Code.
This project should be generic enough to allow proting to other microcontrollers, with little to no changes in code.


Some of the added "features of this code" include: 
* Code has been split into more functions and commented more, so it's more readable now, 
* Reading to memory slot from iButton by serial command,
* Writing from memory slot to iButton by serial command,
* Detect / list / read - currently connected iButton & writing out the address, without saving it anywhere,
* Stop code execution / wait - for iButton to be detected. Allows for schedulling ~~multiple~~ one operation~~s~~.


Hopefully, more to come!
