#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <modbus.h>
#include <time.h>

#define OTB_IP "192.168.1.11"
#define OTB_PORT 502
#define OTB_OUT 100 // registro uscite OTB
#define FARI_ESTERNI_SOPRA 0 // bit 0 del registro OUT OTB
#define FARI_ESTERNI_SOTTO 1 // bit 1 del registro OUT OTB

#define OTB_IN 0 // registro ingressi OTB
#define FARI_ESTERNI_IN_SOPRA 11 // bit 11 registro IN OTB 
#define FARI_ESTERNI_IN_SOTTO 10 // bit 10 registro IN OTB 

#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))
#define CON(what) (1<<(what))
#define COFF(what) (0<<(what))

#define LOG_FILE "/home/reario/faretti/faretti.log"

void printbitssimple(uint16_t n) {
  /*dato l'intero n stampa la rappresentazione binaria*/
  unsigned int i;
  i = 1<<(sizeof(n) * 8 - 1); /* 2^n */
  while (i > 0) {
    if (n & i)
      printf("1");
    else
      printf("0");
    i >>= 1;
  }
  printf("\n");
}

int ts(char * tst, char * fmt)
{
  time_t current_time;
  char MYTIME[50];
  struct tm *tmp ;
  /* Obtain current time. */
  current_time = time(NULL);
  if (current_time == ((time_t)-1))
    {
      return -1;
    }
  tmp = localtime(&current_time);
  // [04/15/19 11:40AM] "[%F %T]"
  if (strftime(MYTIME, sizeof(MYTIME), fmt, tmp) == 0)
    {      
      return -1;
    }
  strcpy(tst,MYTIME);
  return 0;
}

void logvalue(char *filename, char *message)
{
  /*scrive su filename il messaggio message*/
  FILE *logfile;
  char t[30];
  ts(t,"[%F %T]");
  //ts(t,"[%Y%m%d-%H%M%S]");
  logfile=fopen(filename,"a");
  if(!logfile) return;
  fprintf(logfile,"%s %s",t,message);
  fclose(logfile);
}

uint16_t interruttore (modbus_t *m, uint16_t R, const uint8_t COIL, const uint8_t V) {
  /* 
     ad ogni sua chiamata questa funzione inverte lo stato del bit COIL 
     nel registro R a seconda del valore di V: V=TRUE 0->1, V=FALSE 1>0 
  */
  /*------------------------------------------*/  
  /*---> Usa la MaskWrite FC22 del modbus <---*/
  /*------------------------------------------*/
  /* V=TRUE se transizione da 0->1, V=FALSE se transizione da 1->0 */
  /* R num registro remoto (per OTB R = registro delle uscite ad indirizzo=100 */
  /* COIL il numero del BIT del registro R da mettere a 1 o a 0 in base al valore di V */
  /* IL TWIDO NON SUPPORTA LA FC022 !!! */
  /* ---------------------------------- */
  char errmsg[100];
  uint16_t mask_coil;

  mask_coil = (1<<COIL);
  uint16_t and_mask = ~mask_coil; 
  uint16_t or_mask = V ? mask_coil : ~mask_coil;    

  // int modbus_write_registers(modbus_t *ctx, int addr, int nb, const uint16_t *src);
  if (modbus_mask_write_register(m,R,and_mask,or_mask) == -1) {
    sprintf(errmsg,"ERRORE nella funzione interruttore %s\n",modbus_strerror(errno));
    logvalue(LOG_FILE,errmsg);
    return -1;
  }
  return 0;
}

int operate(modbus_t *m, uint16_t R, uint16_t coils, uint16_t actions) {

  char errmsg[100];
  uint16_t and_mask = ~coils;
  uint16_t or_mask  = actions;

  if (modbus_mask_write_register(m,R,and_mask,or_mask) == -1) {
    sprintf(errmsg,"ERRORE nella funzione interruttore %s\n",modbus_strerror(errno));
    logvalue(LOG_FILE,errmsg);
    return -1;
  }
  return 0; 

  /* printf("prima R = "); */
  /* printbitssimple(R); */
  /* R = (R & and_mask) | (or_mask & (~and_mask)); */
  /* printf("dopo R = "); */
  /* printbitssimple(R); */
}

int main () {
  
  modbus_t *mb_otb;
  const uint32_t otb_response_timeout_sec = 2;
  const uint32_t otb_response_timeout_usec = 0;

  char errmsg[100];
  
  mb_otb = modbus_new_tcp(OTB_IP,OTB_PORT);
  modbus_set_response_timeout(mb_otb,otb_response_timeout_sec, otb_response_timeout_usec);

  if ( (modbus_connect(mb_otb) == -1)) {
    sprintf(errmsg,"ERRORE non riesco a connettermi con OTB. Premature exit [%s]\n",modbus_strerror(errno));
    logvalue(LOG_FILE,errmsg);
    exit(EXIT_FAILURE);
  }
  /**************************************************/
  
  operate(mb_otb, OTB_OUT, (1<<FARI_ESTERNI_SOPRA) | (1<<FARI_ESTERNI_SOTTO), COFF(FARI_ESTERNI_SOPRA) | COFF(FARI_ESTERNI_SOTTO) );
  sleep(1);
  operate(mb_otb, OTB_OUT, (1<<FARI_ESTERNI_SOPRA) | (1<<FARI_ESTERNI_SOTTO), COFF(FARI_ESTERNI_SOPRA) | CON(FARI_ESTERNI_SOTTO) );
  sleep(1);
  operate(mb_otb, OTB_OUT, (1<<FARI_ESTERNI_SOPRA) | (1<<FARI_ESTERNI_SOTTO), CON(FARI_ESTERNI_SOPRA) | COFF(FARI_ESTERNI_SOTTO) );
  sleep(1);
  operate(mb_otb, OTB_OUT, (1<<FARI_ESTERNI_SOPRA) | (1<<FARI_ESTERNI_SOTTO), CON(FARI_ESTERNI_SOPRA) | CON(FARI_ESTERNI_SOTTO) );
  sleep(1);
  operate(mb_otb, OTB_OUT, (1<<FARI_ESTERNI_SOPRA) | (1<<FARI_ESTERNI_SOTTO), COFF(FARI_ESTERNI_SOPRA) | COFF(FARI_ESTERNI_SOTTO) );
  
  /*
    if (interruttore(mb_otb,OTB_OUT,FARI_ESTERNI_SOPRA,TRUE) == 0) {
    logvalue(LOG_FILE,"\tFunzione interruttore avvenuta...0->1\n");
    } else {
    sprintf(errmsg,"ERRORE DI SCRITTURA:INTERRUTTORE %s\n",modbus_strerror(errno));
    logvalue(LOG_FILE,errmsg);
    }
    
    sleep(1);
    
    if (interruttore(mb_otb,OTB_OUT,FARI_ESTERNI_SOPRA,FALSE) == 0) {
    logvalue(LOG_FILE,"\tFunzione interruttore avvenuta...1->0\n");
    } else {
    sprintf(errmsg,"ERRORE DI SCRITTURA:INTERRUTTORE %s\n",modbus_strerror(errno));
    logvalue(LOG_FILE,errmsg);
    }
  */
  
  /**************************************************/

  modbus_close(mb_otb);
  modbus_free(mb_otb);
  return 0;
  
}
