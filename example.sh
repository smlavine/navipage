#!/bin/sh

# This is an example of what your $NAVIPAGE_SH could look like.
# Feel free to do with this file what you like; it is too short and unoriginal
# for me to consider it worth putting under the GPL.

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
