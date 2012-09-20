# countkeys - a GNU/Linux keycounter that works!

  0. Licence
  1. Description
  2. Installation
  3. Usage how-to
  4. Further information


## 0. Licence

countkeys is dual licensed under the terms of either GNU GPLv3 or later, or
WTFPLv2 or later. It is entirely your choice! See COPYING for further
licensing information.


## 1. Description

countkeys is a linux keystroke counter. It simply (and I mean simply- it
doesn't do anything too bright as far as working out what a keyboard looks
like) keeps track of how many keys you press in a session and then lets you
know


## 2. Installation

Provided your GNU/Linux distribution doesn't include countkeys package in its
repositories, manual installation of countkeys from source is as easy as:

```bash

git clone git@github.com:richoH/countkeys.git  # to extract the countkeys archive

cd countkeys/build         # move to build directory to build there
../configure               # invoke configure from parent directory
make                       # make compiles what it needs to compile
# become superuser now     # you need root to install in system dir
make install               # installs binaries, manuals and scripts
```

That's it. If everything went through fine (as it mostly should, I think), you
can probably skip ahead to the next section now.

To ever uninstall countkeys, remove accompanying scripts and manuals, issue

```bash

 make uninstall    # in the same countkeys/build dir from before
 ```

To install the binaries in path other than /usr/local/bin, use configure with
--prefix switch, for example:

```bash
../configure --prefix=/usr
```

Along with other standard configure options, you can also use:

```bash
../configure --enable-evdev-path=PATH
```

to have countkeys look for input event devices in PATH ( $(PATH)/eventX ) instead
of preconfigured default /dev/input (/dev/input/eventX), and

```bash
../configure --enable-evdev=DEV
```

to have countkeys define static event device DEV (i.e. /dev/input/eventX) instead
of looking for it in default /dev/input path or custom evdev-path.

The input event device we are referring to, here, is the kernel event interface
echoing keyboard events. If using either of these two --enable-evdev\*
switches, make sure you provided correct PATH/DEV.

A copy of these instructions is in the accompanying INSTALL file.


## 3. Usage how-to

countkeys is simple. You can either invoke it directly or by typing full command
line.

Default log file is /var/log/countkeys.log and is not readable by others.

I suggest you first test the program manually with

```bash
countkeys --start --foreground
```

Which launches countkeys without detaching from the terminal and logs to stdout

You should see a line like

    Logging started ...2011-08-16 17:46:38+1000

At this time, press some keys and then press ^C to kill countkeys. Bear in mind
that countkeys processes events with a resolution of 1 second, so killing
immediately after keypresses will not include those keypresses in keycount's
output.

To start countkeys as a daemon, invoke without the --foreground switch, and
observe it's output in /var/log/countkeys.log If you enable the init script for
countkeys, you will thus see how many keys typed per boot.

In this mode, countkeys acts as a daemon, and you stop the running logger process with

```bash
countkeys --kill
```

For more information about countkeys log file format, countkeys keymap format, and
command line arguments, read the application manual,

```bash
man countkeys
```


## 4. Further information

Read the man page.

Report any bugs and request reasonable features on the issues list page

   https://github.com/richo/countkeys/issues

Always provide descriptively keyworded summary and description.

You are more than welcome to implement unreasonable features yourself, as well
as hack the program to your liking.

## 5. Credits

Countkeys is a fork of logkeys by Kernc <kerncece+countkeys@gmail.com>
