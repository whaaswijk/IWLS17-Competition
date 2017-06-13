/* C file

File name [mlpread.c]

Package [Majority Logic Package]

Synopsis [Read a logic circuit in various input formats]

Description [Read a logic circuit in [AIGER format], [Verilog flattened into AND/OR/INV primitives], [VHDL flattened into AND/OR/INV primitives]]

Author [Luca Amaru]

Copyright [Copyright (c) EPFL, Integrated Systems Laboratory]

*/

#include <mlp.h>

/* function getnoneofch

Synopsis [getting characters in a binary AIGER format]

Description [copy pasted from http://fmv.jku.at/aiger/]

Side effects []

*/ 
unsigned char getnoneofch(FILE *file) {
	int ch = getc (file); 
	if (ch != EOF) {
		return ch;
	}
	printf ("AIGER decode: unexpected EOF\n");
	return ch;
}/*end of getnoneofch*/

/* function decode

Synopsis [decoding the character to number convention in a binary AIGER format]

Description [copy pasted from http://fmv.jku.at/aiger/]

Side effects []

*/ 
unsigned int decode(FILE *file) {
	unsigned int x = 0, i = 0; 
	unsigned char ch;

	while ((ch = getnoneofch (file)) & 0x80) {
		x |= (ch & 0x7f) << (7 * i++);
	}

	return x | (ch << (7 * i)); 
}/*end of decode*/

/* function aigerheader

Synopsis [getting the header in AIGER format]

Description [returns 1 if successfull, 0 otherwise]

Side effects []

*/ 
unsigned int aigerheader(FILE *file, unsigned int *M, unsigned int *I, unsigned int *L, unsigned int *O, unsigned int *A) {
    char str[MAXCAR];
	int c;	
	c=fscanf(file,"%s",str);	
		
	if((c<1)||(strcmp(str,"aig")!=0)) { //less than 1 element read or not aig header: error
		printf("AIGER header not recognized\n");
		return 0;
	}
	
	c=fscanf(file,"%u %u %u %u %u", M, I, L, O, A);
	
	if(c<5) {
		return 0;//error not all parameters read
	}
	else {
		return 1;//ok to go
	}

}/*end of aigerheader*/

/* function readaiger

Synopsis [read AIGER format]

Description [returns NULL if not successfull]

Side effects []

*/ 
LogicCircuit *readaiger(FILE *file) {
	LogicCircuit *ret;
	ret=NULL;
	unsigned int M, I, L, O, A;
	unsigned int count, countaux;
	unsigned int delta0, delta1;
	unsigned int lhs, rhs0, rhs1;
	unsigned int ri0, ri1;
	unsigned int newout;
	int c;
	char nwln;
	char string[MAXCAR];

	if(aigerheader(file, &M, &I, &L, &O, &A)==0) {
		printf("AIGER parameters not set\n");
		return NULL;
	}
		
	if(L>0) {
		printf("Latches not supported: just combinational circuits\n");
		return NULL;
	}
	
	//create the unconnected logic circuit
	ret=(LogicCircuit*)malloc(sizeof(LogicCircuit));
	
	ret->Ngates=A;	
	ret->Nin=I;
	ret->Nout=O;
	
	ret->in=(LogicGate **)malloc(sizeof(LogicGate *)*ret->Nin);
	ret->innames=(char **)malloc(sizeof(char *)*ret->Nin);
	ret->out=(LogicGate **)malloc(sizeof(LogicGate *)*ret->Nout);
	ret->outnames=(char **)malloc(sizeof(char *)*ret->Nout);
	ret->gates=(LogicGate **)malloc(sizeof(LogicGate *)*ret->Ngates);
	
	ret->outcompl=(unsigned int *)malloc(sizeof(unsigned int)*ret->Nout);
	ret->outone=(unsigned int *)malloc(sizeof(unsigned int)*ret->Nout);
	
	
	for(count=0;count<I;count++) { //create inps
		ret->in[count]=(LogicGate *)malloc(sizeof(LogicGate));
		ret->in[count]->Nin=0;
		ret->in[count]->FanOut=0;
		ret->in[count]->label=count;//to be updated
		ret->in[count]->in=NULL;
		ret->in[count]->type=PI;
		ret->in[count]->cmpl=NULL;
	}
	
	for(count=0;count<A;count++) { //create gates
		ret->gates[count]=(LogicGate *)malloc(sizeof(LogicGate));
		ret->gates[count]->Nin=2;//coming from an AIG - Nin fixed to 2
		ret->gates[count]->FanOut=0;//to be updated
		ret->gates[count]->label=count;//to be updated
		ret->gates[count]->in=(LogicGate **)malloc(sizeof(LogicGate *)*ret->gates[count]->Nin);
		ret->gates[count]->type=AND;
		ret->gates[count]->cmpl=malloc(sizeof(unsigned int)*ret->gates[count]->Nin);
		for(countaux=0;countaux<ret->gates[count]->Nin;countaux++) {
			ret->gates[count]->cmpl[countaux]=0;//regular by default
		}
	}
	
	//actual elements of the outputs are already contained in the gates or inputs
				
	for(count=0;count<O;count++) { //read outputs and link them
		c=fscanf(file,"%u",&newout);
		if(c!=1) {
			printf("AIGER outputs not matching the parameters\n");
			return NULL;
		}
		if(newout==0) {//logic 0
			ret->out[count]=NULL;
			ret->outone[count]=1;
			ret->outcompl[count]=1;
		}
		else if(newout==1) {//logic 1
			ret->out[count]=NULL;
			ret->outone[count]=1;
			ret->outcompl[count]=0;
		}
		else if(newout%2==1) {//compl edge
			if(((newout-1)/2)<=I) {//input
				ret->out[count]=ret->in[(((newout-1)/2))-1];	
			}
			else {//gate
				ret->out[count]=ret->gates[((newout-1)/2-I)-1];
			}
			ret->outcompl[count]=1;
			ret->outone[count]=0;
		}
		else {
			if(((newout)/2)<=I) {//input
				ret->out[count]=ret->in[(((newout)/2))-1];	
			}
			else {//gate
				ret->out[count]=ret->gates[((newout)/2-I)-1];
			}
			ret->outcompl[count]=0;
			ret->outone[count]=0;
		}
	}
		
	nwln=getc(file);
	while(nwln!='\n') { // scroll to the beginning of the binary data
		nwln=getc(file);
	}
	
	for(count=0;count<A;count++) {
		lhs=2*(I+count+1);
		delta0=decode(file);
		delta1=decode(file);	
		rhs0=lhs-delta0;	
		rhs1=rhs0-delta1;
		//after this point the connection of the AND node with the rest of the circuit happens
		//first child
		if(rhs0%2==1) {//compl edge
			if(((rhs0-1)/2)<=I) {//input
				ri0=((rhs0-1)/2);
				ret->gates[count]->in[0]=ret->in[ri0-1];
			}
			else {//gate
				ri0=((rhs0-1)/2)-I;
				ret->gates[count]->in[0]=ret->gates[ri0-1];
			}
			ret->gates[count]->cmpl[0]=1;
		}
		else {//reg edge
			if((rhs0/2)<=I) {//input
				ri0=(rhs0/2);
				ret->gates[count]->in[0]=ret->in[ri0-1];
			}
			else {//gate
				ri0=(rhs0/2)-I;
				ret->gates[count]->in[0]=ret->gates[ri0-1];
			}
		}
		//second child
		if(rhs1%2==1) {//compl edge
			if(((rhs1-1)/2)<=I) {//input
				ri1=((rhs1-1)/2);
				ret->gates[count]->in[1]=ret->in[ri1-1];
			}
			else {//gate
				ri1=((rhs1-1)/2)-I;
				ret->gates[count]->in[1]=ret->gates[ri1-1];
			}
			ret->gates[count]->cmpl[1]=1;
		}
		else {//reg edge
			if((rhs1/2)<=I) {//input
				ri1=(rhs1/2);
				ret->gates[count]->in[1]=ret->in[ri1-1];
			}
			else {//gate
				ri1=(rhs1/2)-I;
				ret->gates[count]->in[1]=ret->gates[ri1-1];
			}
		}
	}
	
	for(count=0;count<I;count++) {//input naming
		c=fscanf(file,"%s",string);
		if(c<1) {
			printf("AIGER: error in input naming\n");
		}
		c=fscanf(file,"%s",string);
		if(c<1) {
			printf("AIGER: error in input naming\n");
		}
		ret->innames[count]=malloc(sizeof(char)*MAXCAR);
		strcpy(ret->innames[count],string);
	}
	
	for(count=0;count<O;count++) {//output naming
		c=fscanf(file,"%s",string);
		if(c<1) {
			printf("AIGER: error in output naming\n");
		}
		c=fscanf(file,"%s",string);
		if(c<1) {
			printf("AIGER: error in output naming\n");
		}
		ret->outnames[count]=malloc(sizeof(char)*MAXCAR);
		strcpy(ret->outnames[count],string);
	}
	
	printf("AIGER read, Inputs %u, Outputs %u, ANDs %u\n", I, O, A);

	return ret;
}/*end of readaiger*/

