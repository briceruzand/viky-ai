/*
 *  Rules automaton for ogm_pho functions
 *  Copyright (c) 2006 Pertimm by P.Constant,G.Logerot and L.Rigouste
 *  Dev : November 2006
 *  Version 1.0
*/
#include "ogm_pho.h"







int PhoRulesRuleAdd(info)
struct og_xml_info *info;
{
struct og_ctrl_pho *ctrl_pho=info->ctrl_pho;
char word[DPcPathSize*2],B1[DPcPathSize];
int Irule,iword=0,iB1;
struct rule *rule;

IFE(Irule=AllocRule(ctrl_pho,&rule));

memcpy(rule->left,info->left,info->ileft);
rule->ileft=info->ileft;
memcpy(rule->key,info->key,info->ikey);
rule->ikey=info->ikey;
memcpy(rule->right,info->right,info->iright);
rule->iright=info->iright;
memcpy(rule->phonetic,info->phonetic,info->iphonetic);
rule->iphonetic=info->iphonetic;
rule->replace=info->replace;
rule->step=info->step;
rule->context_size=rule->ileft+rule->ikey+rule->iright;

if(rule->context_size>DPcPathSize*2) {
  OgMsg(ctrl_pho->hmsg,"",DOgMsgDestInLog,"PhoRulesRuleAdd: too big left+key+right (context_size=%d)",rule->context_size);
  ctrl_pho->RuleUsed--;
  DONE;
  }

memcpy(word+iword,rule->left,rule->ileft); iword += rule->ileft;
memcpy(word+iword,rule->key,rule->ikey); iword += rule->ikey;
memcpy(word+iword,rule->right,rule->iright); iword += rule->iright;

if (ctrl_pho->loginfo->trace & DOgPhoTraceRules) {
  IFE(OgUniToCp(iword,word,DPcPathSize,&iB1,B1,DOgCodePageANSI,0,0));
  OgMsg(ctrl_pho->hmsg,"",DOgMsgDestInLog,"PhoRulesRuleAdd: Irule = '%d', Expanding context [[%.*s]]",Irule,iB1,B1);
  }

IFE(PhoRulesRuleAddExpense(ctrl_pho,iword,word,0,Irule));

DONE;
}








int PhoRulesRuleGet(ctrl_pho,offset,step)
struct og_ctrl_pho *ctrl_pho;
int offset;
int step;
{
int retour=0,nstate0=0,nstate1=0,iout=0,i,Irule,ibuffer,iunibansi;
oindex states[DPcAutMaxBufferSize+9];
unsigned char out[DPcAutMaxBufferSize+9];
unsigned char buffer[DPcAutMaxBufferSize],*p;

p=buffer;
OggNout(step,&p);
iunibansi=p-buffer;
memcpy(p,ctrl_pho->bufferIn+offset,ctrl_pho->ibufferIn-offset);
p += ctrl_pho->ibufferIn-offset;
ibuffer=p-buffer;
IFE(retour=OgAufScanf(ctrl_pho->ha_rules,ibuffer,buffer,&iout,out,&nstate0,&nstate1,states));

if (nstate0<=0) return(0);

for(i=nstate0-1;i>iunibansi;i--) {
  buffer[i]=0; buffer[i+1]=1;

  if ((retour=OgAufScanf(ctrl_pho->ha_rules,i+2,buffer,&iout,out,&nstate0,&nstate1,states))) {
    do {
      IFE(retour);
      if(retour) {
        p=out;
        Irule=OggNin4(&p);
        IFE(PhoMatchingAdd(ctrl_pho,offset,Irule));
        }
      }
    while((retour=OgAufScann(ctrl_pho->ha_rules,&iout,out,nstate0,&nstate1,states)));
    }

  }

return(1);
}



int RulesLog(ctrl_pho,filename)
struct og_ctrl_pho *ctrl_pho;
char *filename;
{
char B1[DPcPathSize],B2[DPcPathSize];
struct rule *rule;
int i,iB1,iB2;
FILE *fd;


IFn(fd=fopen(filename,"wb")) {
  OgMsg(ctrl_pho->hmsg,"",DOgMsgDestInLog,"RulesLog: impossible to fopen '%s'",filename);
  DONE;
  }

/* code uni (2) + "STEP::Number::[[left::key::right]]::phonetic::replace\n" (45) */
fwrite("\xFE\xFF\0S\0T\0E\0P\0:\0:\0N\0u\0m\0b\0e\0r\0:\0:\0[\0[\0l\0e\0f\0t\0:\0:\0k\0e\0y\0:\0:\0r\0i\0g\0h\0t\0]\0]\0:\0:\0p\0h\0o\0n\0e\0t\0i\0c\0\n"
      ,sizeof(unsigned char),92,fd);

for(i=0;i<ctrl_pho->RuleUsed;i++) {
  rule=ctrl_pho->Rule+i;
  /* step and i */
  iB1=sprintf(B1,"%d::%d::[[",rule->step,i);
  IFE(OgCpToUni(iB1,B1,DPcPathSize,&iB2,B2,DOgCodePageANSI,0,0));
  fwrite(B2,sizeof(unsigned char),iB2,fd);
  /* left, key and right */
  fwrite(rule->left,sizeof(unsigned char),rule->ileft,fd);
  fwrite("\0:\0:",sizeof(unsigned char),4,fd);
  fwrite(rule->key,sizeof(unsigned char),rule->ikey,fd);
  fwrite("\0:\0:",sizeof(unsigned char),4,fd);
  fwrite(rule->right,sizeof(unsigned char),rule->iright,fd);
  fwrite("\0]\0]\0:\0:",sizeof(unsigned char),8,fd);
  /* phonetic */
  fwrite(rule->phonetic,sizeof(unsigned char),rule->iphonetic,fd);
  /* replace */
  iB1=sprintf(B1,"::%d\n",rule->replace);
  IFE(OgCpToUni(iB1,B1,DPcPathSize,&iB2,B2,DOgCodePageANSI,0,0));
  fwrite(B2,sizeof(unsigned char),iB2,fd);
  }

fclose(fd);

DONE;
}

