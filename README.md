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
* Dump / write out all memory slots, in a bit more aligned "table" & marking the currently active one.
* Stop code execution / wait - for iButton to be detected. Allows for schedulling multiple operations.
* Advanced mode which exposes a bit more control.
* Changing **currently active** memory slot manually. Also ignoring GPIO memory slot selecting in advanced mode.
* Editing memory slots - contents (7 or all 8 bytes in advanced mode) & name.
* Ability to edit name of the memory slot without modifying contents.
* Ability to clear / wipe all memory slots at once.

Hopefully, more to come!




Currently, I'm not planning on adding any more features for now... Not that I would be out of ideas :)

The code needs testing now, so, feel free to open issue, if something isn't working right.

There isn't much documentation, but there are lots of comments in the code, and Serial "UI" is pretty good,
even for those that just want to compile, upload it to arduino & use / play with it...

At least I'd say that...
