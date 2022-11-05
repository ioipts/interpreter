#include "interpreter.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Value* runstep(CodeBlock c, int index);

Statement initStatement(char type)
{ 
	 Statement s=NULL;
	 if (type == CONSTANTSTATEMENT) s = (Statement)ALLOCMEM(sizeof(struct ConstantStatementS));
	 else if (type == EXPRESSIONSTATEMENT) s = (Statement)ALLOCMEM(sizeof(struct ExpressionStatementS));
	 else if (type == VARIABLESTATEMENT) s = (Statement)ALLOCMEM(sizeof(struct VariableStatementS));
	 else if (type == COMPARESTATEMENT) s = (Statement)ALLOCMEM(sizeof(struct CompareStatementS));
	 else if (type == LOGICSTATEMENT) s = (Statement)ALLOCMEM(sizeof(struct LogicStatementS));
	 else if (type == ASSIGNSTATEMENT) s=(Statement)ALLOCMEM(sizeof(struct AssignmentStatementS));
	 else if (type == IFSTATEMENT) s=(Statement)ALLOCMEM(sizeof(struct IfStatementS));
	 else if (type == WHILESTATEMENT) s=(Statement)ALLOCMEM(sizeof(struct WhileStatementS));
	 else if (type == BINDINGSTATEMENT) s = (Statement)ALLOCMEM(sizeof(struct BindingStatementS));
	 else if (type == CALLSTATEMENT) s = (Statement)ALLOCMEM(sizeof(struct CallStatementS));
	 else if (type == RETURNSTATEMENT) s = (Statement)ALLOCMEM(sizeof(struct ReturnStatementS));
	 if (s != NULL) { s->type = type; s->next = 0;  }
	 return s;
}

void destroyProperties(Properties* p)
{
	if (p->name != NULL) FREEMEM(p->name);
	if (p->value != NULL) FREEMEM(p->value);
}

void destroyStatement(Statement s)
{
	if (s->type == BINDINGSTATEMENT) {
		BindingStatement fs = (BindingStatement)s;
		if (fs->param != NULL) FREEMEM(fs->param);
		if (fs->func != NULL) FREEMEM(fs->func);
		if (fs->varname != NULL) FREEMEM(fs->varname);
	}
	else if (s->type == VARIABLESTATEMENT) {
		VariableStatement vs = (VariableStatement)s;
		if (vs->property != NULL) {
			for (int i = 0; i < vs->numproperty; i++)
			{
				if (vs->property[i].staticproperty != NULL) FREEMEM(vs->property[i].staticproperty);
			}
			FREEMEM(vs->property);
		}
	}
	else if (s->type == CONSTANTSTATEMENT) {
		ConstantStatement cs = (ConstantStatement)s;
		for (int i = 0; i < cs->constant.numproperties; i++) {
			destroyProperties(&cs->constant.properties[i]);
		}
		if (cs->constant.properties!= NULL) FREEMEM(cs->constant.properties);
	}
	FREEMEM(s);
}

Value* initValue(const char* value)
{
	Value* v = (Value*)ALLOCMEM(sizeof(Value));
	v->numproperties = 1;
	v->properties = (Properties*)ALLOCMEM(sizeof(Properties));
	v->properties[0].name = (char*)ALLOCMEM(1);
	v->properties[0].name[0] = 0;
	v->properties[0].value = (char*)ALLOCMEM(strlen(value) + 1);
	strcpy(v->properties[0].value, value);
	return v;
}

Value* initValue(int size)
{
	Value* v = (Value*)ALLOCMEM(sizeof(Value));
	v->numproperties = 1;
	v->properties = (Properties*)ALLOCMEM(sizeof(Properties));
	v->properties[0].name = (char*)ALLOCMEM(1);
	v->properties[0].name[0] = 0;
	v->properties[0].value = (char*)ALLOCMEM(size + 1);
	v->properties[0].value[0] = 0;
	return v;
}

void destroyValue(Value* v)
{
	for (int i = 0; i < v->numproperties; i++)
	{
		destroyProperties(&v->properties[i]);
	}
	if (v->properties != NULL) FREEMEM(v->properties);
	FREEMEM(v);
}

Variable initVariable()
{
	Variable v = (Variable)ALLOCMEM(sizeof(struct ValueS));
	v->numproperties = 1;
	v->properties = (Properties*)ALLOCMEM(sizeof(Properties));
	v->properties[0].name = (char*)ALLOCMEM(1);
	v->properties[0].name[0] = 0;
	v->properties[0].value = (char*)ALLOCMEM(1);
	v->properties[0].value[0] = 0;
	return v;
}

void setVariable(Value* v, const char* propertie, Value* value)
{
	//clear first 
	int len = strlen(propertie);
	int i = 0;
	while (i < v->numproperties)
	{
		Properties* s = &v->properties[i];
		if ((len==0) || ((strncmp(s->name, propertie, len) == 0) && ((s->name[len]==0) || (s->name[len]=='.'))))
		{
			if (v->numproperties > 1) {
				destroyProperties(s);
				//FREEMEM(s);
				v->numproperties--;
				v->properties[i] = v->properties[v->numproperties];
				v->properties = (Properties*)REALLOCMEM(v->properties, sizeof(Properties) * v->numproperties);
			}
			else { //left only one 
				strcpy(v->properties[0].name, "");
				strcpy(v->properties[0].value,"");
				i = v->numproperties;
			}
		}
		else i++;
	}
	//then merge or replace
	for (int i = 0; i < value->numproperties; i++)
	{
		struct PropertiesS* s = &value->properties[i];
		int plen = strlen(propertie) + strlen(s->name)+3;
		char* p = (char*)ALLOCMEM(plen);
		sprintf(p, "%s%s%s", propertie, ((strlen(propertie)>0) && (strlen(s->name)>0))?".":"", s->name);
		int found = -1;
		for (int j = 0; j < v->numproperties; j++)
		{
			if (strcmp(v->properties[j].name, p) == 0) { found = j;  }
		}
		if (found == -1) {
			v->properties = (Properties*)REALLOCMEM(v->properties,sizeof(Properties)*(v->numproperties+1));
			v->properties[v->numproperties].name = (char*)ALLOCMEM(strlen(p)+1);
			v->properties[v->numproperties].value= (char*)ALLOCMEM(strlen(s->value) + 1);
			strcpy(v->properties[v->numproperties].name, p);
			strcpy(v->properties[v->numproperties].value, s->value);
			v->numproperties++;
		}
		else {
			FREEMEM(v->properties[found].value);
			v->properties[found].value = (char*)ALLOCMEM(strlen(s->value)+1);
			strcpy(v->properties[found].value,s->value);
		}
		FREEMEM(p);
	}
}

Value* getVariable(Value* v, const char* property)
{
	int len = strlen(property);
	Value* res = (Value*)ALLOCMEM(sizeof(Value));
	res->numproperties = 0;
	res->properties = NULL;
	for (int i = 0; i < v->numproperties; i++)
	{
		struct PropertiesS* s=&v->properties[i];
		if (strncmp(s->name, property,len) == 0)
		{
			res->properties = (Properties*)REALLOCMEM(res->properties, sizeof(Properties) * (res->numproperties + 1));
			res->properties[res->numproperties].name = (char*)ALLOCMEM(strlen(s->name) + 1);
			char* n = s->name;
			n += len;
			if (*n == '.') n++;
			strcpy(res->properties[res->numproperties].name, n);
			res->properties[res->numproperties].value = (char*)ALLOCMEM(strlen(s->value) + 1);
			strcpy(res->properties[res->numproperties].value, s->value);
			res->numproperties++;
		}
	}
	return res;
}

void variableArray(CodeBlock c, int vid)
{	//expand the array
	if (vid>=c->numvar)
	{
		c->var = (Variable*)REALLOCMEM(c->var,sizeof(Variable)*(vid+1));
		for (int i = c->numvar; i < vid + 1; i++)
		{
			c->var[i] = initVariable();
		}
		c->numvar = vid + 1;
	}
}

char* getVariableProperty(CodeBlock c,VariableStatement vs)
{	
	char* result = NULL;
	Value** vlist = (Value**)ALLOCMEM(sizeof(Value*)*vs->numproperty);
	size_t len = 0;
	for (int i = 0; i < vs->numproperty; i++)
	{
		if (vs->property[i].type == STATICVARPROPERTY) {
			len += strlen(vs->property[i].staticproperty)+1;
		}
		else {
			Value* v=runstep(c, vs->property[i].dynamicid);
			if ((v != NULL) && (v->numproperties == 1))
			{
				len += strlen(v->properties[0].value);
				vlist[i] = v;
			}
		}
	}
	result = (char*)ALLOCMEM(len+2);
	result[0] = 0;
	for (int i = 0; i < vs->numproperty; i++)
	{
		if (i!=0) strcat(result, ".");
		if (vs->property[i].type == STATICVARPROPERTY) {
			strcat(result,vs->property[i].staticproperty);
		}
		else {
			Value* v = vlist[i];
			if ((v != NULL) && (v->numproperties == 1))
			{
				strcat(result, v->properties[0].value);
			}
			destroyValue(v);
		}
	}
	FREEMEM(vlist);
	return result;
}

void destroyVariable(Variable v)
{
	for (int i = 0; i < v->numproperties; i++)
	{
		destroyProperties(&v->properties[i]);
	}
	if (v->properties!=NULL) FREEMEM(v->properties);
	FREEMEM(v);
}

CodeBlock initCodeBlock(int maxstack)
{
	CodeBlock c = (CodeBlock)ALLOCMEM(sizeof(struct CodeBlockS));
	SETMEM(c, 0, sizeof(struct CodeBlockS));
	c->maxstack = maxstack;
	c->stack = (int*)ALLOCMEM(sizeof(int) * (maxstack+1));
	c->varstack = (int*)ALLOCMEM(sizeof(int) * (maxstack+1));

	return c;
}

void destroyCodeBlock(CodeBlock c)
{
	 int i;
	 FREEMEM(c->stack);
	 FREEMEM(c->varstack);
	 for (i=1;i<c->numstatement;i++)
	 {
		destroyStatement(c->statement[i]);
	 }
	 FREEMEM(c->statement);
	 for (i = 0; i < c->numvar; i++)
	 {
		 destroyVariable(c->var[i]);
	 }
	 if (c->var!=NULL) FREEMEM(c->var);
	 if (c->sourcecode != NULL) FREEMEM(c->sourcecode);
	 FREEMEM(c); 
}

/**
* main to run each step
* 
* @return result
*/
Value* runstep(CodeBlock c,int index)
{
 Value* result=NULL;
 int i=0;
 Value* left=NULL;
 Value* right=NULL;
 char *leftp;
 double leftnum;
 double rightnum;
 int leftf,rightf; 
 ConstantStatement constant;
 AssignmentStatement assignment;
 BindingStatement func;
 IfStatement ifstatement;
 WhileStatement whilestatement;
 ExpressionStatement expression;
 LogicStatement logic;
 Statement sta;
 CompareStatement compare;
 Statement s;
 char type;
 while (index != 0) {
	 s = (Statement)c->statement[index];
	 type = s->type;
	 switch (type)
	 {
	 case ASSIGNSTATEMENT:
	 {
		 assignment = (AssignmentStatement)c->statement[index];
		 right = runstep(c, assignment->right);
		 sta = (Statement)c->statement[assignment->left];
		 if (sta->type == VARIABLESTATEMENT)
		 {
			 VariableStatement vs = (VariableStatement)sta;
			 char* property = getVariableProperty(c, vs);
			 int vid = (vs->vartype == GLOBALVAR) ? vs->vid : c->currentvarstack + vs->vid;
			 variableArray(c, vid);
			 setVariable(c->var[vid], property, right);
			 FREEMEM(property);
		 }
		 destroyValue(right);
		 index = assignment->next;
	 } break;
	 case IFSTATEMENT:
	 {
		 ifstatement = (IfStatement)c->statement[index];
		 if (ifstatement->logic == 0) {		//else
			 index = ifstatement->ifthen;
		 }
		 else {
			 left = runstep(c, ifstatement->logic);
			 if (strcmp(left->properties[0].value, "1") == 0)
			 {
				 index=ifstatement->ifthen;
			 }
			 else {
				 index=ifstatement->next;
			 }
			 FREEMEM(left);
		 }
	 } break;
	 case COMPARESTATEMENT:
	 {
		 result = initValue("0");
		 compare = (CompareStatement)c->statement[index];
		 left=runstep(c, compare->left);
		 right=runstep(c, compare->right);
		 leftf = sscanf(left->properties[0].value, "%lf", &leftnum);
		 rightf = sscanf(right->properties[0].value, "%lf", &rightnum);
		 if ((leftf > 0) && (rightf > 0))
		 {
			 switch (compare->compare[0])  
			 {
			 case '=': { strcpy(result->properties[0].value, ((float)fabs(leftnum - rightnum) < 0.0001f) ? "1" : "0"); } break;
			 case '!': { strcpy(result->properties[0].value, ((float)fabs(leftnum - rightnum) > 0.0001f) ? "1" : "0"); } break;
			 case '>': {
				 if (compare->compare[1]=='=') strcpy(result->properties[0].value, (leftnum >= rightnum) ? "1" : "0");
				 else strcpy(result->properties[0].value, (leftnum > rightnum) ? "1" : "0");
					  } break;
			 case '<': { 
				 if (compare->compare[1] == '=') strcpy(result->properties[0].value, (leftnum <= rightnum) ? "1" : "0");
				 else strcpy(result->properties[0].value, (leftnum < rightnum) ? "1" : "0");
					   } break;
			 }
		 }
		 else
		 {
			 switch (compare->compare[0])  
			 {
			 case '=': { strcpy(result->properties[0].value, (strcmp(left->properties[0].value, right->properties[0].value) == 0) ? "1" : "0"); } break;
			 case '!': { strcpy(result->properties[0].value, (strcmp(left->properties[0].value, right->properties[0].value) != 0) ? "1" : "0"); } break;
			 case '>': {
				 if (compare->compare[1]=='=') strcpy(result->properties[0].value, (strcmp(left->properties[0].value, right->properties[0].value) >= 0) ? "1" : "0");
				 else strcpy(result->properties[0].value, (strcmp(left->properties[0].value, right->properties[0].value) > 0) ? "1" : "0");
				} break;
			 case '<': { 
				 if (compare->compare[1] == '=') strcpy(result->properties[0].value, (strcmp(left->properties[0].value, right->properties[0].value) <= 0) ? "1" : "0");
				 else strcpy(result->properties[0].value, (strcmp(left->properties[0].value, right->properties[0].value) < 0) ? "1" : "0");
			    } break;
			 }
		 }
		 destroyValue(left);
		 destroyValue(right);
		 return result;
	 } break;
	 case LOGICSTATEMENT:
	 {
		 result = initValue("");
		 left = NULL;
		 right = NULL;
		 logic = (LogicStatement)c->statement[index];
		 left=runstep(c, logic->left);
		 if (logic->logic != '!') {
			 right = runstep(c, logic->right);
		 }
		 if (logic->logic == '&') 
		 {
			 if ((strcmp(left->properties[0].value, "1") == 0) && (strcmp(right->properties[0].value, "1") == 0)) strcpy(result->properties[0].value, "1");
			 else strcpy(result->properties[0].value, "0");
		 }
		 else if (logic->logic=='|')
		 {
			 if ((strcmp(left->properties[0].value, "1") == 0) || (strcmp(right->properties[0].value, "1") == 0)) strcpy(result->properties[0].value, "1");
			 else strcpy(result->properties[0].value, "0");
		 }
		 else
		 {
			 if ((strcmp(left->properties[0].value, "0") == 0) || (strcmp(left->properties[0].value,"")==0)) strcpy(result->properties[0].value, "1");
			 else strcpy(result->properties[0].value, "0");
		 }
		 if (left != NULL) destroyValue(left);
		 if (right != NULL) destroyValue(right);
		 return result;
	 } break;
	 case EXPRESSIONSTATEMENT:
	 {
		 expression = (ExpressionStatement)c->statement[index];
		 left=runstep(c, expression->left);
		 right=runstep(c, expression->right);
		 leftf = sscanf(left->properties[0].value, "%lf", &leftnum);
		 rightf = sscanf(right->properties[0].value, "%lf", &rightnum);
		 if ((leftf != 0) && (rightf != 0))
		 {
			 switch (expression->op)
			 {
			 case '+': { leftnum += rightnum; } break;
			 case '-': { leftnum -= rightnum; } break;
			 case '*': { leftnum *= rightnum; } break;
			 case '/': { leftnum /= rightnum; } break;
			 case '%': { leftnum = (double)((int)leftnum % (int)rightnum); } break;
			 case '&': { leftnum = (double)((int)leftnum & (int)rightnum); } break;
			 case '|': { leftnum = (double)((int)leftnum | (int)rightnum); } break;
			 case '^': { leftnum = (double)((int)leftnum ^ (int)rightnum); } break;
			 }
			 result = initValue(strlen(left->properties[0].value) + strlen(right->properties[0].value) + 64);
			 sprintf(result->properties[0].value, "%.4lf", leftnum);
			 char* dp = strstr(result->properties[0].value,".0000");
			 if (dp != NULL) *dp = 0;
		 }
		 else {
			 if (expression->op == '+') {
				 result = initValue(strlen(left->properties[0].value)+strlen(right->properties[0].value)+2);
				 strcpy(result->properties[0].value, left->properties[0].value);
				 strcat(result->properties[0].value, right->properties[0].value);
			 } 
			 else {
				 result = initValue(strlen(left->properties[0].value)+1);
				 strcpy(result->properties[0].value, left->properties[0].value);
			 }
		 }
		 destroyValue(left);
		 destroyValue(right);
		 return result;
	 } break;
	 case CONSTANTSTATEMENT:
	 {
		 constant = (ConstantStatement)c->statement[index];
		 result = initValue("");
		 setVariable(result,"", &constant->constant);
		 return result;
	 } break;
	 case VARIABLESTATEMENT:
	 {
		 VariableStatement var = (VariableStatement)c->statement[index];
		 int vid = (var->vartype == GLOBALVAR) ? var->vid : var->vid + c->currentvarstack;
		 variableArray(c, vid);
		 char* property = getVariableProperty(c,var);
		 Value* res=getVariable(c->var[vid],property);
		 FREEMEM(property);
		 if (res == NULL) {
			 result = initValue("");
		 }
		 else {
			 result = res;
		 }
		 return result;
	 } break;
	 case CALLSTATEMENT:
	 {
		 CallStatement call = (CallStatement)c->statement[index];
		 if (c->currentstack >= c->maxstack) {
			 //overflow
			 if (call->next != 0) {
				 index = call->next;
			 }
			 else
				 return initValue("");
		 }
		 else {
			 //save state
			 c->stack[c->currentstack] = call->next;
			 c->varstack[c->currentstack] = c->currentvarstack;
			 c->currentstack++;
			 //load variable
			 c->var = (Variable*)REALLOCMEM(c->var, sizeof(Variable) * (c->numvar + call->stack));
			 for (int i = 0; i < call->numparam; i++) {
				 c->var[c->numvar + i] = runstep(c, call->param[i]);
			 }
			 for (int i = call->numparam; i < call->stack; i++) {
				 c->var[c->numvar+i]= initVariable();
			 }
			 c->currentvarstack = c->numvar;
			 c->numvar += call->stack;
			 result = runstep(c, call->go);
			 if (call->next != 0) {
				 destroyValue(result);
				 index = call->next;
			 }
			 else
				 return result;
		 }
	 } break;
	 case RETURNSTATEMENT:
	 {
		 ReturnStatement ret = (ReturnStatement)c->statement[index];
		 //get the return first
		 result = (ret->ret != 0) ? runstep(c, ret->ret) : initValue("");
		 //load state
		 c->currentstack--;
		 c->currentvarstack = c->varstack[c->currentstack];
		 for (int i = c->currentvarstack+ret->stack; i < c->numvar; i++) {
			 destroyVariable(c->var[i]);
		 }
		 c->numvar = c->currentvarstack+ret->stack;
		 return result;
	 } break;
	 case BINDINGSTATEMENT:
	 {
		 func = (BindingStatement)c->statement[index];
		 Value** param = (Value**)ALLOCMEM(sizeof(Value*)*func->numparam);
		 for (i = 0; i < func->numparam; i++)
		 {
			 param[i]=runstep(c, func->param[i]);
		 }
		 result = initValue("");		//default
		 char* prop = NULL;
		 VariableStatement vs = NULL;
		 if (func->left > 0) {
			 vs = (VariableStatement)c->statement[func->left];
			 int vid = (vs->vartype == GLOBALVAR) ? vs->vid : vs->vid + c->currentvarstack;
			 variableArray(c, vid);
			 prop = getVariableProperty(c, vs);
		 }
		 bool ret=c->func((vs==NULL)?NULL:((vs->vartype == GLOBALVAR)?c->var[vs->vid]:c->var[vs->vid+c->currentvarstack]), func->varname, prop, func->func, param, func->numparam, c->id, result);
		 for (i = 0; i < func->numparam; i++)
		 {
			 destroyValue(param[i]);
		 }
		 FREEMEM(param);
		 if (func->next == 0) {
			 return result;
		 }
		 if (result!=NULL) destroyValue(result);
		 index = func->next;
	 } break;
	 case WHILESTATEMENT:
	 {
		 whilestatement = (WhileStatement)c->statement[index];
		 left = runstep(c, whilestatement->logic);
		 while (strcmp(left->properties[0].value, "1") == 0)
		 {
			 destroyValue(left);
			 right=runstep(c, whilestatement->dowhile);
			 if (right != NULL) destroyValue(right);
			 left=runstep(c, whilestatement->logic);
		 }
		 destroyValue(left);
		 index=whilestatement->next;
	 } break;
	 }
 }
 return NULL;
}

void runCodeBlock(CodeBlock c,Binding* func,void* id)
{
	c->id = id;
	c->func = func;
	Value* result=runstep(c, c->start);
	if (result != NULL) destroyValue(result);
}
