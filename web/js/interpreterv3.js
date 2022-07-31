/**
 * non blocking version
 * 
 * 31/07/22 for loop
 * 28/07/22 calling & return
 * 25/07/22 remove if 
 */

/********* STATEMENT TYPE *************/
const ASSIGNSTATEMENT = 1;
const IFSTATEMENT = 2;
const CONSTANTSTATEMENT = 3;
const LOGICSTATEMENT = 4;
const COMPARESTATEMENT = 5;
const EXPRESSIONSTATEMENT = 6;
const BINDINGSTATEMENT = 7;
const VARIABLESTATEMENT = 8;
const WHILESTATEMENT = 9;
const CALLSTATEMENT = 11;
const RETURNSTATEMENT = 12;

/********* VARIABLE TYPE *************/
const GLOBALVAR = 1;
const LOCALVAR = 2;
const STATICVARPROPERTY = 1;
const DYNAMICVARPROPERTY = 2;

const STOP_STATE = 0;
const PLAY_STATE = 1;
const PAUSE_STATE = 2;

const DEBUG = 0;

/********* code block & result *****************/
var pauseFlag=STOP_STATE;
var cInterval;
var c=null;
var r=null;

function initValue(value) {
	var p = { name: "", value: value };
	var v = { numproperties: 1, properties: [] };
	v.properties[0] = p;
	v.properties[0].value = value;
	return v;
}

function initVariable() {
	var p = { name: "", value: "" };
	var v = { numproperties: 1, properties: [] };
	v.properties[0] = p;
	v.properties[0].value = "";
	return v;
}

function setVariable(v, propertie, value) {	//clear first 
	var i = 0;
	var len = propertie.length;
	while (i < v.numproperties) {
		var s = v.properties[i];
		if ((len == 0) || ((s.name === propertie) || (s.name.substring(0, len + 1) == propertie + '.'))) {
			if (v.numproperties > 1) {
				v.numproperties--;
				v.properties[i] = v.properties[v.numproperties];
			}
			else { //left only one 
				v.properties[0].name = "";
				v.properties[0].value = "";
				i = v.numproperties;
			}
		}
		else i++;
	}
	//then merge or replace
	for (var i = 0; i < value.numproperties; i++) {
		var s = value.properties[i];
		p = propertie + (((propertie.length > 0) && (s.name.length > 0)) ? "." : "") + s.name;
		var found = -1;
		for (var j = 0; j < v.numproperties; j++) {
			if (v.properties[j].name == p) { found = j; }
		}
		if (found == -1) {
			v.properties[v.numproperties]={name: p,value:s.value};
			v.numproperties++;
		}
		else {
			v.properties[found].value = s.value;
		}
	}
	return v;
}

function getVariable(v, property) {
	var res = { numproperties: 0, properties: [] };
	for (var i = 0; i < v.numproperties; i++) {
		var s = v.properties[i];
		if ((property.length==0) || (s.name.substr(0,property.length) == property)) {
			var n=s.name.substr(property.length);
			if ((n.length>0) && (n[0]=='.')) n=n.substr(1);
			res.properties[res.numproperties] = { name: n, value: s.value };
			res.numproperties++;
		}
	}
	return res;
}

function variableArray(vid) {	//expand the array
	if (c.var[vid] == null) {
		c.var[vid] = initVariable();
	}
}

//get the property 
function getVariableProperty(v,i) {
	if (v.properties[i].type === STATICVARPROPERTY) {
		c.return=initValue(v.properties[i].static);
	}
	else {
		//dynamic array  
		c.statestack[c.statestackindex]=c.state;
		c.statestackindex++;
		c.index=v.properties[i].dynamic;
		c.state=null;
	}
}

/**
* run each step
* @return result
*/
function runstep() {
		if (c.index==0) { 
			console.log("end"); 
			clearInterval(cInterval); 
			postMessage({type:"stop"});
			return; 
		}	//end
		var s = c.statement[c.index];
		var type = s.type;
		if (c.state===null) {			
		 console.log("index "+c.index);
		 console.log("type "+type);
		}
		switch (type) {
			case CONSTANTSTATEMENT:
				{	//tested 19/07/2022
					c.return=s.value;
				} break;
			case VARIABLESTATEMENT: 
				{	//tested 19/07/2022
					if (c.state==null) { 
						c.state={index:c.index,s:0,vsindex:0,properties:""};	//get property state
						c.return=null;
					} 
					if (c.state.s==0) {	
						if (c.state.properties!="") c.state.properties=c.state.properties+".";
						if (c.return!=null) c.state.properties=c.state.properties+c.return.properties[0].value;
						if (c.state.vsindex<s.properties.length) {	
							c.state.vsindex++;						
							getVariableProperty(s,c.state.vsindex-1);
							return ;
						} 
					} 
					var vartype = s.vartype;
					var vid = (vartype == GLOBALVAR) ? s.vid : s.vid + c.currentvar;
					var property = c.state.properties; 
					variableArray(vid);
					var res = getVariable(c.var[vid], property);
					var result = (res == null) ? initValue("") : res;
					c.return=result;
				} break;
			case LOGICSTATEMENT:
				{	//tested 10/07/2022
					if (c.state==null) { 
						c.state={index:c.index,s:0,left:null,right:null};	//first state	
					}
					if (c.state.s==0) {
						c.state.s=1;
						c.statestack[c.statestackindex]=c.state;
						c.statestackindex++;
						c.index=s.left;
						c.state=null;
						return ;
					} else if (c.state.s==1) {
						c.state.left=c.return;
						c.state.s=2;
						if (s.logic!='!') {
							c.statestack[c.statestackindex]=c.state;
							c.statestackindex++;
							c.index=s.right;
							c.state=null;
							return ;	
						} else 
							c.state.right=initValue("");
					} else if (c.state.s==2) {
						c.state.right=c.return;
					} 
					var left=c.state.left;		//var left = runstep(s.left);
					var right=c.state.right;	//var right = (s.logic != '!') ? runstep(s.right) : initValue("");
					var result = "";
					if (s.logic == '&') {
						result = ((left.properties[0].value == "1") && (right.properties[0].value == "1")) ? "1" : "0";
					}
					else if (s.logic == '|') {
						result = ((left.properties[0].value == "1") || (right.properties[0].value == "1")) ? "1" : "0";
					}
					else if (s.logic == '!') {
						if (left.properties[0].value == "0") result = "1";
						else if (left.properties[0].value == "1") result = "0";
					}
					c.return=initValue(result);
				} break;
			case COMPARESTATEMENT:
				{	//tested 19/7/2022	
					if (c.state==null) { 
						c.state={index:c.index,s:0,left:null,right:null};	//first state	
					}
					if (c.state.s==0) {
						c.state.s=1;
						c.statestack[c.statestackindex]=c.state;
						c.statestackindex++;
						c.index=s.left;
						c.state=null;
						return ;
					} else if (c.state.s==1) {
						c.state.left=c.return;
						c.state.s=2;
						c.statestack[c.statestackindex]=c.state;
						c.statestackindex++;
						c.index=s.right;
						c.state=null;
						return ;
					} else if (c.state.s==2) {
						c.state.right=c.return;
					} 
					var left=c.state.left;			//var left = runstep(s.left);
					var right = c.state.right;		//runstep(s.right);
					var result = "";
					var leftnum = parseFloat(left.properties[0].value);
					var rightnum = parseFloat(right.properties[0].value);	
					if ((!isNaN(leftnum)) && (!isNaN(rightnum))) {	 //compare number
						if (s.compare == '=') { result = (Math.abs(leftnum - rightnum) < 0.0001) ? "1" : "0"; }
						else if (s.compare[0] == '!') {  result = (Math.abs(leftnum - rightnum) > 0.0001) ? "1" : "0"; }
						else if (s.compare == '>') { result = (leftnum > rightnum) ? "1" : "0"; }
						else if (s.compare == '>=') { result = (leftnum >= rightnum) ? "1" : "0"; }
						else if (s.compare == '<') { result = (leftnum < rightnum) ? "1" : "0"; }
						else if (s.compare == '<=') { result = (leftnum <= rightnum) ? "1" : "0"; }
					}
					else {
						leftstr=left.properties[0].value;
						rightstr=right.properties[0].value;
						if (s.compare == '=') { result = (leftstr == rightstr) ? "1" : "0"; }
						else if (s.compare[0] == '!') {  result = (leftstr != rightstr) ? "1" : "0"; }
						else if (s.compare == '>') { result = (leftstr > rightstr) ? "1" : "0"; }
						else if (s.compare == '>=') { result = (leftstr >= rightstr) ? "1" : "0"; }
						else if (s.compare == '<') { result = (leftstr < rightstr) ? "1" : "0"; }
						else if (s.compare == '<=') { result = (leftstr <= rightstr) ? "1" : "0"; }
					}
					c.return=initValue(result);
				} break;
			case EXPRESSIONSTATEMENT:
				{	//tested 19/07/2022
					if (c.state==null) { 
						c.state={index:c.index,s:0,left:null,right:null};	//first state	
					}
					if (c.state.s==0) {
						c.state.s=1;
						c.statestack[c.statestackindex]=c.state;
						c.statestackindex++;
						c.index=s.left;
						c.state=null;
						return ;
					} else if (c.state.s==1) {
						c.state.left=c.return;
						c.state.s=2;
						c.statestack[c.statestackindex]=c.state;
						c.statestackindex++;
						c.index=s.right;
						c.state=null;
						return ;
					} else if (c.state.s==2) {
						c.state.right=c.return;
					} 
					var left = c.state.left;//runstep(s.left);
					var right = c.state.right;//runstep(s.right);
					var result = "";
					var leftnum = parseFloat(left.properties[0].value);
					var rightnum = parseFloat(right.properties[0].value);
					if ((leftnum != NaN) && (rightnum != NaN)) {
						switch (s.op) {
							case '+': { result = leftnum + rightnum; } break;
							case '-': { result = leftnum - rightnum; } break;
							case '*': { result = leftnum * rightnum; } break;
							case '/': { result = leftnum / rightnum; } break;
							case '%': { result = (Math.round(leftnum) % Math.round(rightnum)); } break;
							case '&': { result = (Math.round(leftnum) & Math.round(rightnum)); } break;
							case '|': { result = (Math.round(leftnum) | Math.round(rightnum)); } break;
							case '^': { result = (leftnum ^ rightnum); } break;
						}
					}
					else {
						result = (s.op == '+') ? left + right : left;
					}
					c.return=initValue(result);
				} break;
			case BINDINGSTATEMENT:
				{	//tested 19/07/2022
					if (c.state==null) { 
						c.state={index:c.index,s:0,paramindex:-1,param:new Array(s.param.length),vsindex:0,properties:""};	//first state	
					}
					if (c.state.s==0) {
						//param
						if (c.state.paramindex>=0) {
							c.state.param[c.state.paramindex]=c.return;
						}
						c.state.paramindex++;
						if (c.state.paramindex<=s.param.length-1) {
						    c.statestack[c.statestackindex]=c.state;
						    c.statestackindex++;
							c.index=s.param[c.state.paramindex];
							c.state=null;
							return ;
						} else {
							c.state.s=1;
							c.return=initValue("");	//dummy		
						}
					} 
					if (c.state.s==1) {
						//prop
						if (s.left>0) {
							var vs=c.statement[s.left];
							if (c.state.properties!="") c.state.properties=c.state.properties+".";
							 c.state.properties=c.state.properties+c.return.properties[0].value;
							if (c.state.vsindex<vs.properties.length) {							
								c.state.vsindex++;		//in case of dynamic
								getVariableProperty(vs,c.state.vsindex-1);
								return ;
							} else c.state.s=2;
						} else c.state.s=2;	
					}
					if (c.state.s==2) {
						var prop=c.state.properties;
						var vs = c.statement[s.left];
						c.state.s=3;
						c.return=null;
						c.callback((s.left==0) ? null : ((vs.vartype == GLOBALVAR) ? c.var[vs.vid] : c.var[vs.vid + c.currentvar]), s.varname, prop, s.funcname, c.state.param, s.param.length, c.id);
					}
					if ((c.state.s==3) && (c.return==null)) { return ; }	//wait for result
					
				} break;
			case ASSIGNSTATEMENT:
				{	//tested 19/07/2022
					if (c.state==null) { 
						c.state={index:c.index,s:0,left:null,right:null,vsindex:0,properties:""};	//first state	
					}
					if (c.state.s==0) {
						c.state.s=1;
						c.statestack[c.statestackindex]=c.state;
						c.statestackindex++;
						c.index=s.right;
						c.state=null;
						return ;
					} else if (c.state.s==1) {
						c.state.s=2;
						c.state.right=c.return;
						c.state.left = c.statement[s.left];
						c.return=initValue("");
					} 
					if (c.state.s==2) {
						var left=c.state.left;
						if (left.type == VARIABLESTATEMENT) {
							if (c.state.properties!="") c.state.properties=c.state.properties+".";
							 c.state.properties=c.state.properties+c.return.properties[0].value;
							if (c.state.vsindex<left.properties.length) {			
								c.state.vsindex++;		//in case of dynamic
								getVariableProperty(c.state.left,c.state.vsindex-1);
								return ;
							} 
						}
					}
					var right = c.state.right;				
					var left = c.state.left;
					if (left.type == VARIABLESTATEMENT) {
						var vid = (left.vartype == GLOBALVAR) ?left.vid : left.vid + c.currentvar;
						variableArray(vid);
						setVariable(c.var[vid], c.state.properties, right);
					}
				} break;
			case IFSTATEMENT:
				{	//tested 19/07/2022
					if (c.state==null) { 
						c.state={index:c.index,s:0,logic:null};	//first state	
					}	
					if (s.logic == 0) {		//else
						c.index=s.ifthen;
						c.state=null;
						c.return=null;
						return ;
					} else {	
						if (c.state.s==0) {			
							c.state.s=1;
							c.statestack[c.statestackindex]=c.state;
							c.statestackindex++;
							c.index=s.logic;				//var left = runstep(s.logic);
							c.state=null;
							return ;
						}
						if (c.return.properties[0].value == "1") {
							c.index=s.ifthen;
							c.state=null;
							c.return=null;
							return ;
						}
						else {
							c.index=s.next;
							c.state=null;
							c.return=null;
							return ;
						}
					}
				} break;
			case WHILESTATEMENT:
				{	//tested 10/07/2022
					if (c.state==null) { 
						c.state={index:c.index,s:0};		
					}	
					if (c.state.s==0) {	//go for logic
							c.state.s=1;
							c.statestack[c.statestackindex]=c.state;
							c.statestackindex++;
							c.index=s.logic;				//var left = runstep(s.logic);
							c.state=null;
							return ;
					} else if (c.state.s==1) {
						if (c.return.properties[0].value==="1")
						{
							c.state.s=0;
							c.statestack[c.statestackindex]=c.state;
							c.statestackindex++;
							c.index=s.dowhile;				//var left = runstep(s.while);
							c.state=null;
							return ;
						} //then next
					}
				} break;
			case CALLSTATEMENT:
				{	//tested 31/07/2022
					if (c.state==null) { 
						if (c.stackindex>c.maxstack) {
							c.return=initValue("");
						} else {
							c.state={index:c.index,s:0,param:0};	//first state
						}
					}
					if (c.state!=null) {
						if (c.state.s==0) {
							//save state
							c.state.s=1;
							c.varstack[c.stackindex] = c.currentvar;
							c.stackindex++;
							if (s.stack>0) {
								c.statestack[c.statestackindex]=c.state;
								c.statestackindex++;
								c.index=s.param[0];				
								c.state=null;
								return ;
							} else 
								c.state.s=2;
						} else if (c.state.s==1) {
							c.var[c.numvar + c.state.param]=c.return;
							c.state.param++;
							if (c.state.param<s.stack) {
								c.statestack[c.statestackindex]=c.state;
								c.statestackindex++;
								c.index=s.param[c.state.param];				
								c.state=null;
								return ;
							} else {
								c.state.s=2;
								c.currentvar = c.numvar;	//begin with param
								c.numvar += s.stack;		//increase later
							}
						} 
						if (c.state.s==2) {
							c.state.s=3;		//end
							c.statestack[c.statestackindex]=c.state;
							c.statestackindex++;
							c.index=s.go;			
							c.state=null;
							return ;
						}
					}
				} break;
			case RETURNSTATEMENT:
				{	//test 31/07/2022
					if (c.state==null) { 
						c.state={index:c.index,s:0};	//first state	
					}
					if (c.state.s==0) {
						//load state
						c.state.s=1;
						c.stackindex--;
						c.currentvar = c.varstack[c.stackindex];
						c.numvar -= s.stack;
						if (s.ret!=0) {
							c.state.s=1;
							c.statestack[c.statestackindex]=c.state;
							c.statestackindex++;
							c.index=s.ret;				
							c.state=null;
							return ;
						} else {
							c.return=initValue("");		//default
						}
					}
				} break;
		}
		if (s.next!==0) {					//next
			c.index=s.next;
			c.state=null;
			c.return=null;
		} else if (c.statestackindex>0) { 	//return
			c.state=c.statestack[c.statestackindex-1];
			c.index=c.state.index;
			c.statestackindex--;
		} else {							//end
			c.index=0;
			c.state=null;
		}
}

/**
* step run
*/
function runcodeblock() {
	c.index=c.start;
	c.state=null;
	c.statestackindex=0;
	pauseFlag=PLAY_STATE;
	cInterval =setInterval(runstep,1);
}

function pausecodeblock() {
	if (pauseFlag==PLAY_STATE) {
		pauseFlag=PAUSE_STATE;
		clearInterval(cInterval);
	}
}

function resumecodeblock() {
	if (pauseFlag==PAUSE_STATE) {
		pauseFlag=PLAY_STATE;
		cInterval =setInterval(runstep,1);
	}
}

function getuarray32(uarray, i) {
	return (uarray[i + 3] << 24) + (uarray[i + 2] << 16) + (uarray[i + 1] << 8) + (uarray[i + 0]);
}

function getuarray16(uarray, i) {
	return (uarray[i + 1] << 8) + (uarray[i + 0]);
}

function getuarraystr(uarray, index, len) {
	var result = "";
	for (var j = 0; j < len; j++) {
		if (uarray[j + index] != 0) result += String.fromCharCode(uarray[j + index]);
	}
	return result;
}

/**
* main load
*/
function loadbinary(uarray) {
	var i = 0;
	if (uarray.length < 25) return null;
	/**
		 uint32_t version;
		 uint32_t numstatement;
		 uint32_t start;		//start point
		 uint32_t numvar;	//num of global variable
		 uint32_t codesize;  //0=not available
	 */
	var version = getuarray32(uarray, 0);
	var numstatement = getuarray32(uarray, 4);
	var start = getuarray32(uarray, 8);
	var numvar = getuarray32(uarray, 12);
	var codesize = getuarray32(uarray, 16);
	//console.log("version " + version);
	//console.log("num statement " + numstatement);
	//console.log("num global var " + numvar);
	//console.log("start " + start);
	//console.log("source code size " + codesize);
	i += 5 * 4;

	var statement = new Array(numstatement);
	for (var k = 1; k < numstatement; k++) {
		var type = uarray[i];
		var next = getuarray32(uarray, i + 1);
		var starttext = getuarray32(uarray, i + 5);
		var endtext = getuarray32(uarray, i + 9);
		i += 1 + (3 * 4);
		var s;
		//console.log(k+" "+type+" "+next)
		switch (type) {
			case CONSTANTSTATEMENT: {
				s = { type: type, next: next, start: starttext, end: endtext, value: {} };
				/*		
					uint32_t numproperties;
				*/
				var numproperties = getuarray32(uarray, i);
				i += 4;
				var pa = new Array(numproperties);
				for (var j = 0; j < numproperties; j++) {
					/*
						char vartype;				//STRINGVAR,ARRAYVAR,STRUCTUREVAR,ARRAYOFSTRUCTUREVAR
						uint32_t namelength;		//name of property (can be NULL)
						uint32_t valuelength;		//array statement, can be 0
						//name..
						//value...
					*/
					pa[j] = { name: "", value: "" };
					var namelen = getuarray32(uarray, i + 1);
					var valuelen = getuarray32(uarray, i + 5);
					i += 9;
					pa[j].name = getuarraystr(uarray, i, namelen);
					i += namelen;
					pa[j].value = getuarraystr(uarray, i, valuelen);
					i += valuelen;
				}
				s.value = { numproperties: numproperties, properties: pa };
			} break;
			case VARIABLESTATEMENT: {
				s = { type: type, next: next, start: starttext, end: endtext, vartype: 0, vid: 0, properties: [] };
				/*	struct BinaryDynamicVariablePropertiesStatementS {
						char type;				//DYNAMIC
						uint32_t next;
					} PACKED;
					struct BinaryStaticVariablePropertiesStatementS {
						char type;				//STATIC 
						uint32_t namelength;
						//name...
					} PACKED;
					uint32_t vartype;		//GLOBALVAR, LOCALVAR
					uint32_t vid;
					uint32_t numproperties;
					//properties...
				*/
				s.vartype = getuarray32(uarray, i);
				s.vid = getuarray32(uarray, i + 4);
				var numproperties = getuarray32(uarray, i + 8);
				i += 12;
				var pa = new Array(numproperties);
				for (var j = 0; j < numproperties; j++) {
					if (uarray[i] == STATICVARPROPERTY) {
						p = { type: uarray[i], static: "" };
						var namelen = getuarray32(uarray, i + 1);
						p.static = getuarraystr(uarray, i + 5, namelen);
						i += 5 + namelen;
					} else if (uarray[i] == DYNAMICVARPROPERTY) {
						p = { type: uarray[i], dynamic: 0 };
						p.dynamic = getuarray32(uarray, i + 1);
						i += 5;
					}
					pa[j] = p;
				}
				s.properties = pa;
			} break;
			case BINDINGSTATEMENT: {
				s = { type: type, next: next, start: starttext, end: endtext, left: 0, varname: "", funcname: "", param: [] };
				/*	uint32_t left;				//variable statement
					  uint32_t varlength;			//variable name
					uint32_t funclength;		//function name
					uint32_t numparam;
					//variable name
					//function name
					//param ...uint32_t
				*/
				s.left = getuarray32(uarray, i);
				var varlen = getuarray32(uarray, i + 4);
				var funclen = getuarray32(uarray, i + 8);
				var numparam = getuarray32(uarray, i + 12);
				i += 16;
				if (varlen > 0) { s.varname = getuarraystr(uarray, i, varlen); i += varlen; }
				if (funclen > 0) { s.funcname = getuarraystr(uarray, i, funclen); i += funclen; }
				var param = new Array(numparam);
				for (var j = 0; j < numparam; j++) {
					param[j] = getuarray32(uarray, i);
					i += 4;
				}
				s.param = param;
			} break;
			case CALLSTATEMENT: {
				s = { type: type, next: next, start: starttext, end: endtext, go: 0, stack: 0, param: [] };
				/*				
					uint32_t go;
					uint32_t stack;
					uint32_t numparam;
					//param ...
				*/
				s.go = getuarray32(uarray, i);
				s.stack = getuarray32(uarray, i + 8);
				var numparam = getuarray32(uarray, i + 4);
				i += 12;
				var param = new Array(numparam);
				for (var j = 0; j < numparam; j++) {
					param[j] = getuarray32(uarray, i);
					i += 4;
				}
				s.param = param;
			} break;
			case RETURNSTATEMENT: {
				s = { type: type, next: next, start: starttext, end: endtext, stack: 0, ret: 0 };
				/*	uint32_t stack;				//decrease stack
					uint32_t ret;				//variable or constant to return
				*/
				s.stack = getuarray32(uarray, i);
				s.ret = getuarray32(uarray, i + 4);
				i += 8;
			} break;
			case COMPARESTATEMENT: {
				s = { type: type, next: next, start: starttext, end: endtext, compare: 0, left: 0, right: 0 };
				/*	char compare[4];			//== > >= < <=
					uint32_t left;				//left statement
					uint32_t right;				//right statement
				*/
				s.compare = String.fromCharCode(uarray[i]);
				if (uarray[i + 1] != 0) s.compare += String.fromCharCode(uarray[i + 1]);
				s.left = getuarray32(uarray, i + 4);
				s.right = getuarray32(uarray, i + 8);
				i += 12;
			} break;
			case LOGICSTATEMENT: {
				s = { type: type, next: next, start: starttext, end: endtext, logic: 0, left: 0, right: 0 };
				/*	char logic;					//& | !
					uint32_t left;
					uint32_t right;
				*/
				s.logic = String.fromCharCode(uarray[i]);
				s.left = getuarray32(uarray, i + 1);
				s.right = getuarray32(uarray, i + 5);
				i += 9;
			} break;
			case EXPRESSIONSTATEMENT: {
				s = { type: type, next: next, start: starttext, end: endtext, op: 0, left: 0, right: 0 };
				/*	char op;					//+ - * / % & | xor
					uint32_t left;
					uint32_t right;
				*/
				s.op = String.fromCharCode(uarray[i]);
				s.left = getuarray32(uarray, i + 1);
				s.right = getuarray32(uarray, i + 5);
				i += 9;
			} break;
			case ASSIGNSTATEMENT: {
				s = { type: type, next: next, start: starttext, end: endtext, left: 0, right: 0 };
				/*	uint32_t left;		//variable
					uint32_t right;		//expression 
				*/
				s.left = getuarray32(uarray, i);
				s.right = getuarray32(uarray, i + 4);
				i += 8;
			} break;
			case IFSTATEMENT: {
				s = { type: type, next: next, start: starttext, end: endtext, logic: 0, ifthen: 0 };
				/*	uint32_t logic;			
					uint32_t ifthen;
				*/
				s.logic = getuarray32(uarray, i);
				s.ifthen = getuarray32(uarray, i + 4);
				i += 8;
			} break;
			case WHILESTATEMENT: {
				s = { type: type, next: next, start: starttext, end: endtext, logic: 0, dowhile: 0 };
				/*	uint32_t logic;
					uint32_t dowhile;
				*/
				s.logic = getuarray32(uarray, i);
				s.dowhile = getuarray32(uarray, i + 4);
				i += 8;
			} break;
		}
		//console.log(s);
		statement[k] = s;
	}
	//return in global
	/* 
	 int type;					//always 0
	 int start;			    	//index start always 1

	 any return;				//the return

	 int currentindex;			//current index
	
	 int maxstack;				//max stack level
	 int statestackindex;		//state stack index
	 int* statestack;			//state stack

	 int stackindex;			//stack index for return stack and var stack
	 int currentvar;			//current stack
	 int* varstack;				//num of var stack
	 
	 int numvar;				//num of var array 
	 Variable* var;				//variable stack
	 int numglobalvar;			//num of global variable
	 int numstatement;
	 Statement* statement;		//all statement
	 void *id;			    	//for callback
	 Binding *func;
	 char* sourcecode;			//original
	 */
	c = { start: start, 
		index: start, 
		state: null, 
		maxstack: 10000, 
		
		statestackindex: 0, 
		statestack: [], 
		
		stackindex: 0, 
		currentvar: 0, 
		varstack: [],

		numvar: 0, 
		var: [], 
		numglobalvar: 0, 
		numstatement: numstatement, 
		statement: statement, 
		code: (codesize != 0) ? getuarraystr(uarray, i, codesize) : "", 
		id: 0, 
		callback: null }
	return c;
}

function workercallback(v,varname,propertie,funcname,param)
{
	postMessage({type:"callback",v:v,varname:varname,propertie:propertie,funcname:funcname,param:param});
}

addEventListener("message", event => {
	const t=event.data.type;
	if (t=="start") {
	 c=event.data.codeblock;
	 c.id=event.data.id;
	 c.callback=workercallback;
	 runcodeblock();
	} else 
	if (t=="pause") {
		pauseFlag=PAUSE_STATE;
		clearInterval(cInterval);
	} else 
	if (t=="resume") {
		pauseFlag=PLAY_STATE;
		cInterval =setInterval(runstep,1);
	} else 
	if (t=="return") {
		console.log("return");
		c.return=event.data.return;
	}
});
