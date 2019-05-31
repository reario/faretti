#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>

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



uint16_t interruttore(uint16_t R1, uint16_t R2, const uint16_t COIL1, uint16_t COIL2, uint8_t V1, uint8_t V2) {

  //char errmsg[100];

  uint16_t and_mask = ~((1<<COIL1)|(1<<COIL2)); 
  uint16_t or_mask =   (V1?(1<<COIL1):0) | (V2?(1<<COIL2):0);

  uint16_t final_reg = (R1 & and_mask) | (or_mask & (~and_mask));    

  printf("%u (0 in pos. 1), %u (1 in pos. 1)\n",(0<<1),(1<<1));
  printf("R1:\t");
  printbitssimple(R1);
  printf("Result:\t");
  printbitssimple(final_reg);
  
  /*
    
  uint16_t and_mask1 = ~mask_coil;
  uint16_t or_mask1 = V1 ? mask_coil : ~mask_coil;

  uint16_t and_mask2 = ~mask_coil;
  uint16_t or_mask2 = V2 ? mask_coil : ~mask_coil;

  uint16_t and_mask = and_mask1 | and_mask2; // and_mask1 & and_mask2; 
  uint16_t or_mask  = or_mask1 | or_mask2;
   
  //uint16_t final_reg = ( ((R1 & and_mask1) | (or_mask1 & (~and_mask1)) ) & and_mask2) | (or_mask2 & (~and_mask2));

  uint16_t final_reg = ((R1 & and_mask1) | (or_mask1 & (~and_mask1))) | ((R1 & and_mask2) | (or_mask2 & (~and_mask2)));
    
  */
  
  // 0000000000010010
  

  /*
  printbitssimple(R1);

  printf("totale: ");
  printbitssimple(t1);
  
  printf("Final reg: ");
  printbitssimple(final_reg);
  */

  /*****************************/
  
  // (holdingRegister[registerAddress] and andMask) or   (orMask and not andMask)

  /* int modbus_write_registers(modbus_t *ctx, int addr, int nb, const uint16_t *src); */
  /* if (modbus_mask_write_register(m,R,and_mask,or_mask) == -1) { */
  /*   sprintf(errmsg,"ERRORE nella funzione interruttore %s\n",modbus_strerror(errno)); */
  /*   logvalue(LOG_FILE,errmsg); */
  /*   return -1; */
  /* } */
    
  return 0;
}

int main() {
#define TRUE 1
#define FALSE 0
  
  interruttore(18,27,1,2,TRUE,TRUE);
  return 0;
  
}
