# NAVIPAGE

navipage - multi-file pager for watching YouTube videos

# SYNOPSIS

**navipage** \[**-dhrsv**\] \[_files_...\]

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
-s     | If **$NAVIPAGE_SH** is set, run it as a shell script before files are read.
-v     | See -h.

# USAGE

**navipage** is a pager, which means its primary function is to view files.

On startup, **navipage** will read all of the _files_ passed as arguments into
buffers, unless a file is a directory, in which case it will read all of the
files in that directory. It will not go into directories within that
directory unless the **-r** option is specified.

If **-s** was specified and **$NAVIPAGE_SH** is set, then that file will be ran
as a shell script before files are read. See below for an example.

If **$NAVIPAGE_DIR** is set, then all the files in that directory will be read, as if they were passed as arguments and -r was set.

**navipage**'s key bindings are simple and few, and will be familiar to
anyone who's used the popular \*NIX programs **less**(1) or **vi**(1).

Key           | Action
--------------|-------
g             | Scroll to the top of the file.
G             | Scroll to the bottom of the file.
h             | Move one buffer to the left.
H             | Move to the first buffer.
i             | Display help information (through 'man navipage')
j / MouseDown | Scroll down one line.
k / MouseUp   | Scroll up one line.
l             | Move one buffer to the right.
L             | Move to the last buffer.
q             | Quit navipage.
r             | Redraw the buffer.
!             | Execute a command with sh.

# EXAMPLES
```
#!/bin/sh

# This is an example of what your $NAVIPAGE_SH could look like.

# Compares the files stored on a server with the files stored locally. Any
# files that are found only once (and are therefore only on the server) are
# formatted into a list and copied with scp.

# On my own server, there is a user that runs omnavi on a cronjob every
# morning. A variation on this script is how I get updates on my own machine
# every day.

# Of course, you can just as well run Omnavi on your own local machine, but I
# like getting the update automatically every morning without having to wait
# the five minutes or so it takes for it to run. Also, I used to read my new
# videos every morning by an email, which is easier to send when ran on the
# server where the email is hosted.

server="user@example.notreal"
serverdir="backups"

( ssh "$server" ls -1 "$serverdir"; ls -1 $NAVIPAGE_DIR )\
	| sort | uniq -u | sed "s/^/$serverdir\//" | tr '\n' ' '\
	| sed 's/^/"/;s/$/"/;'\
	| xargs -I '{}' scp -T "$server":'{}' "$NAVIPAGE_DIR"
```
# INSPIRATION

My original plan with [**omnavi**](https://sr.ht/~smlavine/omnavi) was to
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

To discuss **navipage**, suggest features, or report bugs, use the public
mailing list at [](https://lists.sr.ht/~smlavine/navipage).

# SEE ALSO

For more information on **navipage**, see
<https://sr.ht/~smlavine/navipage>.

For information on the companion project **omnavi**, see
<https://sr.ht/~smlavine/omnavi>.
