/**
 * EECS 338 Operating Systems
 * Case Western Reserve University
 * (C) 2015 Christian Gunderman
 */
#include "proto.h"

// Include my hashtable and arraylist/stack implementation.
#include "ht.h"
#include "stk.h"

// Include stdlibs.
#include <memory.h>
#include <stdlib.h>

// Initial datastructure sizes. Hashtable grows automagically to fit capacity,
// but arraylist is configured for constant size.
#define USERS_TABLE_INIT_SIZE 10
#define USERS_TABLE_EXPAN_SIZE 10
#define USERS_TABLE_LOAD_FACTOR 0.75f
#define MAILBOX_INIT_SIZE 100

// Global Variables:
// Did my best to avoid this, but I can't see any other way to maintain
// state beween RPC calls. This is a mailbox/users hashtable of stk's (arraylists)
// both written by me in a previous project.
HT *g_users_ht = NULL;

/*
 * Starts a new mailbox for the given user.
 */
MailboxResult *mailbox_start_1_svc(MailboxParams *argp, struct svc_req *rqstp) {
  static MailboxResult result = MailboxResultSuccess;

  // Create users hashtable if not exists.
  if (g_users_ht == NULL) {
    if (!(g_users_ht = ht_new(USERS_TABLE_INIT_SIZE,
                              USERS_TABLE_EXPAN_SIZE,
                              USERS_TABLE_LOAD_FACTOR))) {
      result = MailboxResultServerFailure;
      return &result;
    }
  }

  // Check if users hashtable contains given user name:
  if (ht_get(g_users_ht, argp->user, NULL)) {
    result = MailboxResultUserExists;
    return &result;
  }

  // Create user's mailbox.
  Stk *mailbox = stk_new(MAILBOX_INIT_SIZE);

  // Register the user's mailbox under their name.
  if (!ht_put_pointer(g_users_ht, argp->user, mailbox, NULL, NULL)) {
    result = MailboxResultServerFailure;
    return &result;
  }

  result = MailboxResultSuccess;
  return &result;
}

/*
 * Deletes the mailbox for the specified user.
 */
MailboxResult *mailbox_quit_1_svc(MailboxParams *argp, struct svc_req *rqstp) {
  static MailboxResult result = MailboxResultSuccess;

  // Check that users hashtable exists.
  if (g_users_ht == NULL) {
    result = MailboxResultUserNotExists;
    return &result;
  }

  bool exists;
  DSValue mailbox;

  // Try to delete mailbox, and handle any malloc/free errors.
  if (!ht_put(g_users_ht, argp->user, NULL, &mailbox, &exists)) {
    result = MailboxResultServerFailure;
    return &result;
  }

  // User Mailbox doesn't exist, return error code.
  if (!exists) {
    result = MailboxResultUserNotExists;
    return &result;
  }

  // Delete user's message list if it exists.
  if (mailbox.pointerVal != NULL) {
    Stk *mailbox_stk = (Stk*)mailbox.pointerVal;

    // Free messages.
    for (int i = 0; i < stk_depth(mailbox_stk); i++) {
      DSValue value;
      if (stk_get(mailbox_stk, &value, i) &&
          value.pointerVal != NULL) {
        free(value.pointerVal);
      }
    }
    stk_free((Stk*)mailbox.pointerVal);
  }

  result = MailboxResultSuccess;
  return &result;
}

/*
 * Adds message to specified user's mailbox in specified slot.
 */
MailboxResult *mailbox_insert_message_1_svc(MailboxParams *argp, struct svc_req *rqstp) {
  static MailboxResult result = MailboxResultSuccess;
  DSValue mailbox;

  // Check for negative message numbers.
  if (argp->mnum < 0) {
    result = MailboxResultInvalidMnum;
    return &result;
  }

  // Check that users hashtable exists and user has registered on server.
  if (g_users_ht == NULL ||
      !ht_get(g_users_ht, argp->user, &mailbox)) {
    result = MailboxResultUserNotExists;
    return &result;
  }

  Stk *mailbox_stk = (Stk*)mailbox.pointerVal;

  // Add message to mailbox.
  if (mailbox_stk == NULL) {
    result = MailboxResultUserNotExists;
    return &result;
  }

  // Free old message if one exists.
  DSValue value;
  if (stk_get(mailbox_stk, &value, argp->mnum) &&
      value.pointerVal != NULL) {
    free(value.pointerVal);
  }

  // Allocate new string for message.
  char *msg = calloc(strlen(argp->message) + 1, sizeof(char*));
  strcpy(msg, argp->message);

  // Per the document prompt, we'll store the message in the CLIENT
  // provided index, however, we could just as easily do things the
  // right way and return an ID.
  if (!stk_set_pointer(mailbox_stk, msg, argp->mnum)) {
    result = MailboxResultMailboxFull;
    return &result;
  }

  result = MailboxResultSuccess;
  return &result;
}

/*
 * Gets the message with the specified ID from the specified user's mailbox.
 */
MailboxMessageResponse *mailbox_retrieve_message_1_svc(MailboxParams *argp,
                                                       struct svc_req *rqstp) {
  static MailboxMessageResponse result;
  DSValue mailbox;

  result.message = "\0";
  result.result = MailboxResultSuccess;

  // Check for negative message numbers.
  if (argp->mnum < 0) {
    result.result = MailboxResultInvalidMnum;
    return &result;
  }

  // Check that users hashtable exists and user has registered on server.
  if (g_users_ht == NULL ||
      !ht_get(g_users_ht, argp->user, &mailbox)) {
    result.result = MailboxResultUserNotExists;
    return &result;
  }

  Stk *mailbox_stk = (Stk*)mailbox.pointerVal;

  // Add message to mailbox.
  if (mailbox.pointerVal == NULL) {
    result.result = MailboxResultUserNotExists;
    return &result;
  }

  // Get value from mailbox, handle any errors.
  DSValue wrapper;
  if (!stk_get(mailbox_stk, &wrapper, argp->mnum)) {
    result.result = MailboxResultMessageNotExists;
    return &result;
  }

  // Check message exists.
  if (wrapper.pointerVal == NULL) {
    result.result = MailboxResultMessageNotExists;
    return &result;
  }

  // Copy message into response.
  result.message = wrapper.pointerVal;

  result.result = MailboxResultSuccess;
  return &result;
}

/*
 * Lists up to 20 of the user's messages in their mailbox.
 */
MailboxMessageListResponse *mailbox_list_all_messages_1_svc(MailboxParams *argp,
                                                            struct svc_req *rqstp) {
  static MailboxMessageListResponse result;
  DSValue mailbox;

  // Initialize return values.
  result.result = MailboxResultSuccess;
  for (int i = 0; i < MAX_EMAIL; i++) {
    result.messages[i] = "\0";
  }

  // Check that users hashtable exists and user has registered on server.
  if (g_users_ht == NULL ||
      !ht_get(g_users_ht, argp->user, &mailbox)) {
    result.result = MailboxResultUserNotExists;
    return &result;
  }

  Stk *mailbox_stk = (Stk*)mailbox.pointerVal;

  // Get up to 20 emails.
  for (int i = 0, j = 0; i < stk_depth(mailbox_stk) && j < MAX_EMAIL; i++) {

    DSValue wrapper;
    if (stk_get(mailbox_stk, &wrapper, i) &&
        wrapper.pointerVal != NULL) {

      // Copy message into response.
      result.messages[j++] = wrapper.pointerVal;
    }
  }

  result.result = MailboxResultSuccess;
  return &result;
}

/*
 * Deletes the specified user's message from the specified slot.
 */
MailboxResult *mailbox_delete_message_1_svc(MailboxParams *argp, struct svc_req *rqstp) {
 static MailboxResult result = MailboxResultSuccess;
  DSValue mailbox;

  // Check for negative message numbers.
  if (argp->mnum < 0) {
    result = MailboxResultInvalidMnum;
    return &result;
  }

  // Check that users hashtable exists and user has registered on server.
  if (g_users_ht == NULL ||
      !ht_get(g_users_ht, argp->user, &mailbox)) {
    result = MailboxResultUserNotExists;
    return &result;
  }

  Stk *mailbox_stk = (Stk*)mailbox.pointerVal;

  // Remove message from mailbox.
  if (mailbox.pointerVal == NULL) {
    result = MailboxResultUserNotExists;
    return &result;
  }

  // Free old string.
  DSValue old_value;
  if (stk_get(mailbox_stk, &old_value, argp->mnum) &&
      old_value.pointerVal != NULL) {
    free(old_value.pointerVal);
  } else {
    result = MailboxResultMessageNotExists;
    return &result;
  }

  // Per the document prompt, we'll store the message in the CLIENT
  // provided index, however, we could just as easily do things the
  // right way and return an ID.
  if (!stk_set_pointer(mailbox_stk, NULL, argp->mnum)) {
    result = MailboxResultServerFailure;
    return &result;
  }

  result = MailboxResultSuccess;
  return &result;
}
