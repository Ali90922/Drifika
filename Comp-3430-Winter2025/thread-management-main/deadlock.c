#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

typedef struct {
  int balance;
  // int id;
  pthread_mutex_t balance_mutex;
} bank_account;

typedef struct {
  bank_account *from;
  bank_account *to;
  int amount;
} deposit_thr_args;

/**
 * Create a new account with the specified initial amount.
 *
 * Dynamically allocates and initializes a new bank account. Caller is
 * responsible for free-ing the account at the end.
 */
bank_account *create_bank_account(int initial_amount) {
  bank_account *new_account = malloc(sizeof(bank_account));

  new_account->balance = initial_amount;
  pthread_mutex_init( &new_account->balance_mutex, NULL );
  //new_account->balance_mutex = PTHREAD_MUTEX_INITIALIZER;

  return new_account;
}

/**
 * Conduct a deposit between accounts. This function takes a void*, but the
 * actual argument should be of type deposit_thr_args*.
 *
 * **This function** frees the argument, the caller should not free it, you
 * don't know when this thread is finished with it!
 */
//pthread_mutex_t deposit_lock; // FILE SCOPE LOCK, but serializes deposits (we can't
                                // even deposit between different accounts).
void *deposit(void *ptr) {
  deposit_thr_args *args = (deposit_thr_args *)ptr;


// ORDER ACCOUNTs TO BREAK CIRCULAR WAIT

/*  bank_account *from, *to;
  int amount;

  // effectively implements total ordering of locks
  if (args->from < args->to)
  {
    from = args->from;
    to = args->to;
    amount = args->amount;
  }
  else
  {
    from = args->to;
    to   = args->from;
    amount = -args->amount;
  }
*/





    //pthread_mutex_lock( &deposit_lock );

  // we're going to modify "from", lock it so nobody else tries to modify it at
  // the same time, and so that nobody changes the balance before we test it.
  pthread_mutex_lock(&(args->from->balance_mutex));

  /* Not enough balance to transfer */
  if (args->from->balance < args->amount) {
    // not enough in the from account, unlock and bail out
    pthread_mutex_unlock(&(args->from->balance_mutex));
    return NULL;
  }

  // there was enough in the from account, let's lock the to account balance
  // just before we modify that (it's an increment, we need a lock!)
  pthread_mutex_lock(&(args->to->balance_mutex));

  args->from->balance -= args->amount;
  args->to->balance += args->amount;

  pthread_mutex_unlock(&(args->from->balance_mutex));
  pthread_mutex_unlock(&(args->to->balance_mutex));

  // pthread_mutex_unlock( &deposit_lock );

  free(ptr);
  return NULL;
}

deposit_thr_args *create_transfer(bank_account *from, bank_account *to,
                                  int amount) {
  deposit_thr_args *transfer = malloc(sizeof(deposit_thr_args));

  transfer->from = from;
  transfer->to = to;
  transfer->amount = amount;

  return transfer;
}

int main(void) {

  pthread_t thr1, thr2;

  bank_account *account1 = create_bank_account(1000);
  bank_account *account2 = create_bank_account(1000);

  deposit_thr_args *transfer1 = create_transfer(account1, account2, 100);
  deposit_thr_args *transfer2 = create_transfer(account2, account1, 100);

  /* Perform the deposits */
  pthread_create(&thr1, NULL, deposit, (void *)transfer1);
  pthread_create(&thr2, NULL, deposit, (void *)transfer2);

  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  printf("Balance of account 1: $%d\n", account1->balance);
  printf("Balance of account 2: $%d\n", account2->balance);

  free(account1);
  free(account2);
  free(transfer1);
  free(transfer2);

  return 0;
}
