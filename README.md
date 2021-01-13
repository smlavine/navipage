# NAVIPAGE

```
navipage - multi-file pager for watching YouTube videos
Copyright (C) 2021 Sebastian LaVine <mail@smlavine.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see http://www.gnu.org/licenses/.

Usage: navipage [-dhrv] files...
Options:
    -d  Enable debug output.
    -h  Print this help and exit.
    -r  Infinitely recurse in directories.
    -v  Print version and exit.
```

# INSPIRATION

My original plan with [Omnavi](https://github.com/smlavine/omnavi) was to
receive an email every morning with my YouTube subscriptions. This works
reasonably well, however it is not perfect.  In particular, it is too
cumbersome to see videos that were uploaded on previous days (this requires
searching through past emails), and it is too cumbersome to save videos to
watch for later.

# USAGE

At the moment, navipage works like a basic pager. You can use the following
bindings:

Key           | Action
--------------|-------
g             | Scroll to the top of the file.
G             | Scroll to the bottom of the file.
h             | Move one buffer to the left.
j / MouseDown | Scroll down one line.
k / MouseUp   | Scroll up one line.
l             | Move one buffer to the right.
q             | Quit navipage.
r             | Redraw the buffer.
?             | Display help information (through 'man navipage')

# QUESTIONS, QUERIES, POSERS, CONCERNS, THINGS THAT HAUNT YOU IN THE NIGHT

Cool! Make an issue on the GitHub page, or email me at
*mail {at} smlavine {dot} com*.
