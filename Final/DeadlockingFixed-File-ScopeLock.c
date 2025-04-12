#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

// File-scope lock: this mutex is shared by every deposit
static pthread_mutex_t deposit_lock = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
  int balance;
  pthread_mutex_t balance_mutex;
} bank_account;

typedef struct {
  bank_account *from;
  bank_account *to;
  int amount;
} deposit_thr_args;

bank_account *create_bank_account(int initial_amount) {
  bank_account *new_account = malloc(sizeof(bank_account));
  new_account->balance = initial_amount;
  pthread_mutex_init(&new_account->balance_mutex, NULL);
  return new_account;
}

void *deposit(void *ptr) {
  deposit_thr_args *args = (deposit_thr_args *)ptr;

  // Acquire the file-scope lock to serialize deposit operations.
  pthread_mutex_lock(&deposit_lock);

  // Even though we still lock each account, 
  // they are now done sequentially because deposit_lock is held:
  pthread_mutex_lock(&(args->from->balance_mutex));

  // Check for sufficient funds:
  if (args->from->balance < args->amount) {
      pthread_mutex_unlock(&(args->from->balance_mutex));
      pthread_mutex_unlock(&deposit_lock);
      free(ptr);
      return NULL;
  }

  // Proceed with transfer:
  pthread_mutex_lock(&(args->to->balance_mutex));
  args->from->balance -= args->amount;
  args->to->balance   += args->amount;
  pthread_mutex_unlock(&(args->from->balance_mutex));
  pthread_mutex_unlock(&(args->to->balance_mutex));

  // Release the file-scope lock after completing the deposit.
  pthread_mutex_unlock(&deposit_lock);

  free(ptr);
  return NULL;
}

