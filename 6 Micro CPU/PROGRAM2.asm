ORG  0000H 
  DB  00100000B;  JMP1, 06H    
  DB   00000110B             
  DB   11101010B;  HLT     
  DB  00001010B;  NOP/Addr 
     
  DB  01100000B;  JMP3, [[0BH]]         
  DB   00001011B         
  DB   00000010B;  NOP/Addr     
  DB   11100001B ;  HLT     
 
  DB  01000000B;  JMP2, [06H]         
  DB   00000110B   
  DB  11100000B;  HLT         
  DB   00000011B;  NOP/Addr     
END 