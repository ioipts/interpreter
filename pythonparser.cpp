#include "interpreter.h"
#include "pythonparser.h"
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <string>

#define ISALPHA(S) (((S)>='a') && ((S)<='z')) || (((S)>='A') && ((S)<='Z'))
#define ISNUM(S) ((((S)>='0') && ((S)<='9')))
#define ISREALNUM(S) ((((S)>='0') && ((S)<='9')) || ((S)=='.'))
#define ISNAME(S) ((((S)>='a') && ((S)<='z')) || (((S)>='A') && ((S)<='Z')) || (((S)>='0') && ((S)<='9')))
#define ISCOMPARE(S)  ((strstr((S),"==")==(S)) || (strstr((S),"<")==(S))  || (strstr((S),">")==(S)) || (strstr((S),"<=")==(S)) || (strstr((S),">=")==(S)) || (strstr((S),"!=")==(S)))
#define ISOPERATOR(S) (((S)=='+') || ((S)=='-') || ((S)=='*') || ((S)=='/') || ((S)=='%'))
#define ISLOGIC(S) ((strstr((S),"&&")==(S)) || (strstr((S),"||")==(S))) 
#define ISSPACE(S) ((S)==' ') 
#define ISTAB(S) ((S)=='\t')
#define ISQUOTE(S) ((S)=='"')
#define ISNEWLINE(S) ((S)=='\n')


#define NUMTYPECODE 1
#define ALPHATYPECODE 2
#define STRINGTYPECODE 3
#define ARRAYTYPECODE 4
//' ' or '\t'
#define BLOCKCODE 5
#define LOGICCODE 6
//\n
#define DELIMITERCODE 7
//no more
#define ENDCODE 8
//,
#define ARRAYDELIMITERCODE 10
//+-*/
#define OPERATORTYPECODE 11
// > < 
#define COMPARETYPECODE 12
// || &&
#define LOGICTYPECODE 13
//#
#define COMMENTCODE 14

typedef struct PythonSubprocedureS PythonSubprocedure;
struct PythonSubprocedureS {
	char* name;
	int go;
	int numstack;
	int numparam;
	char** param;
};

typedef struct PythonCodeS* PythonCode;
struct PythonCodeS {
	char* sourcecode;
	bool globalOrLocal;
	PythonSubprocedure* currentSub;
	std::vector<std::string> globalVar;
	std::vector<std::string> localVar;
	std::vector<Statement> statement;
	std::vector<PythonSubprocedure*> subprocedure;
};

char* pythonCompareParser(char* statement, PythonCode code, int* result);
char* pythonLogicParser(char* statement, PythonCode code, int* result);
char* pythonExpressionParser(char* statement, PythonCode code, int* result);
char* pythonExecuteParser(char* statement, char* next, PythonCode code, int* result);
char* pythonVariableParser(char* statement,  PythonCode code, int* result);
char* pythonBindingParser(char* statement, BindingStatement func, PythonCode code, int* result);
char* pythonIfParser(char* statement, PythonCode code, int currentblock, int* resultif, int* resultlast);
char* pythonForParser(char* statement, PythonCode code, int currentblock, int* result);
char* pythonWhileParser(char* statement, PythonCode code, int currentblock, int* result);
char* pythonRoutineParser(char* statement, PythonCode code, int currentblock, int* result, int* resultfirst);

/**
* count how many lines
*/
int getLines(char* statement)
{
	int num = 0;
	int n = strlen(statement);
	if (n == 0) return 0;
	for (int i = 0; i < n; i++)
	{
		if (statement[i] == '\n') num++;
	}
	return num + 1;
}

/**
* python left trim
*/
char* trimToken(char* statement)
{
	int i = 0;
	while (statement[i] == ' ') i++;
	return &statement[i];
}

/**
* until see p or \n or 0
*/
char* nextTokenTo(char* statement, char p)
{
	while ((*statement != 0) && (*statement != '\n') && (*statement != p))
	{
		statement++;
	}
	return statement;
}

/**
* get token
* 
* @param statement
* @param type can be error (negative value)
* @param token 
* @return next statement
*/
char* pythonToken(char* statement, int* type, char** token)
{
	int i = 0, j;
	char* ptr;
	char* ptr1;
	float testnum;
	//trim
	*type = 0;
	*token = NULL;
	ptr1 = statement;
	if (ISSPACE(*statement))					//space
	{
		*type = BLOCKCODE;
		return &statement[1];
	}
	else if (ISALPHA(*statement))				//name
	{
		j = i;
		while (ISNAME(statement[i])) i++;
		char* name = (char*)ALLOCMEM(i - j + 1);
		CPYMEM(name, &statement[j], i - j);
		name[i - j] = 0;
		*token = name;
		*type = ALPHATYPECODE;
		return &statement[i];
	}
	else if (ISNUM(*statement))					//number
	{
		j = i;
		if (statement[i] == '-') i++;
		while (ISREALNUM(statement[i])) i++;
		char* num = (char*)ALLOCMEM(i - j + 1);
		CPYMEM(num, &statement[j], i - j);
		num[i - j] = 0;
		*token = num;
		if (sscanf(num, "%f", &testnum) == 0) { FREEMEM(num); *type = ERRORNUMBERIC; return ptr1; }
		*type = NUMTYPECODE;
		return &statement[i];
	}
	else if (ISOPERATOR(*statement))
	{
		char* name = (char*)ALLOCMEM(10);
		name[0] = *statement;
		name[1] = 0;
		j = i;
		i++;
		*token = name;
		*type = OPERATORTYPECODE;
		return &statement[i];

	}
	else if (*statement == '(')
	{
		*type = LOGICCODE;
		return &statement[1];
	}
	else if (*statement == '"')
	{
		statement++;
		ptr = nextTokenTo(statement, '\"');
		if (*ptr != '\"') { *type = ERRORSTRINGQUOTE; return ptr1; }
		*ptr = 0;
		char* str = (char*)ALLOCMEM(ptr - statement + 1);
		strcpy(str, statement);
		*token = str;
		*type = STRINGTYPECODE;
		return ptr + 1;
	}
	else if (*statement == '[')
	{
		*type = ARRAYTYPECODE;
		return ptr1;
	}
	else if (*statement == ',')
	{
		*type = ARRAYDELIMITERCODE;
		return ptr1;
	}
	else if ((*statement == '\n') || (*statement == '\r'))
	{
		*type = DELIMITERCODE;
		return statement + 1;
	}
	else if (*statement == '#')
	{
		*type = COMMENTCODE;
		return statement + 1;
	}
	else if (*statement == 0)
	{
		*type = ENDCODE;
		return NULL;
	}
	*type = ERRORSYNTAX;
	return statement;
}

void pythonDestroyVariableParser(std::vector<VariableProperty> property)
{
	for (int i = 0; i < property.size(); i++)
	{
		VariableProperty v = property[i];
		if (v.staticproperty != NULL) FREEMEM(v.staticproperty);
	}
}

void pythonDestroySubprocedure(PythonSubprocedure* sub)
{
	for (int i = 0; i < sub->numparam; i++)
	{
		FREEMEM(sub->param[i]);
	}
	FREEMEM(sub);
}

int pythonGetVar(PythonCode code,const char* varname,bool* isLocal)
{
	int vid = -1;
	if (!code->globalOrLocal) //find in local if not found then global
	{
		for (int i = 0; i < code->localVar.size(); i++) {
			if (strcmp(code->localVar[i].c_str(), varname) == 0) {
				vid = i;
				*isLocal = true;
				break;
			}
		}
	}
	if (vid == -1) {		//if not found then find in global
		for (int i = 0; i < code->globalVar.size(); i++) {
			if (strcmp(code->globalVar[i].c_str(), varname) == 0) {
				vid = i;
				*isLocal = false;
				break;
			}
		}
	}
	if (vid == -1) { 
		if (code->globalOrLocal) { 
			code->globalVar.push_back(varname); 
			vid = code->globalVar.size() - 1; 
		} else { 
			code->localVar.push_back(varname); 
			vid = code->localVar.size() - 1; 
			code->currentSub->numstack++;		//add stack
		} 
	}
	return vid;
}

/**
* get variable simple, array or structure 
* support a, a.b.c, a[10], a[b] 
*
* 9/5/2022
*/
char* pythonVariableParser(char* statement, PythonCode code, int* result)
{
	char* token;
	int type;
	bool res = true;
	statement = trimToken((char*)statement);
	char* ptr = statement;
	statement = pythonToken((char*)statement, &type, &token);
	if (type != ALPHATYPECODE) { FREEMEM(token); *result = ERROREXPECTVARIABLE;  return statement; }
	statement = trimToken((char*)statement);
	VariableStatement var = (VariableStatement)initStatement(VARIABLESTATEMENT);
	var->type = VARIABLESTATEMENT;
	bool isLocal=false;
	var->vid = pythonGetVar(code,token,&isLocal);
	var->vartype = (isLocal) ? LOCALVAR : GLOBALVAR;
	var->starttext = ptr - code->sourcecode;
	var->endtext = statement - code->sourcecode;
	*result = code->statement.size();
	code->statement.push_back((Statement)var);
	bool end = false;
	std::vector<VariableProperty> properties;
	while (!end) {			//loop through 
		VariableProperty p;
		if ((statement != NULL) && (*statement == '['))
		{
			int r;
			statement++;
			char* ptr = statement;
			statement = pythonExpressionParser(statement, code, &r);
			if (r < 0)
			{
				*result = r;		//Error
				pythonDestroyVariableParser(properties);
				return ptr;
			}
			if (*statement != ']')
			{
				*result = ERROREXPECTBRACKET;
				pythonDestroyVariableParser(properties);
				return statement;
			}
			statement++;
			p.type = DYNAMICVARPROPERTY;
			p.staticproperty = NULL;
			p.dynamicid = r;
			properties.push_back(p);
		}
		else if ((statement != NULL) && (statement[0] == '.'))
		{
			statement++;
			statement = pythonToken((char*)statement, &type, &token);
			if (type != ALPHATYPECODE) { 
				res = false; 
				FREEMEM(token);  
				*result = ERROREXPECTVARIABLE; 
			}
			p.type = STATICVARPROPERTY;
			p.staticproperty = token;
			p.dynamicid = 0;
			properties.push_back(p);
		}
		else {
			end = true;
		}
	}
	//convert
	var->numproperty = properties.size();
	var->property = (VariableProperty*)ALLOCMEM(sizeof(VariableProperty) * var->numproperty);
	for (int i = 0; i < properties.size(); i++)
	{
		var->property[i] = properties[i];
	}
	return statement;
}

/**
* in case token ( found 
* variable.binding(expression,expression,)
* 9/5/2022
*/
char* pythonBindingParser(char* statement,BindingStatement func, PythonCode code, int* result)
{
	char* token = NULL;
	int type;
	statement = pythonToken((char*)statement, &type, &token);
	if (type != LOGICCODE) {
		if (token != NULL) FREEMEM(token);
		*result = ERROREXPECTBRACKET;
		return statement;
	}
	statement = trimToken(statement);
	func->numparam = 0;
	int maxparam = 1;
	int countbracket = 0;
	char* s = statement;
	bool end = false;
	//count max param
	while (!end)
	{
		if (*s == ',') maxparam++;
		else if (*s == '(') countbracket++;
		else if ((*s == ')') && (countbracket <= 0))  end = true;
		else if (*s == ')') countbracket--;
		else if ((*s == '\n') || (*s == 0)) end = true;
		s++;
	}
	func->param = (int*)ALLOCMEM(sizeof(int) * maxparam);
	//get param
	while (*statement != ')')
	{
		int r1;
		statement = pythonExpressionParser(statement, code, &r1);
		if (r1 < 0) { 
			destroyStatement((Statement)func);  
			*result = r1;  
			return statement; 
		}
		statement = trimToken(statement);
		if (*statement == ',') {
			statement++;
			statement = trimToken(statement);
		}
		func->param[func->numparam] = r1;
		func->numparam++;
	}
	statement = trimToken(statement);
	if (*statement != ')') {
		destroyStatement((Statement)func);
		*result = ERROREXPECTBRACKET;
		return statement;
	}
	statement++;
	*result = 0;
	return statement;
}

/**
* in case token ( found
*
* function(expression,expression,)
* 10/05/2022
*/
char* pythonCallingParser(char* statement, CallStatement func, PythonCode code, int* result)
{
	char* token = NULL;
	int type;
	statement=trimToken(statement);
	*result = 0;
	if (*statement != '(') {
		if (token != NULL) FREEMEM(token);
		*result = ERROREXPECTBRACKET;
		return statement;
	}
	statement++;
	statement = trimToken(statement);
	func->numparam = 0;
	int maxparam = 1;
	int countbracket = 0;
	char* s = statement;
	bool end = false;
	//count max param
	while (!end)
	{
		if (*s == ',') maxparam++;
		else if (*s == '(') countbracket++;
		else if ((*s == ')') && (countbracket <= 0))  end = true;
		else if (*s == ')') countbracket--;
		else if ((*s == '\n') || (*s == 0)) end = true;
		s++;
	}
	func->param = (int*)ALLOCMEM(sizeof(int) * maxparam);
	//get param
	while (*statement != ')')
	{
		int r1;
		statement = pythonExpressionParser(statement, code, &r1);
		if (r1 < 0) {
			destroyStatement((Statement)func);
			*result = r1;
			return statement;
		}
		statement = trimToken(statement);
		if (*statement == ',') {
			statement++;
			statement = trimToken(statement);
		}
		func->param[func->numparam] = r1;
		func->numparam++;
	}
	statement = trimToken(statement);
	if (*statement != ')') {
		destroyStatement((Statement)func);
		*result = ERROREXPECTBRACKET;
		return statement;
	}
	statement++;
	return statement;
}

/**
* execute
* assignment or binding or function
* a=10+20
* a.b(10)
* a[1].walk(10,20)
* a[10].walk(10,10)
* func(19)
* 
* 9/5/2022
*/
char* pythonExecuteParser(char* statement, char* next,PythonCode code, int* result)
{
	char* ptr1 = statement;
	if (*next == '(') //function case
	{
		int type;
		char* token;
		int sid = -1;
		statement = pythonToken(statement, &type, &token);
		for (int i = 0; i < code->subprocedure.size(); i++)
		{
			if (strcmp(code->subprocedure[i]->name, token) == 0) {
				sid = i;
				break;
			}
		}
		if (sid == -1) {	// Binding without variable
			int rl;
			BindingStatement b = (BindingStatement)initStatement(BINDINGSTATEMENT);
			b->left = 0;
			b->varname = NULL;
			b->func = token;
			*result = code->statement.size();
			code->statement.push_back((Statement)b);
			statement = pythonBindingParser(statement, b, code,&rl);
			if (rl < 0) {
				*result = rl;
			}
		}
		else {				// Call subprocedure
			int rl;
			FREEMEM(token);
			CallStatement b = (CallStatement)initStatement(CALLSTATEMENT);
			b->go = code->subprocedure[sid]->go;
			b->numparam = code->subprocedure[sid]->numparam;
			b->stack = code->subprocedure[sid]->numstack;
			*result = code->statement.size();
			code->statement.push_back((Statement)b);
			statement = pythonCallingParser(statement, b, code, &rl);
			if (rl < 0) {
				*result = rl;
			}
		}
		return statement;
	}
	int rl;	
	statement=pythonVariableParser(statement,  code, &rl);
	if (rl < 0) return statement;		//Parse error
	if (*statement == '(') {			//binding
		VariableStatement vstate=(VariableStatement)code->statement[rl];
		if (vstate->property[vstate->numproperty - 1].type == DYNAMICVARPROPERTY) {
			*result = ERRORSYNTAX;
			return statement;
		}
		//get var name 
		char* token;
		int type;
		pythonToken(ptr1, &type, &token);
		BindingStatement b = (BindingStatement)initStatement(BINDINGSTATEMENT);
		b->left = rl;
		b->varname = token;
		b->func = vstate->property[vstate->numproperty - 1].staticproperty;
		vstate->numproperty--;
		*result = code->statement.size();
		code->statement.push_back((Statement)b);
		int rp;
		statement=pythonBindingParser(statement, b, code, &rp);
		if (rp < 0) {
			*result = rp;
		}
	}
	else if (*statement == '=') {		//assignment
		int r;
		statement++;
		statement = pythonExpressionParser(statement, code, &r);
		if (r < 0) { 
			*result = r;  
			return statement; 
		}
		AssignmentStatement a = (AssignmentStatement)initStatement(ASSIGNSTATEMENT);
		a->starttext=ptr1-code->sourcecode;
		a->endtext = statement - code->sourcecode;
		a->right = r;
		a->left = rl;
		*result = code->statement.size();
		code->statement.push_back((Statement)a);
	}
	return statement;
}

/**
* get Operand
* Constant or variable or binding or function
* ex.
* a.func(1)
* a.b
* a
* 10
* a[0]
* 
* 9/5/2022
*/
char* pythonOperandParser(char* statement, PythonCode code, int* result)
{
	int r = 0;
	int type;
	char* token;
	statement = trimToken(statement);
	char* ptr1 = statement;
	statement = pythonToken(statement, &type, &token);
	if (type == ALPHATYPECODE) {
		statement = ptr1;		//back
		statement = pythonVariableParser(statement, code, &r);
		if (r < 0)
		{
			*result = r;
			return ptr1;
		}
		//binding
		if (*statement == '(') {
			statement = pythonBindingParser(statement, NULL,code, &r);
		}
	}
	else if ((type == NUMTYPECODE) || (type==STRINGTYPECODE))  {
		ConstantStatement con = (ConstantStatement)initStatement(CONSTANTSTATEMENT);
		con->constant.numproperties = 1;
		con->constant.properties = (Properties*)ALLOCMEM(sizeof(Properties));
		con->constant.properties[0].name = (char*)ALLOCMEM(1);
		con->constant.properties[0].name[0] = 0;
		con->constant.properties[0].value = (char*)ALLOCMEM(strlen(token)+1);
		strcpy(con->constant.properties[0].value, token);
		con->starttext = ptr1 - code->sourcecode;
		con->endtext = statement - code->sourcecode;
		r = code->statement.size();
		code->statement.push_back((Statement)con);
	}
	else {
		*result = ERROREXPECTVARIABLE;
		return ptr1;
	}
	*result = r;
	return statement;
}

/**
* get single comparation
* ex.
* operand compare operand
* a<b
* b<=c
*
* 9/5/2022
*/
char* pythonCompareParser(char* statement, PythonCode code, int* result)
{
	int type;
	int r1, r2;
	char* token1, *token2,*token;
	char* ptr = statement;

	//left 
	statement = pythonOperandParser(statement, code, &r1);
	if (r1 < 0) {
		*result = r1;
		return statement;
	}
	//operator
	statement = trimToken(statement);
	char* ns = statement + 1;
	char op[4]; op[0] = 0;
	if ((*statement == '=') && (*ns == '='))
	{
		strcpy(op, "=");
		statement += 2;
	}
	else if ((*statement == '!') && (*ns == '='))
	{
		strcpy(op, "!");
		statement += 2;
	}
	else if ((*statement == '<') && (*ns == '='))
	{
		strcpy(op, "<=");
		statement += 2;
	}
	else if ((*statement == '>') && (*ns == '='))
	{
		strcpy(op, ">=");
		statement += 2;
	}
	else if (*statement == '<')
	{
		strcpy(op, "<");
		statement ++;
	}
	else if (*statement == '>')
	{
		strcpy(op, ">");
		statement ++;
	}
	else
	{
		*result = ERROREXPECTOPERATOR;
		return statement;
	}
	//right
	statement=pythonOperandParser(statement, code, &r2);
	if (r2 < 0) {
		*result = r2;
		return statement;
	}
	CompareStatement com = (CompareStatement)initStatement(COMPARESTATEMENT);
	com->left = r1;
	com->right = r2;
	com->starttext = ptr-code->sourcecode;
	com->endtext = statement-code->sourcecode;
	strcpy(com->compare, op);
	*result = code->statement.size();
	code->statement.push_back((Statement)com);
	return statement;
}

/**
* logic statement for if, while, for
* ex.
* (a<=b) and (b>c) or !(e==f)
*
* 9/5/2022
*/
char* pythonLogicParser(char* statement, PythonCode code, int* result)
{
	int r1 = 0;
	int r2 = 0;
	char operand[12];
	int type;
	char* token;
	statement = trimToken(statement);
	//left
	if (*statement == '(') {
		statement++;
		statement = pythonLogicParser(statement, code, &r1);
		if (r1 < 0) { *result = r1;  return statement; }
		if (*statement != ')') { *result = r1; return statement; }
		statement++;
	} else if (*statement == '!') {
		statement++;
		statement = pythonLogicParser(statement, code, &r1);
		if (r1 < 0) { *result = r1;  return statement; }
		LogicStatement logicstatement = (LogicStatement)initStatement(LOGICSTATEMENT);
		logicstatement->left = r1;
		logicstatement->logic = '!';
		r1 = code->statement.size();
		code->statement.push_back((Statement)logicstatement);
	}
	else {
		statement = pythonCompareParser(statement, code, &r1);
	}
	//operation
	char* ptr1 = statement;
	statement = trimToken(statement);
	char* ns = statement + 1;
	char op[4];
	if (strstr(statement,"or")==statement)
	{
		strcpy(op, "|");
		statement += 2;
	}
	else if (strstr(statement,"and")==statement)
	{
		strcpy(op, "&");
		statement += 3;
	}
	else {
		//only single compare
		*result = r1;
		return statement;
	}
	//result
	statement = pythonLogicParser(statement, code, &r2);
	if (r2 < 0) { *result = r2; return statement; }
	LogicStatement logicstatement = (LogicStatement)initStatement(LOGICSTATEMENT);
	logicstatement->left = r1;
	logicstatement->right = r2;
	logicstatement->logic = op[0];
	*result = code->statement.size();
	code->statement.push_back((Statement)logicstatement);
	return statement;
}

/**
* get expression
* support complex ( and multiple operand
* ex.
* a
* a + b
* a+(b*10)+(c*20)
*
* a(20)+a.walk(20)+;
* 
* 9/5/2022
*/
char* pythonExpressionParser(char* statement, PythonCode code, int* result)
{
	int r1 = 0;
	int r2 = 0;
	char op[12];
	int type;
	char* token;
	statement = trimToken(statement);
	//left
	if (*statement == '(') {
		statement++;
		statement = pythonExpressionParser(statement, code, &r1);
		if (r1 < 0) { *result = r1;  return statement; }
		statement = trimToken(statement);
		if (*statement != ')') { *result = r1; return statement; }
		statement++;
	}
	else {
		statement = pythonOperandParser(statement, code, &r1);
		if (r1 < 0) {
			*result = r1;
			return statement;
		}
	}
	//operation
	statement = trimToken(statement);
	char* ptr1 = statement;
	if (ISOPERATOR(*statement))
	{
		statement = pythonToken(statement, &type, &token);
		strcpy(op, token);
		FREEMEM(token);
	}
	else
	{	//only variable
		*result = r1;
		return statement;
	}
	//right
	char* ptr2 = statement;
	statement = pythonExpressionParser(statement, code, &r2);
	if (r2 < 0)
	{
		*result = r2;
		return ptr2;
	}
	//result
	ExpressionStatement expressstatement = (ExpressionStatement)initStatement(EXPRESSIONSTATEMENT);
	expressstatement->left = r1;
	expressstatement->right = r2;
	expressstatement->op = op[0];
	*result = code->statement.size();
	code->statement.push_back((Statement)expressstatement);
	return statement;
}

/**
* 
* TODO:
*/
char* pythonForParser(char* statement, PythonCode code,int currentblock, int* result)
{
	int type;
	int r1,r2;
	char* token;
	ForStatement forstatement = (ForStatement)initStatement(FORSTATEMENT);
	int fw = code->statement.size();
	code->statement.push_back((Statement)forstatement);
	statement = pythonToken(statement, &type, &token);
	if (strcmp(token, "range") == 0) {
		//forstatement->begin;
		//forstatement->logic;
		//forstatement->step;
	}
	else {
		*result = ERRORLOOPRANGE;
		return statement;
	}
	statement = pythonRoutineParser((char*)statement, code, currentblock + 1, &r1,&r2);
	if (r1 < 0) {
		*result = r1;
		return statement;
	}
	forstatement->dofor = r2;
	code->statement[r1]->next = fw;
	*result = fw;
	return statement;
}

/**
*
* 10/5/2022
*/
char* pythonWhileParser(char* statement, PythonCode code,int currentblock, int* result)
{
	int r1,r2;
	WhileStatement whilestatement = (WhileStatement)initStatement(WHILESTATEMENT);
	int rw = code->statement.size();
	code->statement.push_back((Statement)whilestatement);
	statement = pythonLogicParser(statement, code, &r1);
	whilestatement->logic = r1;
	if (r1 < 0) {
		*result = r1;
		return statement;
	}
	statement = trimToken(statement);
	if (*statement != ':') {
		*result = ERROREXPECTCOLON;
		return statement;
	}
	statement++;
	statement = trimToken(statement);
	if (*statement != '\n') {
		*result = ERROREXPECTCOLON;
		return statement;
	}
	statement++;
	statement = pythonRoutineParser((char*)statement, code, currentblock + 1, &r1,&r2);
	if (r1 < 0) {
		*result = r1;
		return statement;
	}
	whilestatement->dowhile = r2;
	*result = rw;
	return statement;
}

/**
*
* 10/5/2022
*/
char* pythonIfParser(char* statement, PythonCode code, int currentblock,int* resultif,int* resultlast)
{
	int rl,r1,r2;
	IfStatement ifstatement = (IfStatement)initStatement(IFSTATEMENT);
	int is = code->statement.size();
	code->statement.push_back((Statement)ifstatement);
	statement = pythonLogicParser((char*)statement, code, &rl);
	if (rl < 0) {
		*resultif = rl;
		return statement;
	}
	statement = trimToken(statement);
	if (*statement == ':') {
		statement++;
		statement = trimToken(statement);
	}
	statement = pythonRoutineParser(statement, code, currentblock + 1, &r1,&r2);
	ifstatement->logic = rl;
	ifstatement->ifthen = r2;
	ifstatement->next = 0;
	*resultif = is;
	*resultlast = r1;
	return statement;
}

/*
*
* 10/5/2022
*/
char* pythonElseIfParser(char* statement, PythonCode code,int currentblock, int* resultif,int* resultlast)
{
	int rl,r1,r2;
	IfStatement elseifstatement = (IfStatement)initStatement(IFSTATEMENT);
	int is = code->statement.size();
	code->statement.push_back((Statement)elseifstatement);
	statement = pythonLogicParser((char*)statement, code, &rl);
	if (rl < 0) {
		*resultif = rl;
		return statement;
	}
	statement = trimToken(statement);
	if (*statement == ':') {
		statement++;
		statement = trimToken(statement);
	}
	statement = pythonRoutineParser(statement, code, currentblock + 1, &r1,&r2);
	elseifstatement->logic = rl;
	elseifstatement->ifthen = r2;
	elseifstatement->next = 0;
	*resultif = is;
	*resultlast = r1;
	return statement;
}

/*
* 
* 10/5/2022
*/
char* pythonElseParser(char* statement, PythonCode code, int currentblock,int* resultif,int* resultlast)
{
	int r1,r2;
	IfStatement elsestatement = (IfStatement)initStatement(IFSTATEMENT);
	elsestatement->logic = 0;
	int is = code->statement.size();
	code->statement.push_back((Statement)elsestatement);
	statement = trimToken(statement);
	if (*statement == ':') {
		statement++;
		statement = trimToken(statement);
	}
	statement = pythonRoutineParser(statement, code, currentblock + 1, &r1,&r2);
	elsestatement->ifthen = r2;
	elsestatement->next = 0;
	*resultif = is;
	*resultlast = r1;
	return statement;
}

/**
* Subprocedure
* 10/05/2022
*/
char* pythonDefParser(char* statement, PythonCode code,int currentblock, int* result)
{
	char* token = NULL;
	int type;
	
	statement=trimToken(statement);
	statement = pythonToken((char*)statement, &type, &token);
	if (type != ALPHATYPECODE) {
		if (token != NULL) FREEMEM(token);
		*result = ERROREXPECTFUNCTION;
	}
	if (*statement != '(') {
		if (token != NULL) FREEMEM(token);
		*result = ERROREXPECTBRACKET;
		return statement;
	}
	statement++;
	statement = trimToken(statement);

	PythonSubprocedure* sub = (PythonSubprocedure*)ALLOCMEM(sizeof(struct PythonSubprocedureS));
	sub->name = token;
	sub->numparam = 0;
	int maxparam = 1;	//at least
	int countbracket = 0;
	char* s = statement;
	bool end = false;
	//count max param
	while (!end)
	{
		if (*s == ',') maxparam++;
		else if (*s == '(') countbracket++;
		else if ((*s == ')') && (countbracket <= 0))  end = true;
		else if (*s == ')') countbracket--;
		else if ((*s == '\n') || (*s == 0)) end = true;
		s++;
	}
	sub->param = (char**)ALLOCMEM(sizeof(char*)*maxparam);
	//get param
	while (*statement != ')')
	{
		statement = pythonToken((char*)statement, &type, &token);
		if (type != ALPHATYPECODE) {
			if (token != NULL) FREEMEM(token);
			*result = ERROREXPECTVARIABLE;
		}
		statement = trimToken(statement);
		if (*statement == ',') {
			statement++;
			statement = trimToken(statement);
		}
		else if (*statement != ')') {
			if (token != NULL) FREEMEM(token);
			pythonDestroySubprocedure(sub);
			*result = ERROREXPECTBRACKET;  
			return statement;
		}
		sub->param[sub->numparam]=token;
		sub->numparam++;
	}
	statement = trimToken(statement);
	if (*statement != ')') {
		pythonDestroySubprocedure(sub);
		*result = ERROREXPECTBRACKET;
		return statement;
	}
	statement++;
	statement = trimToken(statement);
	if (*statement != ':') {
		pythonDestroySubprocedure(sub);
		*result = ERROREXPECTBRACKET;
		return statement;
	}
	statement++;
	int rl;
	code->globalOrLocal = false;
	//load localvar
	code->localVar.clear();
	for (int i = 0; i < sub->numparam; i++)
	{
		code->localVar.push_back(sub->param[i]);
	}
	int startstatement = code->statement.size();
	statement=pythonRoutineParser(statement, code, currentblock + 1, result, &rl);
	sub->go = rl;
	sub->numstack = sub->numparam;
	code->globalOrLocal = true;
	code->subprocedure.push_back(sub);
	code->currentSub = sub;
	if (*result < 0) {
		return statement;
	}
	//add return null here, check ->next=0 to go to return
	rl = code->statement.size();
	ReturnStatement rs = (ReturnStatement)initStatement(RETURNSTATEMENT);
	rs->ret = 0;
	rs->stack=sub->numstack;
	code->statement.push_back((Statement)rs);
	for (int i = startstatement; i < rl; i++)
	{
		Statement s = code->statement[i];
		if (s->next == 0) {
			if (s->type == ASSIGNSTATEMENT) {
				s->next = rl;
			}
			else if (s->type == FORSTATEMENT) {
				s->next = rl;
			}
			else if (s->type == WHILESTATEMENT) {
				s->next = rl;
			}
			else if (s->type == CALLSTATEMENT) {
				s->next = rl;
			}
			else if (s->type == BINDINGSTATEMENT) {
				s->next = rl;
			}			
		}
	}
	return statement;
}

/**
* get block (tab)
* 10/5/2022
*/
char* pythonBlock(char* statement, int* block)
{
	*block = 0;
	while ((*statement == ' ') || (*statement=='\t')) {
		statement++;
		*block = (*block) + 1;
	}
	return statement;
}

/*
* set jump in if clause
* 10/5/2022
*/
void pythonNextIf(PythonCode code, std::vector<int> iflaststatement, int r)
{
	for (int i = 0; i < iflaststatement.size(); i++)
	{
		code->statement[iflaststatement[i]]->next = r;
	}
}

/**
* parsing source code to codeblock
* 
* @param statement source code
* @param code the binary code
* @param result is the first statement
* @param resultlast is the last statement
*
* @return next in the block
* 
* 10/5/2022
*/
char* pythonRoutineParser(char* statement, PythonCode code, int currentblock,int* result,int* resultfirst)
{
	int type=0;
	int r = 0;
	char* token=NULL;
	Statement last = NULL;
	int ifstatement=0;
	std::vector<int> iflaststatement;
	int fw,lw;
	int previfstatement = 0;
	*resultfirst = 0;
	while (*statement != 0)
	{
		int block=0;
		statement = pythonBlock((char*)statement, &block);
		char* ptr = statement;						 
		statement = pythonToken(statement, &type, &token);
		if (type == COMMENTCODE) {
			statement=nextTokenTo(statement,'\n');
		}
		else if (type == DELIMITERCODE) {			//do nothing
		}
		else if (type == ENDCODE) {
			*result = r;  
			return statement;
		} else {
			if (block > currentblock) {
				*result = ERRORTAB;
				return statement;
			}
			else if (block<currentblock) {			//less tab
				*result = r;
				return ptr;
			}
			else if (block == currentblock) {
				if (type == ALPHATYPECODE) {
					fw = lw = 0;
					int previfstatement = ifstatement;
					if (strcmp(token, "if") == 0)
					{
						FREEMEM(token);
						statement = pythonIfParser(statement, code, currentblock, &fw, &lw);
						if (fw > 0) {
							code->statement[fw]->starttext = ptr-code->sourcecode;
							code->statement[fw]->endtext = statement-code->sourcecode;
							iflaststatement.clear();
							r = ifstatement = fw;
							iflaststatement.push_back(lw);
						} else {
							*result = fw;
							return statement;
						}
					}
					else if (strcmp(token, "elif") == 0)
					{
						FREEMEM(token);
						if (ifstatement==0)
						{
							*result = ERRORREQUIREIF;
							return statement;
						}
						statement = pythonElseIfParser(statement, code, currentblock, &fw, &lw);
						if (fw > 0) {
							code->statement[fw]->starttext = ptr - code->sourcecode;
							code->statement[fw]->endtext = statement - code->sourcecode;
							r = ifstatement = fw;
							iflaststatement.push_back(lw);
						} else {
							*result = fw;
							return statement;
						}
					}
					else if (strcmp(token, "else") == 0)
					{
						FREEMEM(token);
						if (ifstatement==0)
						{
							*result = ERRORREQUIREIF;
							return statement;
						}
						statement = pythonElseParser(statement, code, currentblock, &fw, &lw);
						if (fw > 0) {
							code->statement[fw]->starttext = ptr - code->sourcecode;
							code->statement[fw]->endtext = statement - code->sourcecode;
							r = ifstatement = fw;
							iflaststatement.push_back(lw);
						} else {
							*result = fw;
							return statement;
						}
					}
					else if (strcmp(token, "for") == 0)
					{
						ifstatement = 0;
						FREEMEM(token);
						statement = pythonForParser(statement, code, currentblock, &r);
						if (r > 0) {
							code->statement[r]->starttext = ptr - code->sourcecode;
							code->statement[r]->endtext = statement - code->sourcecode;
						} else {
							*result = r;
							return statement;
						}
					}
					else if (strcmp(token, "while") == 0)
					{
						ifstatement = 0;
						FREEMEM(token);
						statement = pythonWhileParser(statement, code, currentblock, &r);
						if (r > 0) {
							code->statement[r]->starttext = ptr - code->sourcecode;
							code->statement[r]->endtext = statement - code->sourcecode;
						} else {
							*result = r;
							return statement;
						}
					}
					else if (strcmp(token, "def") == 0)
					{
						ifstatement = 0;
						FREEMEM(token);
						if (!code->globalOrLocal) {
							*result = ERRORSUBPROCEDUREINVALID;
							return ptr;
						}
						int r;
						statement = pythonDefParser((char*)statement, code, currentblock, &r);
						if (r < 0) {
							*result = r;
							return statement;
						}
						r = 0;
					}
					else if (strcmp(token, "return") == 0) 
					{
						ifstatement = 0;
						FREEMEM(token);
						if (code->globalOrLocal) {
							*result = ERRORSUBPROCEDUREINVALID;
							return ptr;
						}
						int r;
						statement = pythonExpressionParser((char*)statement, code, &r);
						if (r < 0) {
							*result = r;
							return statement;
						}
					}
					else {
						ifstatement = 0;
						char* next = statement;
						statement = ptr; //back
						statement = pythonExecuteParser(statement, next, code, &r);
						if (r < 0) {
							*result = r;
							return statement;
						}
					}
					if ((*resultfirst==0) && (r>0)) {
						*resultfirst = r;
					}	
				}
				else {
					*result = ERRORSYNTAX;
					return statement;
				}
			}
			//set next 
			if ((last != NULL) && (r != 0)) {
				last->next = r;
			}
			last = (r != 0)?(Statement)code->statement[r]:NULL;
			if ((previfstatement != 0) && (ifstatement==0)) {
				pythonNextIf(code, iflaststatement, r);
			}
		}
	}
	*result = r;
	return statement;	
}

CodeBlock pythonParser(const char* statement)
{
	int i;	
	CodeBlock c;
	char* sta = (char*)ALLOCMEM(strlen(statement) + 1);
	strcpy(sta, statement);
	int numline = getLines((char*)statement);
	PythonCode pc = new PythonCodeS();
	pc->globalOrLocal = true;
	pc->sourcecode = sta;
	pc->statement.push_back((Statement)initStatement(0));					//dummy
	int result = -1;
	int resultfirst;
	char* p = pythonRoutineParser(sta, pc, 0, &result,&resultfirst);		//go go go!
	if (result < 0) {
		c = NULL;
	} else {
		c = initCodeBlock(10000);
		c->start = resultfirst;
		c->numstatement = pc->statement.size();
		c->statement = (Statement*)ALLOCMEM(sizeof(struct StatementS) * c->numstatement);
		c->statement[0] = NULL;
		for (int i = 1; i < pc->statement.size(); i++)
		{
			c->statement[i] = pc->statement[i];
		}
		c->numglobalvar = pc->globalVar.size();
		c->sourcecode = (char*)ALLOCMEM(strlen(statement) + 1);
		strcpy(c->sourcecode, statement);
	}
	delete pc;
	FREEMEM(sta); 
	return c;
}

