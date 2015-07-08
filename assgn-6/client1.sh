#!/bin/bash

echo "RPC Mailbox Client 1"
echo "(C) 2015 Christian Gunderman"
echo

if [ $# -eq 0 ]; then
    echo "Usage: ./client1.sh [server_hostname]"
    exit
fi

# Get the machine hostname.
C_HOST=$1
C_NAME=$(dnsdomainname --long)

# Display commands run on screen.
set -x

# My messages commands.
./client.sh "$C_HOST" "START" "$C_NAME"
./client.sh "$C_HOST" "INSERT_MESSAGE" "$C_NAME" "0" "My first message."
./client.sh "$C_HOST" "INSERT_MESSAGE" "$C_NAME" "0" "My second, first message."
./client.sh "$C_HOST" "INSERT_MESSAGE" "$C_NAME" "1" "It's called #1 despite being second."
./client.sh "$C_HOST" "DELETE_MESSAGE" "$C_NAME" "0"
echo

# Intentionally wrong commands.
echo These commands should return error msgs:
./client.sh "$C_HOST" "DELETE_MESSAGE" "$C_NAME" "0"
./client.sh "$C_HOST" "INSERT_MESSAGE" "$C_NAME" "-1" "Can't create because it's negative."
./client.sh "$C_HOST" "RETRIEVE_MESSAGE" "$C_NAME" "0"
echo

# More correct commands.
echo These are more correct ones.
./client.sh "$C_HOST" "RETRIEVE_MESSAGE" "$C_NAME" "1"
./client.sh "$C_HOST" "INSERT_MESSAGE" "$C_NAME" "2" "Populating.."
./client.sh "$C_HOST" "INSERT_MESSAGE" "$C_NAME" "3" "..the.."
./client.sh "$C_HOST" "INSERT_MESSAGE" "$C_NAME" "4" "...list."
./client.sh "$C_HOST" "LIST_ALL_MESSAGES" "$C_NAME"

# Kill our mailbox.
./client.sh "$C_HOST" "QUIT" "$C_NAME"

# Intentionally try to list killed mailbox messages.
echo Try and list all messages again. It will fail cause we deleted user.
./client.sh "$C_HOST" "LIST_ALL_MESSAGES" "$C_NAME"

