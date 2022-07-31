#include "interpreter.h"
#include "binaryparser.h"

#pragma pack(push,1)
typedef struct BinaryCodeBlockS* BinaryCodeBlock;
struct BinaryCodeBlockS {
	uint32_t version;
	uint32_t numstatement;
	uint32_t start;		//start point
	uint32_t numvar;	//num of global variable
	uint32_t codesize;  //0=not available
} PACKED;

struct BinaryStatementS {
	char type;
	uint32_t next;
	uint32_t starttext;
	uint32_t endtext;
} PACKED;

struct BinaryPropertiesS {
	char vartype;				//STRINGVAR,ARRAYVAR,STRUCTUREVAR,ARRAYOFSTRUCTUREVAR
	uint32_t namelength;		//name of property (can be NULL)
	uint32_t valuelength;		//array statement, can be 0
	//name..
	//value...
} PACKED;

struct BinaryConstantStatementS {
	char type;
	uint32_t next;
	uint32_t starttext;
	uint32_t endtext;
	uint32_t numproperties;
	//binaryproperties...
} PACKED;

struct BinaryDynamicVariablePropertiesStatementS {
	char type;				//DYNAMIC
	uint32_t next;
} PACKED;

struct BinaryStaticVariablePropertiesStatementS {
	char type;				//STATIC 
	uint32_t namelength;
	//name...
} PACKED;

struct BinaryVariableStatementS {
	char type;
	uint32_t next;
	uint32_t starttext;
	uint32_t endtext;
	uint32_t vartype;		//GLOBALVAR, LOCALVAR
	uint32_t vid;
	uint32_t numproperties;
	//properties...
} PACKED;

struct BinaryBindingStatementS {
	char type;
	uint32_t next;
	uint32_t starttext;
	uint32_t endtext;
	uint32_t left;				//variable statement
	uint32_t varlength;			//variable name
	uint32_t funclength;		//function name
	uint32_t numparam;
	//function name
	//param ...uint32_t
} PACKED;

struct BinaryCallStatementS {
	char type;
	uint32_t next;
	uint32_t starttext;
	uint32_t endtext;
	uint32_t go;
	uint32_t stack;
	uint32_t numparam;
	//param ...
} PACKED;

struct BinaryReturnStatementS {
	char type;
	uint32_t next;
	uint32_t starttext;
	uint32_t endtext;
	uint32_t stack;				//decrease stack
	uint32_t ret;				//variable or constant to return
} PACKED;

struct BinaryCompareStatementS {
	char type;
	uint32_t next;
	uint32_t starttext;
	uint32_t endtext;
	char compare[4];			//== > >= < <=
	uint32_t left;				//left statement
	uint32_t right;				//right statement
} PACKED;

struct BinaryLogicStatementS
{
	char type;
	uint32_t next;
	uint32_t starttext;
	uint32_t endtext;
	char logic;					//& | !
	uint32_t left;
	uint32_t right;
} PACKED;

struct BinaryExpressionStatementS
{
	char type;
	uint32_t next;
	uint32_t starttext;
	uint32_t endtext;
	char op;					//+ - * / % & | xor
	uint32_t left;
	uint32_t right;
} PACKED;

/*struct BinaryForStatementS
{
	char type;
	uint32_t next;
	uint32_t starttext;
	uint32_t endtext;
	int begin;		//x=0
	int logic;		//x<10		
	int step;		//x++
	int dofor;
} PACKED;*/

struct BinaryWhileStatementS
{
	char type;
	uint32_t next;
	uint32_t starttext;
	uint32_t endtext;
	uint32_t logic;
	uint32_t dowhile;
} PACKED;

struct BinaryIfStatementS
{
	char type;
	uint32_t next;
	uint32_t starttext;
	uint32_t endtext;
	uint32_t logic;			//0= else 
	uint32_t ifthen;
} PACKED;

struct BinaryAssignmentStatementS {
	char type;
	uint32_t next;
	uint32_t starttext;
	uint32_t endtext;
	uint32_t left;		//variable
	uint32_t right;		//expression 
} PACKED;
#pragma pack(pop)

CodeBlock loadcodeblock(void* binary, size_t size)
{
	if (binary == NULL) return NULL;
	BinaryCodeBlock b = (BinaryCodeBlock)binary;
	CodeBlock c = initCodeBlock(10000);
	c->start = b->start;
	c->numstatement = b->numstatement;
	c->numglobalvar = b->numvar;
	c->statement = (Statement*)ALLOCMEM(sizeof(struct StatementS) * (c->numstatement+1));
	c->statement[0] = NULL;
	c->var = NULL;
	char* bin = (char*)binary;
	bin += sizeof(struct BinaryCodeBlockS);
	//read statement
	for (int j = 1; j < b->numstatement; j++)
	{
		struct BinaryStatementS* s = (struct BinaryStatementS*)bin;
		switch (s->type) {
			case CONSTANTSTATEMENT: {
				struct BinaryConstantStatementS* constantbin = (struct BinaryConstantStatementS*)bin;
				ConstantStatement constant = (ConstantStatement)initStatement(CONSTANTSTATEMENT);
				constant->starttext = constantbin->starttext;
				constant->endtext = constantbin->endtext;
				constant->next = constantbin->next;
				constant->constant.numproperties = constantbin->numproperties;
				constant->constant.properties = (Properties*)ALLOCMEM(constant->constant.numproperties*sizeof(struct PropertiesS));
				bin += sizeof(struct BinaryConstantStatementS);
				for (int i = 0; i < constantbin->numproperties; i++)
				{
					struct BinaryPropertiesS* pp = (struct BinaryPropertiesS*)bin;
					bin += sizeof(struct BinaryPropertiesS);
					constant->constant.properties[i].name=(char*)ALLOCMEM(pp->namelength+1);
					constant->constant.properties[i].value=(char*)ALLOCMEM(pp->valuelength+1);
					CPYMEM(constant->constant.properties[i].name, bin,pp->namelength);
					constant->constant.properties[i].name[pp->namelength] = 0;
					bin += pp->namelength;
					CPYMEM(constant->constant.properties[i].value, bin,pp->valuelength);
					constant->constant.properties[i].value[pp->valuelength] = 0;
					bin += pp->valuelength;
				}
				c->statement[j] = (Statement)constant;
			} break;
			case VARIABLESTATEMENT: {
				struct BinaryVariableStatementS* varbin = (struct BinaryVariableStatementS*)bin;
				VariableStatement var = (VariableStatement)initStatement(VARIABLESTATEMENT);
				var->starttext = varbin->starttext;
				var->endtext = varbin->endtext;
				var->next = varbin->next;
				var->vartype = varbin->vartype;
				var->vid = varbin->vid;
				bin += sizeof(struct BinaryVariableStatementS);
				var->numproperty = varbin->numproperties;
				if (var->numproperty == 0) var->property = NULL;
				else {
					var->property = (VariableProperty*)ALLOCMEM(sizeof(VariableProperty)*varbin->numproperties);
					for (int i = 0; i < varbin->numproperties; i++) {
						struct BinaryDynamicVariablePropertiesStatementS* dvarbin = (struct BinaryDynamicVariablePropertiesStatementS*)bin;
						var->property[i].type = dvarbin->type;
						if (dvarbin->type == DYNAMICVARPROPERTY) {
							var->property[i].dynamicid = dvarbin->next;
							var->property[i].staticproperty = NULL;
							bin += sizeof(struct BinaryDynamicVariablePropertiesStatementS);
						}
						else
						{
							struct BinaryStaticVariablePropertiesStatementS* svarbin = (struct BinaryStaticVariablePropertiesStatementS*)bin;
							var->property[i].staticproperty = (char*)ALLOCMEM(svarbin->namelength + 1);
							bin += sizeof(struct BinaryStaticVariablePropertiesStatementS);
							CPYMEM(var->property[i].staticproperty, bin, svarbin->namelength);
							var->property[i].staticproperty[svarbin->namelength] = 0;
							bin += svarbin->namelength;
						}
					}
				}
				c->statement[j] = (Statement)var;
			} break;
			case EXPRESSIONSTATEMENT: {
				struct BinaryExpressionStatementS* expressionbin = (struct BinaryExpressionStatementS*)bin;
				ExpressionStatement expression = (ExpressionStatement)initStatement(EXPRESSIONSTATEMENT);
				expression->starttext = expressionbin->starttext;
				expression->endtext = expressionbin->endtext;
				expression->next = expressionbin->next;
				expression->left = expressionbin->left;
				expression->right = expressionbin->right;
				expression->op = expressionbin->op;
				bin += sizeof(struct BinaryExpressionStatementS);
			    c->statement[j] = (Statement)expression;
			} break;
			case LOGICSTATEMENT: {
				struct BinaryLogicStatementS* logicbin = (struct BinaryLogicStatementS*)bin;
				LogicStatement logic = (LogicStatement)initStatement(LOGICSTATEMENT);
				logic->starttext = logicbin->starttext;
				logic->endtext = logicbin->endtext;
				logic->next = logicbin->next;
				logic->left = logicbin->left;
				logic->right = logicbin->right;
				logic->logic = logicbin->logic;
				bin += sizeof(struct BinaryLogicStatementS);
				c->statement[j] = (Statement)logic;
			} break;
			case COMPARESTATEMENT: {
				struct BinaryCompareStatementS* comparebin = (struct BinaryCompareStatementS*)bin;
				CompareStatement compare = (CompareStatement)initStatement(COMPARESTATEMENT);
				compare->starttext = comparebin->starttext;
				compare->endtext = comparebin->endtext;
				compare->next = comparebin->next;
				compare->left = comparebin->left;
				compare->right = comparebin->right;
				CPYMEM(compare->compare,comparebin->compare,sizeof(compare->compare));
				bin += sizeof(struct BinaryCompareStatementS);
				c->statement[j] = (Statement)compare;
			} break;
			case ASSIGNSTATEMENT: {
				struct BinaryAssignmentStatementS* assignbin = (struct BinaryAssignmentStatementS*)bin;
				AssignmentStatement assign = (AssignmentStatement)initStatement(ASSIGNSTATEMENT);
				assign->starttext = assignbin->starttext;
				assign->endtext = assignbin->endtext;
				assign->next = assignbin->next;
				assign->left = assignbin->left;
				assign->right = assignbin->right;
				bin += sizeof(struct BinaryAssignmentStatementS);
				c->statement[j] = (Statement)assign;
			} break;
			case BINDINGSTATEMENT: {
				struct BinaryBindingStatementS* funcbin = (struct BinaryBindingStatementS*)bin;
				BindingStatement func = (BindingStatement)initStatement(BINDINGSTATEMENT);
				if (funcbin->varlength > 0) {
					func->varname = (char*)ALLOCMEM(funcbin->varlength + 1);
				}
				else
					func->varname = NULL;
				if (funcbin->funclength > 0) {
					func->func = (char*)ALLOCMEM(funcbin->funclength+1);
				}
				else
					func->func = NULL;
				if (funcbin->numparam > 0) {
					func->param = (int*)ALLOCMEM(sizeof(int)*funcbin->numparam);
				}
				else
					func->param = NULL;
				func->starttext = funcbin->starttext;
				func->endtext = funcbin->endtext;
				func->next = funcbin->next;
				func->numparam = funcbin->numparam;
				func->left = funcbin->left;
				bin += sizeof(struct BinaryBindingStatementS);
				if (funcbin->varlength > 0) {
					CPYMEM(func->varname, bin, funcbin->varlength);
					func->varname[funcbin->varlength] = 0;
				}
				bin += funcbin->varlength;
				if (funcbin->funclength > 0) {
					CPYMEM(func->func, bin, funcbin->funclength);
					func->func[funcbin->funclength] = 0;
				}
				bin += funcbin->funclength;
				for (int i = 0; i < funcbin->numparam; i++)
				{
					uint32_t* p = (uint32_t*)bin;
					func->param[i] = *p;
					bin += sizeof(uint32_t);
				}
				c->statement[j] = (Statement)func;
			} break;
			case IFSTATEMENT: {
				struct BinaryIfStatementS* ifbin = (struct BinaryIfStatementS*)bin;
				IfStatement ifstatement = (IfStatement)initStatement(IFSTATEMENT);
				ifstatement->starttext = ifbin->starttext;
				ifstatement->endtext = ifbin->endtext;
				ifstatement->next = ifbin->next;
				ifstatement->ifthen = ifbin->ifthen;
				ifstatement->logic = ifbin->logic;
				bin += sizeof(struct BinaryIfStatementS);
				c->statement[j] = (Statement)ifstatement;
			} break;
			case CALLSTATEMENT: {
				struct BinaryCallStatementS* callbin = (struct BinaryCallStatementS*)bin;
				CallStatement callstatement = (CallStatement)initStatement(CALLSTATEMENT);
				callstatement->starttext = callbin->starttext;
				callstatement->endtext = callbin->endtext;
				callstatement->next = callbin->next;
				callstatement->go = callbin->go;
				callstatement->stack = callbin->stack;
				callstatement->numparam = callbin->numparam;
				callstatement->param = (int*)ALLOCMEM(callstatement->numparam*sizeof(int));
				bin += sizeof(struct BinaryCallStatementS);
				for (int i = 0; i < callbin->numparam; i++)
				{
					uint32_t* p = (uint32_t*)bin;
					callstatement->param[i] = *p;
					bin += sizeof(uint32_t);
				}
				c->statement[j] = (Statement)callstatement;
			} break;
			case RETURNSTATEMENT: {
				struct BinaryReturnStatementS* returnbin = (struct BinaryReturnStatementS*)bin;
				ReturnStatement returnstatement = (ReturnStatement)initStatement(RETURNSTATEMENT);
				returnstatement->starttext = returnbin->starttext;
				returnstatement->endtext = returnbin->endtext;
				returnstatement->next = returnbin->next;
				returnstatement->ret = returnbin->ret;
				returnstatement->stack = returnbin->stack;
				bin += sizeof(struct BinaryReturnStatementS);
				c->statement[j] = (Statement)returnstatement;
			} break;
			case WHILESTATEMENT: {
				struct BinaryWhileStatementS* whilebin = (struct BinaryWhileStatementS*)bin;
				WhileStatement whilestatement = (WhileStatement)initStatement(WHILESTATEMENT);
				whilestatement->starttext = whilebin->starttext;
				whilestatement->endtext = whilebin->endtext;
				whilestatement->next = whilebin->next;
				whilestatement->dowhile = whilebin->dowhile;
				whilestatement->logic = whilebin->logic;
				bin += sizeof(struct BinaryWhileStatementS);
				c->statement[j] = (Statement)whilestatement;
			} break;
			/*case FORSTATEMENT: {
				struct BinaryForStatementS* forbin = (struct BinaryForStatementS*)bin;
				ForStatement forstatement = (ForStatement)initStatement(FORSTATEMENT);
				forstatement->starttext = forbin->starttext;
				forstatement->endtext = forbin->endtext;
				forstatement->next = forbin->next;
				forstatement->begin = forbin->begin;
				forstatement->logic = forbin->logic;
				forstatement->dofor = forbin->dofor;
				bin += sizeof(struct BinaryForStatementS);
				c->statement[j] = (Statement)forstatement;
			} break;*/
			default: {
				//error
			} break;
		}

	}
	//read source code
	if (b->codesize > 0) {
		c->sourcecode = (char*)ALLOCMEM(b->codesize+1);
		CPYMEM(c->sourcecode,bin,b->codesize);
		c->sourcecode[b->codesize] = 0;
	}
	return c;
}

struct codeblockbuffer {
	char* buffer;
	size_t index;
	size_t size;
};

void codeblockwrite(struct codeblockbuffer* buf,void* data,size_t size)
{
	size_t s = 0;
	if (buf->size - buf->index<size) {
		s = size > 1024 ? size : 1024;
		buf->buffer = (buf->buffer==NULL)?(char*)ALLOCMEM(s): (char*)REALLOCMEM(buf->buffer,buf->size+s);
		buf->size += s;
	}
	CPYMEM(&buf->buffer[buf->index],data,size);
	buf->index += size;
}

void* savecodeblock(CodeBlock code, size_t* size)
{
	struct codeblockbuffer buf; 
	buf.buffer = NULL;
	buf.index = buf.size=0;

	struct BinaryCodeBlockS binaryCodeBlock;
	binaryCodeBlock.version = 1;
	binaryCodeBlock.numstatement = code->numstatement;
	binaryCodeBlock.numvar = code->numglobalvar;
	binaryCodeBlock.start = code->start;
	binaryCodeBlock.codesize = (code->sourcecode!=NULL)?strlen(code->sourcecode):0;
	codeblockwrite(&buf,(void*)&binaryCodeBlock,sizeof(struct BinaryCodeBlockS));
	//write statement, always start at 1
	for (int j = 1; j < code->numstatement; j++)
	{
		switch (code->statement[j]->type)
		{
			case CONSTANTSTATEMENT: {
				struct BinaryConstantStatementS binaryconstant;
				ConstantStatement constantstatement = (ConstantStatement)code->statement[j];
				binaryconstant.type = constantstatement->type;
				binaryconstant.next = constantstatement->next;
				binaryconstant.starttext = constantstatement->starttext;
				binaryconstant.endtext = constantstatement->endtext;
				binaryconstant.numproperties = constantstatement->constant.numproperties;
				codeblockwrite(&buf, &binaryconstant, sizeof(struct BinaryConstantStatementS));
				//write value
				Value* value = &constantstatement->constant;
				for (int i=0;i<value->numproperties;i++)
				{
					struct BinaryPropertiesS binarypropertie;
					Properties* pp = &value->properties[i];
					binarypropertie.namelength = strlen(pp->name);
					binarypropertie.valuelength = strlen(pp->value);
					codeblockwrite(&buf, &binarypropertie, sizeof(struct BinaryPropertiesS));
					if (pp->name) {
						codeblockwrite(&buf, pp->name, strlen(pp->name));
					}
					if (pp->value) {
						codeblockwrite(&buf, pp->value, strlen(pp->value));
					}
				}
			} break;
			case VARIABLESTATEMENT: {
				struct BinaryVariableStatementS binaryvar;
				VariableStatement varstatement = (VariableStatement)code->statement[j];
				binaryvar.type = varstatement->type;
				binaryvar.next = varstatement->next;
				binaryvar.starttext = varstatement->starttext;
				binaryvar.endtext = varstatement->endtext;
				binaryvar.vartype = varstatement->vartype;
				binaryvar.vid=varstatement->vid;
				binaryvar.numproperties = varstatement->numproperty;
				codeblockwrite(&buf, &binaryvar, sizeof(struct BinaryVariableStatementS));
				for (int i = 0; i < binaryvar.numproperties; i++)
				{
					if (varstatement->property[i].type == DYNAMICVARPROPERTY)
					{
						struct BinaryDynamicVariablePropertiesStatementS dvar;
						dvar.type = DYNAMICVARPROPERTY;
						dvar.next = varstatement->property[i].dynamicid;
						codeblockwrite(&buf, &dvar, sizeof(struct BinaryDynamicVariablePropertiesStatementS));
					}
					else {
						struct BinaryStaticVariablePropertiesStatementS svar;
						svar.type = STATICVARPROPERTY;
						svar.namelength = strlen(varstatement->property[i].staticproperty);
						codeblockwrite(&buf, &svar, sizeof(struct BinaryStaticVariablePropertiesStatementS));
						codeblockwrite(&buf, varstatement->property[i].staticproperty, svar.namelength);
					}
				}
			} break;
			case EXPRESSIONSTATEMENT: {
				struct BinaryExpressionStatementS binaryexpression;
				ExpressionStatement expressionstatement = (ExpressionStatement)code->statement[j];
				binaryexpression.type = expressionstatement->type;
				binaryexpression.next = expressionstatement->next;
				binaryexpression.starttext = expressionstatement->starttext;
				binaryexpression.endtext = expressionstatement->endtext;
				binaryexpression.left = expressionstatement->left;
				binaryexpression.right = expressionstatement->right;
				binaryexpression.op= expressionstatement->op;
				codeblockwrite(&buf, &binaryexpression, sizeof(struct BinaryExpressionStatementS));
			} break;
			case LOGICSTATEMENT:
			{
				struct BinaryLogicStatementS binarylogic;
				LogicStatement logicstatement = (LogicStatement)code->statement[j];
				binarylogic.type = logicstatement->type;
				binarylogic.next = logicstatement->next;
				binarylogic.starttext = logicstatement->starttext;
				binarylogic.endtext = logicstatement->endtext;
				binarylogic.logic = logicstatement->logic;
				binarylogic.left = logicstatement->left;
				binarylogic.right = logicstatement->right;
				codeblockwrite(&buf, &binarylogic, sizeof(struct BinaryLogicStatementS));
			} break;
			case COMPARESTATEMENT:
			{
				struct BinaryCompareStatementS binarycompare;
				CompareStatement comparestatement = (CompareStatement)code->statement[j];
				binarycompare.type = comparestatement->type;
				binarycompare.next = comparestatement->next;
				binarycompare.starttext = comparestatement->starttext;
				binarycompare.endtext = comparestatement->endtext;
				CPYMEM(binarycompare.compare, comparestatement->compare, sizeof(binarycompare.compare));
				binarycompare.left = comparestatement->left;
				binarycompare.right = comparestatement->right;
				codeblockwrite(&buf, &binarycompare, sizeof(struct BinaryCompareStatementS));
			} break;
			case BINDINGSTATEMENT:
			{
				struct BinaryBindingStatementS binarybinding;
				BindingStatement funcstatement= (BindingStatement)code->statement[j];
				binarybinding.type = funcstatement->type;
				binarybinding.next = funcstatement->next;
				binarybinding.starttext = funcstatement->starttext;
				binarybinding.endtext = funcstatement->endtext;
				binarybinding.left = funcstatement->left;
				binarybinding.varlength= (funcstatement->varname == NULL) ? 0 : strlen(funcstatement->varname);
				binarybinding.funclength=(funcstatement->func==NULL)?0:strlen(funcstatement->func);
				binarybinding.numparam = funcstatement->numparam;
				codeblockwrite(&buf, &binarybinding, sizeof(struct BinaryBindingStatementS));
				if (funcstatement->varname != NULL) {
					codeblockwrite(&buf, funcstatement->varname, binarybinding.varlength);
				}
				if (funcstatement->func != NULL) {
					codeblockwrite(&buf, funcstatement->func, binarybinding.funclength);
				}
				for (int i = 0; i < funcstatement->numparam;i++)
				{
					uint32_t p = funcstatement->param[i];
					codeblockwrite(&buf, &p, sizeof(uint32_t));
				}
			} break;
			case ASSIGNSTATEMENT:
			{
				struct BinaryAssignmentStatementS binaryassign;
				AssignmentStatement assignstatement = (AssignmentStatement)code->statement[j];
				binaryassign.type = assignstatement->type;
				binaryassign.next = assignstatement->next;
				binaryassign.starttext = assignstatement->starttext;
				binaryassign.endtext = assignstatement->endtext;
				binaryassign.left = assignstatement->left;
				binaryassign.right = assignstatement->right;
				codeblockwrite(&buf, &binaryassign, sizeof(struct BinaryAssignmentStatementS));
			} break;
			case IFSTATEMENT:
			{
				struct BinaryIfStatementS binaryif;
				IfStatement ifstatement = (IfStatement)code->statement[j];
				binaryif.type = ifstatement->type;
				binaryif.next = ifstatement->next;
				binaryif.starttext = ifstatement->starttext;
				binaryif.endtext = ifstatement->endtext;
				binaryif.logic = ifstatement->logic;
				binaryif.ifthen = ifstatement->ifthen;
				codeblockwrite(&buf, &binaryif, sizeof(struct BinaryIfStatementS));
			} break;
			/*case FORSTATEMENT:
			{
				struct BinaryForStatementS binaryfor;
				ForStatement forstatement = (ForStatement)code->statement[j];
				binaryfor.type = forstatement->type;
				binaryfor.next = forstatement->next;
				binaryfor.starttext = forstatement->starttext;
				binaryfor.endtext = forstatement->endtext;
				binaryfor.begin = forstatement->begin;
				binaryfor.logic = forstatement->logic;
				binaryfor.step = forstatement->step;
				binaryfor.dofor = forstatement->dofor;
				codeblockwrite(&buf, &binaryfor, sizeof(struct BinaryForStatementS));
			} break;*/
			case WHILESTATEMENT:
			{
				struct BinaryWhileStatementS binarywhile;
				WhileStatement whilestatement = (WhileStatement)code->statement[j];
				binarywhile.type = whilestatement->type;
				binarywhile.next = whilestatement->next;
				binarywhile.starttext = whilestatement->starttext;
				binarywhile.endtext = whilestatement->endtext;
				binarywhile.dowhile = whilestatement->dowhile;
				binarywhile.logic = whilestatement->logic;
				codeblockwrite(&buf, &binarywhile, sizeof(struct BinaryWhileStatementS));
			} break;
			case CALLSTATEMENT: {
				struct BinaryCallStatementS binarycall;
				CallStatement callstatement = (CallStatement)code->statement[j];
				binarycall.type = callstatement->type;
				binarycall.next = callstatement->next;
				binarycall.starttext = callstatement->starttext;
				binarycall.endtext = callstatement->endtext;
				binarycall.go = callstatement->go;
				binarycall.stack = callstatement->stack;
				binarycall.numparam = callstatement->numparam;
				codeblockwrite(&buf, &binarycall, sizeof(struct BinaryCallStatementS));
				for (int i = 0; i < callstatement->numparam; i++)
				{
					uint32_t p = callstatement->param[i];
					codeblockwrite(&buf, &p, sizeof(uint32_t));
				}
			} break;
			case RETURNSTATEMENT: {
				struct BinaryReturnStatementS binaryreturn;
				ReturnStatement returnstatement = (ReturnStatement)code->statement[j];
				binaryreturn.type = returnstatement->type;
				binaryreturn.next = returnstatement->next;
				binaryreturn.starttext = returnstatement->starttext;
				binaryreturn.endtext = returnstatement->endtext;
				binaryreturn.ret = returnstatement->ret;
				binaryreturn.stack = returnstatement->stack;
				codeblockwrite(&buf, &binaryreturn, sizeof(struct BinaryReturnStatementS));
			} break;
		}
	}
	//write source code
	if (code->sourcecode != NULL) {
		codeblockwrite(&buf,code->sourcecode, strlen(code->sourcecode));
	}
	*size = buf.index;
	return buf.buffer;
}
