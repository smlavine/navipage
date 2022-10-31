# [navipage](https://sr.ht/~smlavine/navipage)

navipage is a multi-file pager designed for reading YouTube video lists
generated by [omnavi](https://sr.ht/~smlavine/omnavi).

See ```man ./navipage.1``` (or ```man navipage``` if navipage is already
installed) for extensive information on how to use navipage.

## Installation

To install:
```
git clone https://git.sr.ht/~smlavine/navipage
cd navipage
sudo make install
```

To uninstall:
```
sudo make uninstall
```

Feel free to modify ```config.mk``` to fit your local installation
needs.

navipage is intended to be built using GNU make. The only known
POSIX-noncompliant code is [this ifdef statement][ifdef] in
```config.mk```.

[ifdef]: https://git.sr.ht/~smlavine/navipage/tree/master/item/config.mk#L16

### Debugging

To compile with debug symbols, set ```DEBUG``` before running
```make```, like ```DEBUG=1 make```. Source code symbols will be
present when debugging with tools like ```gdb ./navipage``` or
```valgrind --leak-check=full --log-file=errors ./navipage```.

# Copyright

Copyright (C) 2021-2022 Sebastian LaVine <mail@smlavine.com>

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

Version 3 of the GNU General Public License is distributed with navipage
in the ```LICENSE``` file.

# Libraries

navipage uses the following libraries:
- [GNU readline](https://tiswww.case.edu/php/chet/readline/rltop.html)
- [rogueutil](https://github.com/sakhmatd/rogueutil)
  - rogueutil is licensed under the terms of the Apache license v2.0.
    See ```LICENSE.rogueutil``` for details.
- [err](https://sr.ht/~smlavine/err)

# Contributing

To follow the project, see <https://sr.ht/~smlavine/navipage>.

To browse the source code repository, see
<https://git.sr.ht/~smlavine/navipage>.

For development discussion and patches related to the navipage project,
see the mailing list at <https://lists.sr.ht/~smlavine/navipage-devel>.

For announcements related to the navipage project, see the mailing list
at <https://lists.sr.ht/~smlavine/navipage-announce>.
