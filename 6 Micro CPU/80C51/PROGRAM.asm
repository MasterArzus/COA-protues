ORG  0000H 
	DB  00100000B;  JMP1, 06H    
       DB   00000110B                                   
       DB   11101010B;  HLT     
	DB  00001010B; NOP/Addr   
                                                                      
       DB  00000000B         
	DB   00000000B                    
  	DB   00000010B;  NOP/Addr     
  	DB   11100001B;  HLT     
                                                                                                 
  	DB  01000000B;  JMP2, [06H]         
  	DB   00000110B
END
