/*
  faretti esterni a riga di comando: newf [-r ON|OFF] [-s ON|OFF]
  esempio: 

  newf -r ON -s OFF (accende sopra e spegne sotto)
  newf -r OFF -s OFF (spegne sopra e spegne sotto)
  newf -r ON -s ON (accende sopra e accende sotto)

  r: faretti sopra
  s: faretti sotto

*/


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

#define SOPRAON(m,reg)  (operate(  (m),(reg), ((1<<FARI_ESTERNI_SOPRA)), ((CON (FARI_ESTERNI_SOPRA)))))
#define SOPRAOFF(m,reg) (operate(  (m),(reg), ((1<<FARI_ESTERNI_SOPRA)), ((COFF(FARI_ESTERNI_SOPRA)))))

#define SOTTOON(m,reg)  (operate(  (m),(reg), ((1<<FARI_ESTERNI_SOTTO)), ((CON (FARI_ESTERNI_SOTTO)))))
#define SOTTOOFF(m,reg) (operate(  (m),(reg), ((1<<FARI_ESTERNI_SOTTO)), ((COFF(FARI_ESTERNI_SOTTO)))))

#define SOPRASOTTOON(m,reg) (operate(  (m),(reg), ( (1<<FARI_ESTERNI_SOTTO) | (1<<FARI_ESTERNI_SOPRA) ), ( CON(FARI_ESTERNI_SOTTO) | CON (FARI_ESTERNI_SOPRA))))
#define SOPRASOTTOOFF(m,reg) (operate(  (m),(reg),( (1<<FARI_ESTERNI_SOTTO) | (1<<FARI_ESTERNI_SOPRA) ), ( COFF(FARI_ESTERNI_SOTTO)| COFF(FARI_ESTERNI_SOPRA))))

#define SOTTOONSOPRAOFF(m,reg)  (operate(  (m),(reg), ((1<<FARI_ESTERNI_SOTTO)|(1<<FARI_ESTERNI_SOPRA)), ((1<<FARI_ESTERNI_SOTTO)|(0<<FARI_ESTERNI_SOPRA))))
#define SOTTOOFFSOPRAON(m,reg)  (operate(  (m),(reg), ((1<<FARI_ESTERNI_SOTTO)|(1<<FARI_ESTERNI_SOPRA)), ((0<<FARI_ESTERNI_SOTTO)|(1<<FARI_ESTERNI_SOPRA))))
#define SOTTOONSOPRAON(m,reg)  (operate(  (m),(reg), ((1<<FARI_ESTERNI_SOTTO)|(1<<FARI_ESTERNI_SOPRA)), ((1<<FARI_ESTERNI_SOTTO)|(1<<FARI_ESTERNI_SOPRA))))


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

int operate(modbus_t *m, uint16_t R, uint16_t coils, uint16_t actions) {

  char errmsg[100];
  // IMPORTANTE: ormask e and mask sono costruite tenendo conto che:
  //  - nella and_mask ci sono 0 per i bit che cambiano e 1 per quelli che non cambiano
  //    passando la diff tra quelli della interazione prima e quelli della interazione dopo
  //    la and_mask la ottengo facendo la negazione dei coils
  //  - nella or_mask c'è 1 se il bit va da 0->1 e c'è 0 se il bit va da 1->0
  //  - nella or_mask contano solo i bit che cambiano. Gli altri bit sono ininfluenti
  //  - AND_MASK: per ogni bit che cambia metto 1: (1<<BITa)|(1<<BITb)|.....(BITn) e poi faccio la negazione trovandomi 0 dove cambiano e 1 dove rimangono invariati
  //  - OR_MASK: se 0->1 (1<<BITa), se 1->0 (0<<BITb) facendo l'OR di tutti 
  uint16_t and_mask = ~coils;
  uint16_t or_mask  = actions;

  if (modbus_mask_write_register(m,R,and_mask,or_mask) == -1) {
    sprintf(errmsg,"ERRORE nella funzione interruttore %s\n",modbus_strerror(errno));
    logvalue(LOG_FILE,errmsg);
    return -1;
  }

  return 0; 

  /* printbitssimple(R); */
  /* R = (R & and_mask) | (or_mask & (~and_mask)); */
  /* printf("dopo R = "); */
  /* printbitssimple(R); */
}

int main (int argc, char *argv[]) {
  
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
  /*+++++++++++++++++++++++++++++++++++++++++++++++++*/
  int opt; 
  
  // put ':' in the starting of the 
  // string so that program can  
  //distinguish between '?' and ':'  
  while((opt = getopt(argc, argv, ":r:s:rs")) != -1)  
    {  
      switch(opt)  
        {  
	case 'r':
	  if (strncmp(optarg,"on",2)==0 || strncmp(optarg,"off",3)==0) {
	    printf("Faretti sopra: %s\n", optarg);
	    if (strncmp(optarg,"on",2)==0) SOPRAON(mb_otb, OTB_OUT);
	    if (strncmp(optarg,"off",3)==0) SOPRAOFF(mb_otb,OTB_OUT);
	  } else {
	    printf("on|off required for [%c]\n",opt);
	  }
	  break;
	case 's':
	  if (strncmp(optarg,"on",2)==0 || strncmp(optarg,"off",3)==0) {
	    printf("Faretti sotto: %s\n", optarg);
	    if (strncmp(optarg,"on",2)==0) SOTTOON(mb_otb, OTB_OUT);
	    if (strncmp(optarg,"off",3)==0) SOTTOOFF(mb_otb,OTB_OUT);
	  } else {
	    printf("on|off required for [%c]\n",opt);
	  }
	  break;
	case ':':  
	  printf("option needs a value [%c]\n",optopt);  
	  break;  
	case '?':  
	  printf("unknown option: %c\n", optopt); 
	  break;  
        }  
    }  
  
  // optind is for the extra arguments 
  // which are not parsed 
  for(; optind < argc; optind++){      
    printf("extra arguments: %s\n", argv[optind]);  
  } 
  /*+++++++++++++++++++++++++++++++++++++++++++++++++*/

  //-------------------------------------------------
  /* int opt; */
  /* // put ':' in the starting of the */
  /* // string so that program can */
  /* // distinguish between '?' and ':' */
  /* while((opt = getopt(argc, argv, ":r:s:")) != -1) */
  /*   { */
  /*     switch(opt) */
  /* 	{ */
  /* 	case 'r': */
  /* 	  printf("r: %s\n", optarg); */
  /* 	  if (strcmp(optarg,"ON") == 0) { */
  /* 	    SOPRAON(mb_otb, OTB_OUT); */
  /* 	  } else if (strcmp(optarg,"OFF") == 0) { */
  /* 	    SOPRAOFF(mb_otb, OTB_OUT); */
  /* 	  } else {printf("Argomento per r non valido\n");} */
  /* 	  break; */
  /* 	case 's': */
  /* 	  printf("s: %s\n", optarg); */
  /* 	  if (strcmp(optarg,"ON") == 0) { */
  /* 	    SOTTOON(mb_otb, OTB_OUT); */
  /* 	  } else if (strcmp(optarg,"OFF") == 0) { */
  /* 	    SOTTOOFF(mb_otb, OTB_OUT); */
  /* 	  } else {printf("Argomento per s non valido\n");} */
  /* 	  break; */
  /* 	case ':': */
  /* 	  printf("option needs a value\n"); */
  /* 	  break; */
  /* 	case '?': */
  /* 	  printf("unknown option: %c\n", optopt); */
  /* 	break; */
  /* 	} */
  /*   }   */
  //-------------------------------------------------
  
  /*
  SOPRAON(mb_otb, OTB_OUT);
  sleep(1);
  SOPRAOFF(mb_otb, OTB_OUT);

  SOTTOON(mb_otb, OTB_OUT);
  sleep(1);
  SOTTOOFF(mb_otb, OTB_OUT);
  sleep(2);
  SOPRASOTTOON(mb_otb, OTB_OUT);
  sleep(1);


  SOTTOONSOPRAOFF(mb_otb, OTB_OUT);
  sleep(2);
  SOTTOOFFSOPRAON(mb_otb, OTB_OUT);
  sleep(2);
  SOPRASOTTOOFF(mb_otb, OTB_OUT);
  sleep(2);
  SOTTOONSOPRAON(mb_otb, OTB_OUT);
  sleep(2);
  SOPRASOTTOOFF(mb_otb, OTB_OUT);

  //SOPRASOTTOOFF(mb_otb, OTB_OUT);
  */

  
  /**************************************************
  operate(mb_otb, OTB_OUT, (1<<FARI_ESTERNI_SOPRA) | (1<<FARI_ESTERNI_SOTTO), COFF(FARI_ESTERNI_SOPRA) | COFF(FARI_ESTERNI_SOTTO) );
  sleep(1);
  operate(mb_otb, OTB_OUT, (1<<FARI_ESTERNI_SOPRA) | (1<<FARI_ESTERNI_SOTTO), COFF(FARI_ESTERNI_SOPRA) | CON(FARI_ESTERNI_SOTTO) );
  sleep(1);
  operate(mb_otb, OTB_OUT, (1<<FARI_ESTERNI_SOPRA) | (1<<FARI_ESTERNI_SOTTO), CON(FARI_ESTERNI_SOPRA) | COFF(FARI_ESTERNI_SOTTO) );
  sleep(1);
  operate(mb_otb, OTB_OUT, (1<<FARI_ESTERNI_SOPRA) | (1<<FARI_ESTERNI_SOTTO), CON(FARI_ESTERNI_SOPRA) | CON(FARI_ESTERNI_SOTTO) );
  sleep(1);
  operate(mb_otb, OTB_OUT, (1<<FARI_ESTERNI_SOPRA) | (1<<FARI_ESTERNI_SOTTO), COFF(FARI_ESTERNI_SOPRA) | COFF(FARI_ESTERNI_SOTTO) );
  **************************************************/
  
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
