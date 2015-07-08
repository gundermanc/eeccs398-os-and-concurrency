#!/bin/bash

echo "RPC Mailbox Client 2"
echo "(C) 2015 Christian Gunderman"
echo

if [ $# -eq 0 ]; then
    echo "Usage: ./client2.sh [server_hostname]"
    exit
fi

# Get the machine hostname.
C_HOST=$1
C_NAME=$(dnsdomainname --long)

# Display commands run on screen.
set -x

# My messages commands.
./client.sh "$C_HOST" "START" "$C_NAME"
./client.sh "$C_HOST" "INSERT_MESSAGE" "$C_NAME" "0" "Zero"
./client.sh "$C_HOST" "INSERT_MESSAGE" "$C_NAME" "1" "One"
./client.sh "$C_HOST" "INSERT_MESSAGE" "$C_NAME" "2" "Two"
./client.sh "$C_HOST" "INSERT_MESSAGE" "$C_NAME" "3" "Three"
./client.sh "$C_HOST" "INSERT_MESSAGE" "$C_NAME" "4" "Four"
./client.sh "$C_HOST" "DELETE_MESSAGE" "$C_NAME" "0"
./client.sh "$C_HOST" "RETRIEVE_MESSAGE" "$C_NAME" "1"
./client.sh "$C_HOST" "INSERT_MESSAGE" "$C_NAME" "1" "One-Two"
./client.sh "$C_HOST" "RETRIEVE_MESSAGE" "$C_NAME" "1"
./client.sh "$C_HOST" "RETRIEVE_MESSAGE" "$C_NAME" "3"
./client.sh "$C_HOST" "DELETE_MESSAGE" "$C_NAME" "4"
./client.sh "$C_HOST" "LIST_ALL_MESSAGES" "$C_NAME"
./client.sh "$C_HOST" "QUIT" "$C_NAME"
echo
