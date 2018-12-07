#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

unsigned char memory[17000000];
int hm=0;

int log2(int n) {
	int r=0;
	while (n>>=1) r++;
		return r;
}

typedef struct blocks{
	//works
	struct blocks *prev, *next;
	int tag;
} blocks;

typedef struct rows{
	//works
	struct blocks *head, *tail;
	int ways;
	int frames; 
	int sets; 
} rows;



int full(rows *row){
	//works
	return(row->ways==row->frames);
}
int empty(rows *row){
	//works
	return (row->frames==0);
}

void removeLRU(rows *row){
	// removes the LRU block
	// should be working 
	if(empty(row)){
		return;
	}
	if(row->frames==1){
		row-> head=NULL;
		row-> tail=NULL;
	}
	else{
		blocks *i= row->head;
		row->head = row->head->next;
		row->head->prev= NULL; 
		i->next= NULL; 
	}
	row-> frames= row->frames-1;
}

void getVal(int tag, int address, unsigned int value_size, unsigned int* j){
	//get value from place it is stored in memory
	//works
	for (int i=0; i<value_size; i++){
		j[i]= (unsigned int)memory[address+i];
	}
}

void endOfTheLine(rows * row, int tag, int add, int value_size, unsigned int *add_value, int add_block){// 1 or 0)
	//1 to add 0 to store 
	blocks* i= (blocks*)malloc(sizeof(blocks));
	i->tag= tag;
	if (full(row)){
		removeLRU(row);
		if (row->frames==0){
			row->head=i;
			row->tail=i;
			row->tail->next=NULL;
			row->tail->prev=NULL;
		}
		else{
			i->prev= row->tail;
			row->tail->next= i;
			row->tail= i;
			row ->tail->next=NULL;
		}
	}
	else if(empty(row)){
		row->head=i;
		row->tail=i;
		row->tail->next=NULL;
		row -> tail->prev= NULL;
	}
	else{
		i-> prev=row->tail;
		row->tail->next=i;
		row->tail=i;
		row->tail->next= NULL;
	}
	row->frames++;
	if (add_block){
		for(int j=0; j<value_size; j++){
			memory[add+ j]= add_value[j];
		}
	}
}

void store(rows *row, int tag,int address, int value_size, unsigned int *add_value) {
	blocks * i= row-> head;
	hm=0;
	while (i!=NULL){
		if (i->tag==tag){
			
			if (row->frames==1){
				hm=1;
				break;
			}
			if(i== row-> head){
				hm=1;
				removeLRU(row);
				endOfTheLine(row, tag, address, value_size, add_value,1);
				break;
			}
			if(i!=row->tail){
				hm=1;
				i->prev->next=i->next;
				i->next->prev= i->prev;
				row->frames=row->frames-1;
				endOfTheLine(row, tag, address, value_size, add_value,0);
				
			}
			break;
		}
		i=i->next;
	}// fixed 
	for (int j=0; j<value_size; i++){
		memory[address+j]= add_value[j];
	}
}

void lop(rows *row, int tag, int address, int value_size, unsigned int *j){
	blocks *i= row-> head;

	while(i != NULL){
		if (i-> tag==tag){
			if (row-> frames==1){
				hm=1;
				getVal(tag, address, value_size, j);
				return;
			}
			if (i== row-> head){
				removeLRU(row);
				endOfTheLine(row, tag, address, value_size, j, 0);
				hm=1;
				getVal(tag, address, value_size, j);
				return;
			}
			if (i != row->tail){
				i-> prev-> next= i-> next;
				i-> next -> prev= i -> prev; 
				row-> frames= row-> frames-1;
				endOfTheLine(row, tag, address, value_size, j, 0);
			}
			hm=1;
			getVal(tag, address, value_size, j);
			return;
		}
		i= i-> next;
	}
	hm=0;
	endOfTheLine(row, tag, address, value_size, j, 0);
	getVal(tag, address, value_size, j);

}



int main(int argc, char* argv[]){
	//least recently used
	//machine is byte-addressed rather than word addressed
	//big endian
	//cache blocks are intially invalid 
	//cache misses satisied by main memory
	//if block hasn't been wriiten before the value in main memory is 0
	//write-through (store state of the cache) and write-non-allocate (store the content of the simulated memory)
	//memory is an array of 16M bytes

	
	int cache_size; //powers of 2
	int ass;
	int block_size;
	int set_num;
	int frame_num;
	int offset;
	int index;


	FILE * tracefile; 
	tracefile= fopen(argv[1], "r"); 
	//store/load | 24-bit address in hex | size of access bytes | value to be written if store
	cache_size=atoi(argv[2]) *1024;
	ass=atoi(argv[3]);
	block_size=atoi(argv[4]);


	set_num= cache_size/(block_size*ass);
	frame_num= cache_size/block_size;
	offset= log2(block_size);
	index= log2(set_num);

	rows data[set_num];
	rows* initialize;

	//make empty cache
	for(int i=0; i<set_num; i++){
		initialize=(rows*)malloc(sizeof(rows));
		initialize->head= initialize->tail= NULL;
		initialize-> ways=ass;
		initialize->frames=0;
		initialize->sets=i;
		data[i]=*initialize;
	}

	char action[6];
	int load;
	int address;
	int access_size;
	unsigned int write_value[8]={0,0,0,0,0,0,0,0};

	int tag;
	int set;
	int add_offset;
	unsigned int *load_val;

	while(!feof(tracefile)){
		strncpy(action, "/0",1);
		fscanf(tracefile, "%s", action);
		if (strcmp(action,"load")){
			load=1;
		}
		else{
			load=0;
		}
		fscanf(tracefile, "%x", &address);
		fscanf(tracefile, "%i", &access_size);


		tag= address>> offset;
		add_offset= address & ((1<<offset)-1);
		set= ((address>>offset) & ((1<<index)-1));



		if(load==0){ //if store 
			for (int i=0; i <access_size; i++){
				fscanf(tracefile, "%2hhx", (unsigned char*)&write_value[i]);
			}
			store(&data[set], tag, address, access_size, write_value);
			printf("%s 0x%x ", action, address); //print for store
			if (hm){
				printf("hit");
			}
			else{
				printf("miss");
			}
			printf("\n");
		}else{ // if load
			unsigned int load_val[access_size];
			lop(&data[set], tag, address, access_size,load_val);
			printf("%s 0x%x ", action, address); //print for load
			if (hm){
				printf("hit ");
			}
			else{
				printf("miss ");
			}
			for (int i=0; i < access_size; i++){
				printf("%0*x", 2, load_val);
			}

			printf("\n");
		}
	}
	fclose(tracefile);
	return 0;
}

// void cachUpdate(address *add){
// 	int i;
// 	int high;
// 	int lru;
// 	for (i=0; i<set_size ; i++){

// 	}
// }
