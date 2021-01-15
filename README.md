# NAVIPAGE

navipage - multi-file pager for watching YouTube videos

# SYNOPSIS

**navipage** \[**-dhrv**\] \[_files_...\]

# DESCRIPTION

**navipage** is a multi-file pager with features to streamline watching YouTube videos which are linked in a file.

_NOTE:_ **navipage** is in an early stage of development. It may have bugs, and
the documentation may at times be outdated or inaccurate. Do not use if you
expect otherwise.

# OPTIONS

Option | Action
-------|-------
-d     | Enable debug output.
-h     | Print usage information and exit.
-r     | Infinitely recurse in directories.
-v     | Print version and exit.

# USAGE

**navipage** is a pager, which means its primary function is to view files.

On startup, **navipage** will read all of the _files_ passed as arguments into
buffers, unless a file is a directory, in which case it will read all of the
files in that directory. It will not go into directories within that
directory unless the **-r** option is specified.

If **$NAVIPAGE_SH** is set, then the file it lists will be ran as a shell
script before doing anything else.

If **$NAVIPAGE_DIR** is set, then all the files in that directory will be read, as if they were passed as arguments and -r was set.

**navipage**'s key bindings are simple and few, and will be familiar to
anyone who's used the popular \*NIX programs **less**(1) or **vi**(1).

Key           | Action
--------------|-------
g             | Scroll to the top of the file.
G             | Scroll to the bottom of the file.
h             | Move one buffer to the left.
i             | Display help information (through 'man navipage')
j / MouseDown | Scroll down one line.
k / MouseUp   | Scroll up one line.
l             | Move one buffer to the right.
q             | Quit navipage.
r             | Redraw the buffer.
!             | Execute a command with sh.

# INSPIRATION

My original plan with [**omnavi**](https://github.com/smlavine/omnavi) was to
receive an email every morning with my YouTube subscriptions. This works
reasonably well, however it is not perfect.  In particular, it is too
cumbersome to see videos that were uploaded on previous days (this requires
searching through past emails), and it is too cumbersome to save videos to
watch for later. **navipage** makes it easier to search for particular creators
or videos, and quickly look through multiple days worth of videos.

# AUTHOR

Written by [Sebastian LaVine](https://smlavine.com).

# COPYRIGHT

Copyright (C) 2021 Sebastian LaVine <mail@smlavine.com>

navipage is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

navipage is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with navipage. If not, see <https://www.gnu.org/licenses/>.

# LIBRARIES

**navipage** makes use of the following libraries:
- [GNU readline](https://tiswww.case.edu/php/chet/readline/rltop.html)
- [rogueutil](https://github.com/sakhmatd/rogueutil)

# QUESTIONS, QUERIES, POSERS, CONCERNS, THINGS THAT HAUNT YOU IN THE NIGHT

To contact me regarding **navipage**, [make an issue on the GitHub page](https://github.com/smlavine/navipage/issues).

Alternatively, email me directly at *mail@smlavine.com*.

# SEE ALSO

For more information on **navipage**, see
<https://github.com/smlavine/navipage>.

For information on the companion project **omnavi**, see
<https://github.com/smlavine/omnavi>.
