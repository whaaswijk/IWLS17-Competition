/* C file

File name [mlpcircuit2mig.c]

Package [Majority Logic Package]

Synopsis [clone a logic circuit into a MIG representation]

Description [clone a logic circuit into a MIG representation]

Author [Luca Amaru]

Copyright [Copyright (c) EPFL, Integrated Systems Laboratory]

*/

#include "mlp.h"

/* function cloneLC

Synopsis [clone a logic circuit into a MIG]

Description [an additional variable const is added to drive MAJ3 nodes configured as AND/ORs]

Side effects [all nodes in the logic circuit have fanin of 2 - otherwise fails]

*/ 
MIG *cloneLC(LogicCircuit *net) {
	MIG *ret=NULL;
	unsigned int i;
	
	ret=(MIG *)malloc(sizeof(MIG));
	
	ret->Nnodes=net->Ngates;	
	ret->Nin=net->Nin;
	ret->Nout=net->Nout;
	
	ret->in=(MAJ3 **)malloc(sizeof(MAJ3 *)*ret->Nin);
	ret->innames=(char **)malloc(sizeof(char *)*ret->Nin);
	ret->out=(MAJ3 **)malloc(sizeof(MAJ3 *)*ret->Nout);
	ret->outnames=(char **)malloc(sizeof(char *)*ret->Nout);
	ret->nodes=(MAJ3 **)malloc(sizeof(MAJ3 *)*ret->Nnodes);
	
	ret->outcompl=(unsigned int *)malloc(sizeof(unsigned int)*ret->Nout);
		
	for(i=0;i<net->Nin;i++) {
		ret->innames[i]=(char *)malloc(sizeof(char)*MAXCAR);
		strcpy(ret->innames[i],net->innames[i]);
	}
		
	for(i=0;i<net->Nout;i++) {
		ret->outnames[i]=(char *)malloc(sizeof(char)*MAXCAR);
		strcpy(ret->outnames[i],net->outnames[i]);
		ret->outcompl[i]=net->outcompl[i];
	}
		
	ret->one=(MAJ3 *)malloc(sizeof(MAJ3));
	ret->one->in1=ret->one->in2=ret->one->in3=NULL;
	ret->one->outEdges=NULL;//to be updated
	ret->one->value=1;
	ret->one->fanout=0;
	ret->one->label=i;
	ret->one->aux=NULL;
	ret->one->flag=0;
	ret->one->compl1=ret->one->compl2=ret->one->compl3=0;
	ret->one->PI=1;
	
	for(i=0;i<ret->Nin;i++) {
		ret->in[i]=(MAJ3 *)malloc(sizeof(MAJ3));
		ret->in[i]->in1=ret->in[i]->in2=ret->in[i]->in3=NULL;
		ret->in[i]->outEdges=NULL;//to be updated
		ret->in[i]->value=2;
		ret->in[i]->aux=NULL;
		ret->in[i]->fanout=0;
		ret->in[i]->label=i;
		ret->in[i]->flag=0;
		ret->in[i]->compl1=ret->in[i]->compl2=ret->in[i]->compl3=0;
		ret->in[i]->PI=1;
		ret->in[i]->PO=0;
	}
		
	for(i=0;i<ret->Nnodes;i++) {
		if(net->gates[i]->Nin>2) {
			printf("Error: direct MIG cloning not possible\n");
			return NULL;
		}
		ret->nodes[i]=(MAJ3 *)malloc(sizeof(MAJ3));
		ret->nodes[i]->in1=ret->nodes[i]->in2=ret->nodes[i]->in3=NULL;
		ret->nodes[i]->outEdges=NULL;//to be updated
		ret->nodes[i]->value=2;
		ret->nodes[i]->flag=0;
		ret->nodes[i]->aux=NULL;
		ret->nodes[i]->fanout=0;
		ret->nodes[i]->label=i;
		ret->nodes[i]->PO=0;
		ret->nodes[i]->compl1=ret->nodes[i]->compl2=ret->nodes[i]->compl3=0;
		ret->nodes[i]->PI=0;
	}

	for(i=0;i<ret->Nnodes;i++) {
		
		if(net->gates[i]->in[0]->type==PI) {
			ret->nodes[i]->in1=ret->in[net->gates[i]->in[0]->label];
			ret->nodes[i]->compl1=net->gates[i]->cmpl[0];
		}
		else {
			ret->nodes[i]->in1=ret->nodes[net->gates[i]->in[0]->label];
			ret->nodes[i]->compl1=net->gates[i]->cmpl[0];
		}
		
		if(net->gates[i]->in[1]->type==PI) {
			ret->nodes[i]->in2=ret->in[net->gates[i]->in[1]->label];
			ret->nodes[i]->compl2=net->gates[i]->cmpl[1];
		}
		else {
			ret->nodes[i]->in2=ret->nodes[net->gates[i]->in[1]->label];
			ret->nodes[i]->compl2=net->gates[i]->cmpl[1];
		}
		
		ret->nodes[i]->in3=ret->one;
		if(net->gates[i]->type==AND) {//const to logic 1 as default
			ret->nodes[i]->compl3=1;
		}
		else {//OR
			ret->nodes[i]->compl3=0;
		}
	}
	
	for(i=0;i<ret->Nout;i++) {
		if(net->outone[i]==1) {
			ret->out[i]=ret->one;
		}
		else if(net->out[i]->type==PI) {
			ret->out[i]=ret->in[net->out[i]->label];
		}
		else {
			ret->out[i]=ret->nodes[net->out[i]->label];
		}
	}
	
	for(i=0;i<ret->Nnodes;i++) {//fanout allocation
		//ret->nodes[i]->outEdges=(MAJ3 **)malloc(sizeof(MAJ3 *)*ret->nodes[i]->fanout);
	}
	
	for(i=0;i<ret->Nin;i++) {//fanout allocation
		//ret->in[i]->outEdges=(MAJ3 **)malloc(sizeof(MAJ3 *)*ret->in[i]->fanout);
		ret->in[i]->PI=1;
	}
	
	ret->one->PI=1;
	
	//ret->one->outEdges=(MAJ3 **)malloc(sizeof(MAJ3 *)*ret->one->fanout);
	
	for(i=0;i<ret->Nnodes;i++) {//fanout update
		addfanout(ret->nodes[i]->in1, ret->nodes[i]);
		addfanout(ret->nodes[i]->in2, ret->nodes[i]);
		addfanout(ret->nodes[i]->in3, ret->nodes[i]);	
	}
	
	
	for(i=0;i<ret->Nout;i++) {//fanout to dont touch the outputs
		//ret->out[i]->fanout++;
		ret->out[i]->PO=1;
		//free(ret->out[i]->outEdges);
		//ret->out[i]->outEdges=(MAJ3 **)realloc(ret->out[i]->outEdges,sizeof(MAJ3 *)*ret->out[i]->fanout);
		//ret->out[i]->outEdges[ret->out[i]->fanout-1]=NULL;
	}
	

	return ret;
}/* end of cloneLC */

