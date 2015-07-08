/**
 * EECS 338 Operating Systems
 * Case Western Reserve University
 * (C) 2015 Christian Gunderman
 */
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <strings.h>

#include "proto.h"

// RPC doesn't allow NULL strings, so, we pass unused params as an empty string.
#define NULL_STR "\0"
// Max length of a message sent to the server.
#define MSG_MAX  80

/*
 * Error codes array.
 */
const char *kErrors[] = {
  "Success",
  "Server Failure",
  "No server response",
  "User exists",
  "User not exists",
  "Mailbox is full or message number too big",
  "Invalid message number",
  "Message not exists",
};

/*
 * Create Mailbox RPC Call.
 */
MailboxResult mailbox_start(CLIENT *clnt, char *user) {
  MailboxParams params;

  // Zero (null) and structs and values.
  memset(&params, 0, sizeof(params));

  params.user = user;
  params.message = NULL_STR;

  // Create user on the server.
  MailboxResult *result = mailbox_start_1(&params, clnt);
  if (result == (MailboxResult *) NULL) {
    clnt_perror (clnt, "Create mailbox call failed.");
    exit(1);
  }

  return *result;
}


/*
 * Delete Mailbox RPC Call.
 */
MailboxResult mailbox_quit(CLIENT *clnt, char *user) {
  MailboxParams params;

  // Zero (null) and structs and values.
  memset(&params, 0, sizeof(params));

  params.user = user;
  params.message = NULL_STR;

  // Create user on the server.
  MailboxResult *result = mailbox_quit_1(&params, clnt);
  if (result == (MailboxResult *) NULL) {
    clnt_perror (clnt, "Quit call failed.");
    exit(1);
  }

  return *result;
}

/*
 * Get message from server RPC Call.
 */
MailboxResult mailbox_retrieve_message(CLIENT *clnt, char *user, int mnum,
                                       char *message, int message_len) {
  MailboxParams params;

  // Zero (null) and structs and values.
  memset(&params, 0, sizeof(params));

  params.user = user;
  params.message = NULL_STR;
  params.mnum = mnum;

  // Create get message from the server.
  MailboxMessageResponse *result = mailbox_retrieve_message_1(&params, clnt);
  if (result == NULL) {
    clnt_perror (clnt, "Retrieve message call failed.");
    exit(1);
  }

  strncpy(message, result->message, message_len-1);

  return (*result).result;
}

/*
 * List all messages from server call.
 */
MailboxResult mailbox_list_all_messages(CLIENT *clnt, char *user,
                                        char **messages, int max_email) {
  MailboxParams params;

  // Zero (null) and structs and values.
  memset(&params, 0, sizeof(params));
  memset(messages, 0, sizeof(char*) * max_email);

  params.user = user;
  params.message = NULL_STR;
  params.mnum = 0;

  // Create get message from the server.
  MailboxMessageListResponse *result = mailbox_list_all_messages_1(&params, clnt);
  if (result == NULL) {
    clnt_perror (clnt, "List messages call failed.");
    exit(1);
  }

  // Store all of the messages in an array.
  for (int i = 0; i < MAX_EMAIL &&
         strcmp(result->messages[i], NULL_STR) != 0; i++) {
      messages[i] = result->messages[i];
  }

  return (*result).result;
}

/*
 * Create new message and insert into specified index RPC Call.
 */
MailboxResult mailbox_insert_message(CLIENT *clnt, char *user, int mnum, char *message) {
  MailboxParams params;

  // Store data.
  params.user = user;
  params.message = message;
  params.mnum = mnum;

  // Create message on the server.
  MailboxResult *result = mailbox_insert_message_1(&params, clnt);
  if (result == (MailboxResult *) NULL) {
    clnt_perror (clnt, "Insert call failed.");
    exit(1);
  }

  return *result;
}

/*
 * Delete message at specified index RPC Call.
 */
MailboxResult mailbox_delete_message(CLIENT *clnt, char *user, int mnum) {
  MailboxParams params;

  // Store data.
  params.user = user;
  params.message = NULL_STR;
  params.mnum = mnum;

  // Delete message on the server.
  MailboxResult *result = mailbox_delete_message_1(&params, clnt);
  if (result == (MailboxResult *) NULL) {
    clnt_perror (clnt, "Delete call failed.");
    exit(1);
  }

  return *result;
}

/*
 * Print application usage info.
 */
static void print_help() {
  printf("RPC Mailbox Client\n");
  printf("(C) 2015 Christian Gunderman\n\n");
  printf("usage: ./client [hostname] [command] ...\n");
  printf("  ./client [hostname] start [user] ...\n");
  printf("  ./client [hostname] quit [user] ...\n");
  printf("  ./client [hostname] insert_message [user] [msg_num] [msg]\n");
  printf("  ./client [hostname] retrieve_message [user] [msg_num]\n");
  printf("  ./client [hostname] list_all_messages [user]\n");
  printf("  ./client [hostname] delete_message [user] [msg_num] [msg]\n");

  // Ascii art courtesy of cowsay unix util.
  printf(" _______________________________________\n");
  printf("/ It's the Moooooo-st amazing RPC       \\\n");
  printf("\\ ever!                                 /\n");
  printf(" ---------------------------------------\n");
  printf("       \\   ^__^\n");
  printf("        \\  (oo)\_______\n");
  printf("           (__)\       )\\/\\\n");
  printf("              ||----w |\n");
  printf("              ||     ||\n");
  exit(1);
}

/*
 * Application entry point.
 */
int main (int argc, char *argv[]) {
  CLIENT *clnt = NULL;

  if (argc < 3) {
    print_help();
  }

  // Generate client.
  clnt = clnt_create (argv[1], MAILBOX_PROG, MAILBOX_VERSION, "udp");
  if (clnt == NULL) {
    clnt_pcreateerror (argv[1]);
    exit (1);
  }

  // Perform desired action.
  MailboxResult result;
  if (strcasecmp(argv[2], "START") == 0) {
    if (argc < 4) {
      print_help();
    }
    result = mailbox_start(clnt, argv[3]);
  } else if (strcasecmp(argv[2], "QUIT") == 0) {
    if (argc < 4) {
      print_help();
    }
    result = mailbox_quit(clnt, argv[3]);
  } else if (strcasecmp(argv[2], "INSERT_MESSAGE") == 0) {
    if (argc < 6) {
      print_help();
    }
    result = mailbox_insert_message(clnt, argv[3], atoi(argv[4]), argv[5]);
  } else if (strcasecmp(argv[2], "RETRIEVE_MESSAGE") == 0) {
    char message[MSG_MAX] = "";
    if (argc < 5) {
      print_help();
    }
    result = mailbox_retrieve_message(clnt, argv[3], atoi(argv[4]), message, MSG_MAX);
    printf(message);
  } else if (strcasecmp(argv[2], "DELETE_MESSAGE") == 0) {
    if (argc < 5) {
      print_help();
    }
    result = mailbox_delete_message(clnt, argv[3], atoi(argv[4]));
  } else if (strcasecmp(argv[2], "LIST_ALL_MESSAGES") == 0) {
    if (argc < 4) {
      print_help();
    }
    char *messages[MAX_EMAIL];
    result = mailbox_list_all_messages(clnt, argv[3], messages, MAX_EMAIL);
    for (int i = 0; i < MAX_EMAIL && messages[i] != NULL; i++) {
      printf("* %s\n", messages[i]);
    }
  } else {
    print_help();
  }

  // Print status.
  if (result == MailboxResultSuccess) {
    printf("Done.\n");
  } else {
    printf("Error: %s\n", kErrors[result]);
  }

  // Free client.
  clnt_destroy(clnt);
  exit(0);
}
