/*
 * navipage - A program to view and organize Omnavi files
 * Copyright (C) 2021 Sebastian LaVine <mail@smlavine.com>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "rogueutil.h" // Taken from <https://github.com/sakhmatd/rogueutil>.

void usage();

void usage()
{
	puts(
		"navipage - A program to view and organize Omnavi files\n"
		"Copyright (C) 2021 Sebastian LaVine <mail@smlavine.com>\n"
		"\n"
		"This program is free software: you can redistribute it and/or modify\n"
		"it under the terms of the GNU General Public License as published by\n"
		"the Free Software Foundation, either version 3 of the License, or\n"
		"(at your option) any later version.\n"
		"\n"
		"This program is distributed in the hope that it will be useful,\n"
		"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
		"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the\n"
		"GNU General Public License for more details.\n"
		"\n"
		"You should have received a copy of the GNU General Public License\n"
		"along with this program. If not, see http://www.gnu.org/licenses/.\n"
		"\n"
		"Usage: navipage [-h]\n"
		"Options:\n"
		"	-h Print this help and exit.\n"
		"For examples, see README.md or https://github.com/smlavine/navipage.\n"
	);
}

int main(int argc, char *argv[])
{
	struct flags {
		unsigned int process_options:1;
	};

	// loop through arguments
	for (int i = 1; i < argc; i++) {

		size_t len = strlen(argv[i]);
		if (argv[i][0] == '-' && len > 0 && flags.process_options) {
			// loop through options
			char *opt = argv[i];
			while (*(++opt) != '\0') {
				switch (*opt) {
				case '-': // don't processing later arguments as options
					flags.process_options = !flags.process_options;
					break;
				case 'h': // print help and exit
				default:
					usage();
					return 0;
					break;
				}
			}
		} else {
			// TODO: Depending on flags, do different things with option
			// arguments, or just regular arguments.
		}
	}

	return 0;
}
