/**
 * EECS 338 Operating Systems
 * Case Western Reserve University
 * (C) 2015 Christian Gunderman
 */

typedef string str<>;

struct MailboxParams {
  str user;
  int mnum;
  str message;
};
typedef struct MailboxParams MailboxParams;

enum MailboxResult {
  MailboxResultSuccess = 0,
  MailboxResultServerFailure = 1,
  MailboxResultNoResponse = 2,
  MailboxResultUserExists = 3,
  MailboxResultUserNotExists = 4,
  MailboxResultMailboxFull = 5,
  MailboxResultInvalidMnum = 6,
  MailboxResultMessageNotExists = 7
};

struct MailboxMessageResponse {
  MailboxResult result;
  str message;
};
typedef struct MailboxMessageResponse MailboxMessageResponse;

const MAX_EMAIL = 20;

struct MailboxMessageListResponse {
  MailboxResult result;
  str messages[MAX_EMAIL];
};
typedef struct MailboxMessageListResponse MailboxMessageListResponse;

program MAILBOX_PROG {
  version MAILBOX_VERSION {
    MailboxResult MAILBOX_START(MailboxParams) = 1;
    MailboxResult MAILBOX_QUIT(MailboxParams) = 2;
    MailboxResult MAILBOX_INSERT_MESSAGE(MailboxParams) = 3;
    MailboxMessageResponse MAILBOX_RETRIEVE_MESSAGE(MailboxParams) = 4;
    MailboxMessageListResponse MAILBOX_LIST_ALL_MESSAGES(MailboxParams) = 5;
    MailboxResult MAILBOX_DELETE_MESSAGE(MailboxParams) = 6;
  } = 1;
} = 2473650;
