#ifndef __TRANSACTION_HEADER_FILE__
#define __TRANSACTION_HEADER_FILE__

#ifdef __cplusplus
extern"C" {
#endif

// Transaction.c
int  NewTxnAllowed(void);
int  OnNewTransaction(unsigned int itemNumber);
int  ProcessTransaction(void);
void Display_Msg(void);
int  Set_Modifier(int Acc_Modifier, int MsgNum, int Winti_Mod);
int  Get_Referred(int Type);
int  GetReferralDetails(char* szAccNum, unsigned long schemeID);

#define STORED_CONFIRMATION_FILE "stored_confirm.txt"

void  CheckAndReSendConfirmation(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __TRANSACTION_HEADER_FILE__
