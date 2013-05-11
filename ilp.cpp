#include <iostream>
#include <string>
#include <fstream>
#include <queue>
#include <vector>
#include <list>
#include <stdlib.h>
#include <algorithm>
#include <functional>
using namespace std;

enum instrState 
     {
     empty, IF, ID, IS, EX, WB 
     };

class instruction {
  protected:
	instrState instructionState;
	int opType;
	//int opState;
	signed int destReg;
	signed int srcReg1;
	signed int srcReg2;
	int srcReady1;
	int srcReady2;
	signed int srcReg1_raw;
	signed int srcReg2_raw;
	int tag;
	int timer;
	
	int if_begin, id_begin, is_begin, ex_begin, wb_begin;

  public:
  
	instruction(int a, signed int b, signed int c, signed int d, int e, int f): opType(a), destReg(b), srcReg1(c), srcReg2(d),  tag(e), instructionState(IF),
                                                     if_begin(f), id_begin(0), is_begin(0), ex_begin(0), wb_begin(0),
                                                     srcReg1_raw(c), srcReg2_raw(d), srcReady1(0), srcReady2(0) 
                                                     {
                                                                     }//cout<<"new instr: "<<e<< " state: "<< opState<<endl;}

    instrState getState(){return instructionState;}
	int getOpType() {return opType;}
	int getTag() {return tag;}
	int getTimer() {return timer;}
	int getDest() {return destReg;}
	signed int getSrc1() {return srcReg1;}
	signed int getSrc2() {return srcReg2;}
	int getSrcReady1() {return srcReady1;}
	int getSrcReady2() {return srcReady2;}
	void setSrc1(int tag_in) {srcReg1=tag_in;}
	void setSrc2(int tag_in) {srcReg2=tag_in;}
	void setSrcReady1(int in) {srcReady1=in;}
	void setSrcReady2(int in) {srcReady2=in;}
	void setState(instrState a){	instructionState= a; }
	void setTimer(){        if(opType==0) timer=0; 
	                        else if(opType==1) timer=1;
					        else if(opType==2) timer=4;
					        else cout<<"ERROR: Invalid operation type. "<<endl;
	  
	               }
	void minTimer() {timer--;}
	void setIfBegin(int in) {if_begin=in;}
	void setIdBegin(int in) {id_begin=in;}
	void setIsBegin(int in) {is_begin=in;}
	void setExBegin(int in) {ex_begin=in;}
	void setWbBegin(int in) {wb_begin=in;}
	int getIfBegin() {return if_begin;}
	int getIdBegin() {return id_begin;}
	int getIsBegin() {return is_begin;}
	int getExBegin() {return ex_begin;}
	int getWbBegin() {return wb_begin;}
	signed int getRaw1() {return srcReg1_raw;}
	signed int getRaw2() {return srcReg2_raw;}
};

class registerFile {
  protected:
    int  tag;
     int registerReady;
  public:
    registerFile():registerReady(1){}
    int getTag(){return tag;}
     int isReady() {return registerReady;}
     void setReady(int ready, int tag_in) 
	                              {
									if(ready==1 && tag==tag_in) {registerReady=1; 
                                                                }
									else if(ready==1 && tag!=tag_in) {   }
                                    else if(ready==0 && tag!=tag_in) {cout<<"ERROR: attempt to set ready 0, failed"<<endl; }
                                    else if(ready==0 && tag==tag_in) {registerReady=0;}                   
	                              }
     void setRfTag(int tag_in) {tag=tag_in;}
};


list<instruction> fifo;

list<int> dispatch_list;    //contains a list of instructions in either IF or ID state. Dispatch queue.
list<int> issue_list;       //contains a list of instructions in the IS state. Scheduling queue.
list<int> exe_list;     //contains a llist of instructons in EX state. FUs.
list<int> execution_list;
registerFile register_file[128];

int main(int argc,char* argv[])
{
    bool execution_completed(int tag);
	void wakeup_dependent_instrs(int tag_in);
	vector<int> get_issue_ready_list();
	void remove_instr_from_issue(int tag);
    void add_instr_to_execute(int tag);
    void state_transition(int tag, instrState from, instrState to);
	void free_issue_list(int tag);
    vector<int> get_dispatch_temp_list();
    void remove_instr_from_dispatch(int tag);
    void add_instr_to_issue(int tag);
    void free_dispatch_list(int tag);
	void add_instr_to_dispatch(int tag);
	void delete_in_execution_list(int tag);
	bool advanceCycle(int& cycle, ifstream& fin);
	void fakeRetire();
	void execute(int& cycle);
	void issue(int S, int N, int P, int& issueCounter, int& cycle);
	void dispatch(int N, int S, int& dispatchCounter, int& issueCounter, int& cycle);
	void fetch(int N, int& tagCounter, int& dispatchCounter, ifstream& fin, int& instructions, int& cycle, int num_instrs);
    list<instruction>::iterator fifo_find_by_tag(int tag);
    list<registerFile>::iterator rf_find_by_tag(int tag);
    
	int S, N, P;
	S=atoi(argv[1]);
	N=atoi(argv[2]);
	P=atoi(argv[3]);
	
	int num_instrs=0;
	string str;
	ifstream fin_check_line(argv[4]);
	while(getline(fin_check_line,str))
	{ num_instrs++;}
	fin_check_line.close();

	ifstream fin(argv[4]);
    int cycle, exeCounter, dispatchCounter,issueCounter, fetchCounter, tagCounter, instructions;
	tagCounter=0;
	issueCounter=0;
	dispatchCounter=0;
	//fetchCounter=0;
	cycle=0;
	instructions=0;
	exe_list.clear();
	dispatch_list.clear();
	issue_list.clear();
	
	do{
      //cout<<"==== "<<cycle<<"th iteration begin ==== "<<endl;             
     fakeRetire();
     execute(cycle);
	 issue(S, N, P, issueCounter, cycle );
	 dispatch(N, S, dispatchCounter, issueCounter, cycle);
	 
	 fetch(N,tagCounter,dispatchCounter, fin, instructions, cycle, num_instrs);

	}while(advanceCycle(cycle, fin));
	
	fin.close();
	cout<<"number of instructions = "<<instructions<<endl;
	cout<<"number of cycles = "<<(cycle-1)<<endl;
	if(cycle>=1) cout<<"IPC = "<<(double)((double)instructions/(cycle-1))<<endl;
	else cout<<"ERROR: invalid cycle."<<endl;
}

list<instruction>::iterator fifo_find_by_tag(int tag_in)
{ 
  for(list<instruction>::iterator iter=fifo.begin();iter!=fifo.end();iter++)
  {
   if(iter->getTag()==tag_in) {return iter; break;} 
   //else {return NULL;}
  }
                            
                            
}//find in fifo queue with given tag, return iterator

bool execution_completed(int tag){
	list<instruction>::iterator instr_in_exe=fifo_find_by_tag(tag);
	if(instr_in_exe->getTimer()==0) {;return true;}
	else {instr_in_exe->minTimer(); return false;}
}

void wakeup_dependent_instrs(int tag_in) 
{
	list<instruction>::iterator iter_fifo=fifo.begin();
	while(iter_fifo!=fifo.end()){
	    if(iter_fifo->getSrc1()==tag_in)
		{  
             iter_fifo->setSrcReady1(1);
		}
		if(iter_fifo->getSrc2()==tag_in)
		{
            iter_fifo->setSrcReady2(1);
		}
        ++iter_fifo;
	}
}
 
vector<int> get_issue_ready_list()  // construct a temp list of instructions whose operancds are ready and reorder the list
{   vector<int> issue_temp;
	list<int>::iterator iter_issue=issue_list.begin();
	while(iter_issue!=issue_list.end()){
		list<instruction>::iterator	instr_issue=fifo_find_by_tag(*iter_issue);
		if(instr_issue->getSrcReady1()&& instr_issue->getSrcReady2()) issue_temp.push_back(*iter_issue);
	++iter_issue;
	}
    sort(issue_temp.begin(),issue_temp.end());
    return issue_temp;
}
void remove_instr_from_issue(int tag) 
{
	for(list<int>::iterator iter_issue_de=issue_list.begin();iter_issue_de!=issue_list.end();iter_issue_de++)
    {
 	 if(*iter_issue_de==tag) {issue_list.erase(iter_issue_de);break;}
	}


}
void add_instr_to_execute(int tag) 
{
	execution_list.push_back(tag);

}
void state_transition(int tag, instrState from, instrState to){
		   list<instruction>::iterator iter_fifo=fifo_find_by_tag(tag);
		   if (iter_fifo->getState()==from) iter_fifo->setState(to);	
}


vector<int> get_dispatch_temp_list()//construct a temp list of instructions in the ID state and reorder the list
{   vector<int> dispatch_temp;
	list<int>::iterator iter=dispatch_list.begin();
    while(iter!=dispatch_list.end()){
		list<instruction>::iterator	instr=fifo_find_by_tag(*iter);
		if(instr->getState()==ID) dispatch_temp.push_back(*iter);
		iter++;
	}
   sort(dispatch_temp.begin(),dispatch_temp.end());
   	return dispatch_temp;
  
}

void remove_instr_from_dispatch(int tag) 
{
 	for(list<int>::iterator iter_dis_dele=dispatch_list.begin();iter_dis_dele!=dispatch_list.end();iter_dis_dele++)
    {
 	 if(*iter_dis_dele==tag) {dispatch_list.erase(iter_dis_dele);break;}
	} 
}
void add_instr_to_issue(int tag) 
{
	issue_list.push_back(tag);
}


void add_instr_to_dispatch(int tag)
{
  dispatch_list.push_back(tag);
}

void delete_in_execution_list(int tag)
{ 
  for(list<int>::iterator ite_dele=execution_list.begin(); ite_dele!=execution_list.end();ite_dele++)
  { 
    if(*ite_dele==tag) { execution_list.erase(ite_dele); break;}
  }   
}

void fakeRetire(){
 
  
  while(fifo.front().getState()==WB)
      {
          int op_type=fifo.front().getOpType();
          int latency;
          int if_dur=fifo.front().getIdBegin()- fifo.front().getIfBegin();
          int id_dur=fifo.front().getIsBegin()-fifo.front().getIdBegin();
          int is_dur=fifo.front().getExBegin()-fifo.front().getIsBegin();
          latency=fifo.front().getWbBegin()-fifo.front().getExBegin();
		  cout<<fifo.front().getTag()<<" fu{"<<op_type<<"} src{"<<fifo.front().getRaw1()<<","<<fifo.front().getRaw2()
		  <<"} dst{"<<fifo.front().getDest()<<"} IF{"<<fifo.front().getIfBegin()<<","<<if_dur<<"} ID{"<<fifo.front().getIdBegin()<<","<<id_dur<<"} IS{"
		  <<fifo.front().getIsBegin()<<","<<is_dur<<"} EX{"<<fifo.front().getExBegin()<<","<<latency<<"} WB{"<<fifo.front().getWbBegin()
		  <<",1}"<<endl;
		  
          fifo.pop_front();      
       } 
}

void execute(int& cycle){

    list<int> temp_remove;

	for(list<int>::iterator first_exe=execution_list.begin();first_exe!=execution_list.end();first_exe++)
    {
       			int tag_exe=*first_exe;
		if(execution_completed(tag_exe)){
	         //step 1
			temp_remove.push_back(tag_exe);

            //step 2
            list<instruction>::iterator iter_fifo=fifo_find_by_tag(tag_exe);
            iter_fifo->setState(WB);
			iter_fifo->setWbBegin(cycle);
			int dest_rf=iter_fifo->getDest();
			//step 3a: update register file
			
            register_file[dest_rf].setReady(1, tag_exe);

			//step 3b: wake up dependent intructions
			wakeup_dependent_instrs(tag_exe);
		}
	}

	for(list<int>::iterator ite=temp_remove.begin();ite!=temp_remove.end();ite++)
	{
      delete_in_execution_list(*ite);                        
    }
	temp_remove.clear();
}
void issue(int S, int N, int P, int& issueCounter, int& cycle){
	//get ready list 
	vector<int> issue_ready_list=get_issue_ready_list();
	//check free FU
	int size_ready_list= issue_ready_list.size();
	int size_FU=execution_list.size();
    int lim;
   if(P==1)	
    {int free_FU;
	if(size_FU>N+1) {cout<<"ERROR: size of FU is greater than N+1. "<<endl;}
	else {free_FU= N+1-size_FU; }

	if(size_ready_list>=free_FU) lim=free_FU;
	else lim=size_ready_list;
    }
   else if(P==0)
    {if(size_ready_list<=N+1) lim=size_ready_list;
     else lim=N+1;   
    }
   else {cout<<"ERROR: invalid P. "<<endl;} 
   if(lim>0){ 
	for(int i=0; i<lim; i++)
	{  
	    int element=issue_ready_list.at(i);
 
        add_instr_to_execute(element); 
        state_transition(element, IS, EX);                     
		list<instruction>::iterator iter_fifo=fifo_find_by_tag(element);
        iter_fifo->setTimer();
        iter_fifo->setExBegin(cycle);
		--issueCounter;
		remove_instr_from_issue(element); 
	}
   }
	if(issue_list.size()!=issueCounter) cout<<"ERROR: number of instructions in issue list is invalid. "<<endl;
 
}
void dispatch(int N, int S, int& dispatchCounter, int& issueCounter, int& cycle){
   vector<int> dispatch_temp_list=get_dispatch_temp_list();
 
     bool scheduleIsFull;
   if(issueCounter>S) scheduleIsFull=true;
   else scheduleIsFull=false; 
   if(scheduleIsFull==false) {
	   int diff=S-issueCounter;
	   int dispatch_temp_list_size=dispatch_temp_list.size();
 
       int lim=diff;
 
  	   if(lim>=dispatch_temp_list_size) lim=dispatch_temp_list_size;
 
	 if(lim>0){
 	    for(int i=0; i<lim; i++){
    
           int element=dispatch_temp_list.at(i); 
		   add_instr_to_issue(element); 
		   issueCounter++; 
		   dispatchCounter--;
	 
		   state_transition(element, ID, IS);
		   fifo_find_by_tag(element)->setIsBegin(cycle);
		   
      	   remove_instr_from_dispatch(element);
     
		}
     }   
 
	   for(list<int>::iterator i=dispatch_list.begin(); i!=dispatch_list.end(); i++){
          if(fifo_find_by_tag(*i)->getState()==IF) 
            {state_transition(*i, IF, ID);
             fifo_find_by_tag(*i)->setIdBegin(cycle);
             //=====================================
           int src1_reg=fifo_find_by_tag(*i)->getSrc1();
           int src2_reg=fifo_find_by_tag(*i)->getSrc2();
 
		   int dest_reg=fifo_find_by_tag(*i)->getDest();
		
	       if(src2_reg==-1) {fifo_find_by_tag(*i)->setSrcReady2(1);}
  	     else  {
			  if(register_file[src2_reg].isReady()!=1)  {fifo_find_by_tag(*i)->setSrc2(register_file[src2_reg].getTag()); 
			                                                 fifo_find_by_tag(*i)->setSrcReady1(0);
			                                                }
			   else {fifo_find_by_tag(*i)->setSrcReady2(1);}
		      }
			       

	       if(src1_reg==-1) {fifo_find_by_tag(*i)->setSrcReady1(1);}
	       else  {
			  if(register_file[src1_reg].isReady()!=1)  {fifo_find_by_tag(*i)->setSrc1(register_file[src1_reg].getTag()); 
			                                                 fifo_find_by_tag(*i)->setSrcReady1(0);
			                                                }
			   else {fifo_find_by_tag(*i)->setSrcReady1(1);}
		      }


		  
		   
    	   register_file[dest_reg].setRfTag(*i);         
           register_file[dest_reg].setReady(0,*i);
 
            }
	   }
   }


}

void fetch(int N, int& tagCounter, int& dispatchCounter, ifstream& fin, int& instructions, int& cycle, int num_instrs){
 	int size_dispatch_list=dispatch_list.size();
	if(size_dispatch_list!=dispatchCounter) cout<<"ERROR: num of instrs in dispatch list is invalid. "<<endl;
	int num_dispatch_entry= 2*N - size_dispatch_list;
	int lima, limb;
	if(num_dispatch_entry<=N) {lima=size_dispatch_list; limb=2*N;}
	else                      {lima=0; limb=N;}
    for(int i=lima; i<limb; i++){
        if(instructions<num_instrs){
        instructions++;
        unsigned long PC;
        int op_type_in;
		signed int dest_in, src1_in, src2_in;
		fin>>hex>>PC;
        fin>>dec>>op_type_in;
        fin>>dec>>dest_in;
        fin>>dec>>src1_in;
        fin>>dec>>src2_in;
		instruction instr_in(op_type_in, dest_in, src1_in, src2_in, tagCounter, cycle);
		fifo.push_back(instr_in);
		add_instr_to_dispatch(tagCounter);
		tagCounter++;	
		dispatchCounter++;
        }
		}

}

bool advanceCycle(int& cycle, ifstream& fin){  
  cycle++;

  if(fifo.empty()&&cycle>1) {return false;}
  else {return true;}
}

