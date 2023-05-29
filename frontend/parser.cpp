/* parser.cpp
*
*  Copyright (C) 2023-2026 Cunjing Ge & Minghao Liu & Fuqi Jia.
*
*  All rights reserved.
*/

#include "frontend/parser.hpp"
#include <queue>
#include <stack>
using namespace ismt;

/*
get symbol from buffer
*/
std::string Parser::get_symbol() {

	char *beg = bufptr;

	// first char was already scanned
	bufptr++;

	// while not eof	
	while (*bufptr != 0) {

		switch (scan_mode) {
		case SM_SYMBOL:
			if (isblank(*bufptr)) {

				// out of symbol mode by ' ' and \t
				std::string tmp_s(beg, bufptr - beg);

				// skip space
				bufptr++;
				scan_to_next_symbol();

				return tmp_s;

			}
			else if (*bufptr == '\n' || *bufptr == '\r' || *bufptr == '\v' || *bufptr == '\f') {
				line_number++;

				// out of symbol mode by '\n', '\r', '\v' and '\f'
				std::string tmp_s(beg, bufptr - beg);

				// skip space
				bufptr++;
				scan_to_next_symbol();

				return tmp_s;

			}
			else if (*bufptr == ';' || *bufptr == '|' || *bufptr == '"' || *bufptr == '(' || *bufptr == ')') {

				// out of symbol mode bu ';', '|', '(', and ')'
				std::string tmp_s(beg, bufptr - beg);
				return tmp_s;

			}
			break;

		case SM_COMP_SYM:

			if (*bufptr == '\n' || *bufptr == '\r' || *bufptr == '\v' || *bufptr == '\f') {
				line_number++;
			}
			else if (*bufptr == '|') {

				// out of complicated symbol mode
				bufptr++;
				std::string tmp_s(beg, bufptr - beg);

				// skip space
				scan_to_next_symbol();

				return tmp_s;

			}
			break;

		case SM_STRING:

			if (*bufptr == '\n' || *bufptr == '\r' || *bufptr == '\v' || *bufptr == '\f') {
				line_number++;
			}
			else if (*bufptr == '"') {

				// out of string mode
				bufptr++;
				std::string tmp_s(beg, bufptr - beg);

				// skip space
				scan_to_next_symbol();

				return tmp_s;

			}
			break;

		default:
			assert(false);
		}

		// go next char
		bufptr++;
	}

	err_unexp_eof();

	return NULL;
}

void Parser::scan_to_next_symbol() {

	// init scan mode
	scan_mode = SM_COMMON;

	// while not eof
	while (*bufptr != 0) {

		if (*bufptr == '\n' || *bufptr == '\r' || *bufptr == '\v' || *bufptr == '\f') {

			line_number++;

			// out of comment mode
			if (scan_mode == SM_COMMENT) scan_mode = SM_COMMON;

		}
		else if (scan_mode == SM_COMMON && !isblank(*bufptr)) {

			switch (*bufptr) {
			case ';':
				// encounter comment
				scan_mode = SM_COMMENT;
				break;
			case '|':
				// encounter next complicated symbol
				scan_mode = SM_COMP_SYM;
				return;
			case '"':
				// encounter next string
				scan_mode = SM_STRING;
				return;
			default:
				// encounter next symbol
				scan_mode = SM_SYMBOL;
				return;
			}

		}

		// go next char
		bufptr++;
	}

}

void Parser::parse_lpar() {
	if (*bufptr == '(') {
		bufptr++;
		scan_to_next_symbol();
	}
	else {
		err_sym_mis("(", line_number);
	}
}

void Parser::parse_rpar() {
	if (*bufptr == ')') {
		bufptr++;
		scan_to_next_symbol();
	}
	else {
		err_sym_mis(")", line_number);
	}
}

void Parser::skip_to_rpar() {

	// skip to next right parenthesis with same depth	
	scan_mode = SM_COMMON;
	unsigned int level = 0;

	while (*bufptr != 0) {

		if (*bufptr == '\n' || *bufptr == '\r' || *bufptr == '\v' || *bufptr == '\f') {
			// new line
			line_number++;
			if (scan_mode == SM_COMMENT)
				scan_mode = SM_COMMON;
		}
		else if (scan_mode == SM_COMMON) {

			if (*bufptr == '(') level++;
			else if (*bufptr == ')') {
				if (level == 0) return;
				else level--;
			}
			else if (*bufptr == ';')
				scan_mode = SM_COMMENT;
			else if (*bufptr == '|')
				scan_mode = SM_COMP_SYM;
			else if (*bufptr == '"')
				scan_mode = SM_STRING;

		}
		else if (scan_mode == SM_COMP_SYM && *bufptr == '|')
			scan_mode = SM_COMMON;
		else if (scan_mode == SM_STRING && *bufptr == '"')
			scan_mode = SM_COMMON;

		// go to next char
		bufptr++;
	}

}


void Parser::parse_smtlib2_file(const std::string filename) {

	/*
	load file
	*/
	std::ifstream fin(filename, std::ifstream::binary);

	if (!fin) {
		err_open_file(filename);
	}

	fin.seekg(0, std::ios::end);
	buflen = (long)fin.tellg();
	fin.seekg(0, std::ios::beg);

	buffer = new char[buflen + 1];
	fin.read(buffer, buflen);
	buffer[buflen] = 0;

	fin.close();


	/*
	parse command
	*/
	bufptr = buffer;
	if (buflen > 0) line_number = 1;

	// skip to first symbol;
	scan_to_next_symbol();

	while (*bufptr) {
		parse_lpar();
		if (parse_command() == CT_EXIT) break;
		parse_rpar();
	}

	// parse finished
	key_map.clear();
	delete[]buffer;

}

CMD_TYPE Parser::parse_command() {

	unsigned int command_ln = line_number;
	std::string command = get_symbol();

	// (assert <expr>)
	if (command == "assert") {

		// parse expression
		dagc * assert_expr = parse_expr();

		// insert into dag
		dag.assert_list.emplace_back(assert_expr);

		return CT_ASSERT;

	}

	// (check-sat)
	if (command == "check-sat") {
		dag.check_sat = true;
		skip_to_rpar();
		return CT_CHECK_SAT;
	}

	if (command == "check-sat-assuming") {
		// ignore
		warn_cmd_nsup(command, command_ln);
		skip_to_rpar();
		return CT_CHECK_SAT_ASSUMING;
	}

	// (declare-const <symbol> <sort>)
	if (command == "declare-const") {

		// get name
		unsigned int name_ln = line_number;
		std::string name = get_symbol();

		// get returned type
		dagc * res = nullptr;
		unsigned int type_ln = line_number;
		std::string type = get_symbol();
		if (type == "Bool") {
			res = mk_bool_decl(name);
		}
		else if (type == "Int") {
			res = mk_var_decl(name, true);
		}
		else if (type == "Real") {
			res = mk_var_decl(name, false);
		}
		else {
			err_unkwn_sym(type, type_ln);
		}

		// multiple declarations
		if (res->iserr()) err_all(res, name, name_ln);

		return CT_DECLARE_CONST;

	}

	// (declare <symbol> () <sort>)
	if (command == "declare-fun") {

		// get name
		unsigned int name_ln = line_number;
		std::string name = get_symbol();

		parse_lpar();
		parse_rpar();

		//get returned type
		dagc * res = nullptr;
		unsigned int type_ln = line_number;
		std::string type = get_symbol();
		if (type == "Bool") {
			res = mk_bool_decl(name);
		}
		else if (type == "Int") {
			res = mk_var_decl(name, true);
		}
		else if (type == "Real") {
			res = mk_var_decl(name, false);
		}
		else {
			err_unkwn_sym(type, type_ln);
		}

		//multiple declarations
		if (res->iserr()) err_all(res, name, name_ln);

		return CT_DECLARE_FUN;

	}

	if (command == "declare-sort") {
		//ignore
		warn_cmd_nsup(command, command_ln);
		skip_to_rpar();
		return CT_DECLARE_SORT;
	}

	//(define-fun <symbol> () <sort> <expr>) ->
	//(define-fun <symbol> ((x Int)) <sort> <expr>)
	if (command == "define-fun") {

		//get name
		// unsigned int name_ln = line_number;
		std::string name = get_symbol();

		//parse ((x Int))
		parse_lpar();
		std::vector<dagc* > params;
		std::vector<std::string> key_list;
		int cnt = 0; // the index of paramters.
		while(*bufptr!=')'){ // there are still (x Int) left.
			parse_lpar();
			std::string pname = get_symbol();
			std::string ptype = get_symbol();
			key_list.emplace_back(pname);
			dagc* expr = NULL;
			// insert to key_map
			// TODO, no parent because fun will be eliminated.
			if(ptype=="Int"){
				expr = new dagc(NT_FUN_PARAM_INT, cnt++, pname);
			}
			else{
				expr = new dagc(NT_FUN_PARAM_BOOL, cnt++, pname);
			}
			std::pair<boost::unordered_map<std::string, dagc *>::iterator, bool>
				p = key_map.insert(std::pair<std::string, dagc *>(pname, expr));

			if (!p.second) {
				// multiple declarations
				delete expr;
				mk_err(ERR_MUL_DECL);
				assert(false);
			}
			else {
				params.emplace_back(expr);
				dag.funp_list.emplace_back(expr);
			}
			parse_rpar();
		}
		
		parse_rpar();

		//get returned type
		unsigned int type_ln = line_number;
		std::string type = get_symbol();
		dagc* func = NULL;
		if (type == "Bool") {
			func = new dagc(NT_FUN_BOOL, cnt, name); // cnt is the size of parameters.
		}
		else if (type == "Int") {
			func = new dagc(NT_FUN_INT, cnt, name); // cnt is the size of parameters.
		}
		else if (type == "Real") {
			func = new dagc(NT_FUN_REAL, cnt, name); // cnt is the size of parameters.
		}
		else {
			err_unkwn_sym(type, type_ln);
		}
		dag.fun_list.emplace_back(func);

		// parse expression: this is function's content.
		dagc * expr = parse_expr();
		// it will insert into params's front.
		params.insert(params.begin(), expr);
		func->children = params;

		//make func
		// const dagc * res = mk_fun_bind(name, func);
		mk_fun_bind(name, func);

		//remove key bindings: parameters.
		while (key_list.size() > 0) {
			key_map.erase(key_list.back());
			key_list.pop_back();
		}

		return CT_DEFINE_FUN;

	}

	if (command == "define-fun-rec") {
		//ignore
		warn_cmd_nsup(command, command_ln);
		skip_to_rpar();
		return CT_DEFINE_FUN_REC;
	}

	if (command == "define-funs-rec") {
		//ignore
		warn_cmd_nsup(command, command_ln);
		skip_to_rpar();
		return CT_DEFINE_FUNS_REC;
	}

	if (command == "define-sort") {
		//ignore
		warn_cmd_nsup(command, command_ln);
		skip_to_rpar();
		return CT_DEFINE_SORT;
	}

	if (command == "echo") {
		//ignore
		warn_cmd_nsup(command, command_ln);
		skip_to_rpar();
		return CT_ECHO;
	}

	//(exit)
	if (command == "exit") {
		skip_to_rpar();
		return CT_EXIT;
	}

	if (command == "get-assertions") {
		//ignore
		warn_cmd_nsup(command, command_ln);
		skip_to_rpar();
		return CT_GET_ASSERTIONS;
	}

	if (command == "get-assignment") {
		//ignore
		// warn_cmd_nsup(command, command_ln);
		skip_to_rpar();
		return CT_GET_ASSIGNMENT;
	}

	if (command == "get-info") {
		//ignore
		warn_cmd_nsup(command, command_ln);
		skip_to_rpar();
		return CT_GET_INFO;
	}

	if (command == "get-option") {
		//ignore
		warn_cmd_nsup(command, command_ln);
		skip_to_rpar();
		return CT_GET_OPTION;
	}

	if (command == "get-model") {
		//ignore
		dag.get_model = true;
		// warn_cmd_nsup(command, command_ln);
		skip_to_rpar();
		return CT_GET_MODEL;
	}

	if (command == "get-option") {
		//ignore
		warn_cmd_nsup(command, command_ln);
		skip_to_rpar();
		return CT_GET_OPTION;
	}

	if (command == "get-proof") {
		//ignore
		warn_cmd_nsup(command, command_ln);
		skip_to_rpar();
		return CT_GET_PROOF;
	}

	if (command == "get-unsat-assumptions") {
		//ignore
		warn_cmd_nsup(command, command_ln);
		skip_to_rpar();
		return CT_GET_UNSAT_ASSUMPTIONS;
	}

	if (command == "get-unsat-core") {
		//ignore
		warn_cmd_nsup(command, command_ln);
		skip_to_rpar();
		return CT_GET_UNSAT_CORE;
	}

	if (command == "get-value") {
		//ignore
		warn_cmd_nsup(command, command_ln);
		skip_to_rpar();
		return CT_GET_VALUE;
	}

	if (command == "pop") {
		//ignore
		warn_cmd_nsup(command, command_ln);
		skip_to_rpar();
		return CT_POP;
	}

	if (command == "push") {
		//ignore
		warn_cmd_nsup(command, command_ln);
		skip_to_rpar();
		return CT_PUSH;
	}

	if (command == "reset") {
		//ignore
		warn_cmd_nsup(command, command_ln);
		skip_to_rpar();
		return CT_RESET;
	}

	if (command == "reset-assertions") {
		//ignore
		warn_cmd_nsup(command, command_ln);
		skip_to_rpar();
		return CT_RESET_ASSERTIONS;
	}

	//<attribute ::= <keyword> | <keyword> <attribute_value>
	//(set-info <attribute>)
	if (command == "set-info") {
		skip_to_rpar();
		return CT_SET_INFO;

	}

	//(set-logic <symbol>)
	if (command == "set-logic") {

		unsigned int type_ln = line_number;
		std::string type = get_symbol();
		if (type == "QF_LIA") {
			dag.logic = QF_LIA;
		}
		else if (type == "QF_NIA") {
			dag.logic = QF_NIA;
		}
		// else if (type == "QF_LRA") {
		// 	dag.logic = QF_LRA;
		// }
		// else if (type == "QF_NRA") {
		// 	dag.logic = QF_NRA;
		// }
		// else if (type == "QF_NIRA") {
		// 	dag.logic = QF_NIRA;
		// }
		else {
			err_unkwn_sym(type, type_ln);
		}

		return CT_SET_LOGIC;

	}

	//<option ::= <attribute>
	//(set-option <option>)
	if (command == "set-option") {
		skip_to_rpar();
		return CT_SET_OPTION;
	}

	err_unkwn_sym(command, command_ln);

	return CT_UNKNOWN;

}


//expr ::= const | func | (<identifier> <expr>+)
dagc * Parser::parse_expr() {

	if (*bufptr != '(') {
		//const | func

		unsigned int expr_ln = line_number;
		std::string s = get_symbol();

		// support -3 (before only - 3)
		if (isdigit(s[0]) || (s[0]=='-' && s.size() > 1 && isdigit(s[1]))) {
			//constant number
			// (0) will error.
			vType c(s);
			return mk_const(c);
		}
		else {
			boost::unordered_map<std::string, dagc *>::iterator kmap_iter = key_map.find(s);
			if (kmap_iter != key_map.end()) {
				//func found
				return kmap_iter->second;
			}
			else if (s == "true") {
				return mk_true();
			}
			else if (s == "false") {
				return mk_false();
			}
			else {
				//unknown symbol
				err_unkwn_sym(s, expr_ln);
			}
		}

	}

	//(<identifier> <expr>+)
	parse_lpar();

	unsigned int expr_ln = line_number;
	std::string s = get_symbol();

	//parse identifier and get params
	dagc * expr = nullptr;
	if (s == "let") {
		expr = parse_let();
		if (expr->iserr())
			err_all(expr, "let", expr_ln);
	}
	else {
		std::vector<dagc *> params = parse_params();
		if (s == "and") {
			expr = mk_and(params);
		}
		else if (s == "or") {
			expr = mk_or(params);
		}
		else if (s == "not") {
			expr = mk_not(params);
		}
		else if (s == "=>") {
			expr = mk_imply(params);
		}
		else if (s == "xor") {
			expr = mk_xor(params);
		}
		else if (s == "=") {
			expr = mk_eq(params);
		}
		else if (s == "distinct") {
			expr = mk_distinct(params);
		}
		else if (s == "ite") {
			expr = mk_ite(params);
		}
		else if (s == "+") {
			expr = mk_add(params);
		}
		else if (s == "-") {
			expr = mk_minus(params);
		}
		else if (s == "*") {
			expr = mk_mul(params);
		}
		else if (s == "div") {
			expr = mk_div_int(params);
		}
		else if (s == "/") {
			expr = mk_div_real(params);
		}
		else if (s == "mod") {
			expr = mk_mod(params);
		}
		else if (s == "abs") {
			expr = mk_abs(params);
		}
		else if (s == "<=") {
			expr = mk_le(params);
		}
		else if (s == "<") {
			expr = mk_lt(params);
		}
		else if (s == ">=") {
			expr = mk_ge(params);
		}
		else if (s == ">") {
			expr = mk_gt(params);
		}
		else if (s == "to_real") {
			expr = mk_toreal(params);
		}
		else if (s == "to_int") {
			expr = mk_toint(params);
		}
		else if (s == "is_int") {
			expr = mk_isint(params);
		}
		else if (key_map[s]->isfun()){
			expr = mk_fun(key_map[s], params);
		}
		else err_unkwn_sym(s, expr_ln);

		// check error
		if (expr->iserr()) err_all(expr, s, expr_ln);
	}

	parse_rpar();

	return expr;

}

std::vector<dagc *> Parser::parse_params() {

	std::vector<dagc *> params;

	while (*bufptr != ')')
		params.emplace_back(parse_expr());

	return params;

}


/*
keybinding ::= (<symbol> expr)
(let (<keybinding>+) expr), return expr
*/
dagc * Parser::parse_let() {

	parse_lpar();

	// 2021.08.29: function like parser.
	// let -> children[0]: expr, children[1, ..., k]: keybinding[1, ..., k].
	std::vector<dagc* > params;
	// parse key bindings
	std::vector<std::string> key_list;
	while (*bufptr != ')') {

		//(<symbol> expr)
		parse_lpar();

		unsigned int name_ln = line_number;
		std::string name = get_symbol();
		// TODO! now we use an variable to represent the let 
		dagc * expr = parse_expr();
		// e.g. ?v_0 -> children[0] = (+ 1 x)
		expr = mk_let_bind(name, expr);
		params.emplace_back(expr);
		if (expr->iserr()) err_all(expr, name, name_ln);

		parse_rpar();

		//new key
		key_list.emplace_back(name);

	}

	parse_rpar();

	//parse let right expression
	dagc * expr = parse_expr();

	// it will insert into params's front.
	params.insert(params.begin(), expr);
	dagc* res = mk_let(NT_LET, params, "let");

	//remove key bindings: for let uses local variables. 
	while (key_list.size() > 0) {
		key_map.erase(key_list.back());
		key_list.pop_back();
	}

	return res;
}

// mk operations

/*
make general operator
*/
dagc * Parser::mk_oper(const NODE_TYPE t, dagc * p, const std::string &s, vType val) {

	std::vector<dagc*> params{ p };
	return mk_oper(t, params, s, val);
}

dagc * Parser::mk_oper(const NODE_TYPE t, dagc * l, dagc * r, const std::string &s, vType val) {

	// binary operator
	std::vector<dagc *> params{ l, r };
	return mk_oper(t, params, s, val);
}

dagc * Parser::mk_oper(const NODE_TYPE t, std::vector<dagc *> &p, const std::string &s, vType val) {
	
	dagc* res = new dagc(t, dag.op_list.size(), s, val);
	res->children = p;
	dag.op_list.emplace_back(res);
	return res;
}

dagc * Parser::mk_let(const NODE_TYPE t, std::vector<dagc *> &p, const std::string &s, vType val) {

	dagc * newlet = new dagc(t, p.size(), s, val);
	// TODO, no parent, because let will be eliminated.
	newlet->children = p;
	dag.let_list.emplace_back(newlet);

	return newlet;
}

// dagc *	Parser::mk_fun_copy(dagc * fun, dagc* newCopy, const std::vector<dagc* > & params){
// 	for(size_t i=0;i<fun->children.size();i++){
// 		if(fun->children[i]->isfunp()){
// 			newCopy->children.emplace_back(params[fun->children[i]->id]);
// 		}
// 		else{
// 			dagc* tmpchild = tmp->children[i];
// 			if(tmp->isop()){
// 				std::vector<dagc* > rchildren;
// 				newCopy->children.emplace_back(mk_oper(tmpchild->t, rchildren, tmpchild->name, tmpchild->v));
				
// 				que.push(tmpchild);
// 				rque.push(rtmp->children[i]);
// 			}
// 			else if(tmp->isconst()){
// 				// const
// 				newCopy->children.emplace_back(mk_const(tmpchild->v, tmpchild->name));
// 			}
// 			else{
// 				// local variables -> error
// 				return mk_err(ERR_FUN_LOCAL_VAR);
// 			}
// 		}
// 	}
// }

// debug: 2021-11-08, post order traverse.
dagc * Parser::mk_fun_post_order(dagc * fun, std::vector<dagc* >& record, const std::vector<dagc* > & params){
	for(size_t i=0;i<fun->children.size();i++){
		if(fun->children[i]->isfunp()){
			// must isfunp first, otherwiswe op will match it.
			record.emplace_back(params[fun->children[i]->id]);
		}
		else if(fun->children[i]->isop()){
			std::vector<dagc* > tmp;
			record.emplace_back(mk_fun_post_order(fun->children[i], tmp, params));
		}
		else if(fun->children[i]->isconst()){
			record.emplace_back(fun->children[i]);
		}
		else{
			return mk_err(ERR_FUN_LOCAL_VAR);
		}
	}

	return mk_oper(fun->t, record, fun->name, fun->v);
}
dagc * Parser::mk_fun(dagc* fun, const std::vector<dagc *> & params){
	// check the number of params
	if (fun->id != params.size()) // parameters not match
		return mk_err(ERR_PARAM_MIS);

	std::vector<dagc *> new_params;
	// push real parameters to the vector
	for (unsigned int i = 0; i < params.size(); i++) {

		if (params[i]->iserr()) {
			return params[i];
		}
		else {
			// insert uncertain param
			new_params.emplace_back(params[i]);
		}
	}
	// function content
	dagc* formula = fun->children[0];
	
	// debug: 2021.11.01, must make a new dagc.
	std::vector<dagc* > record;
	return mk_fun_post_order(formula, record, new_params);
}


/*
(and Bool Bool+ :left-assoc), return Bool
*/
dagc * Parser::mk_and(const std::vector<dagc *> &params) {

	// support one atom 05/28
	// check the number of params
	// if (params.size() < 2)
	// 	return mk_err(ERR_PARAM_MIS);
	if(params.size() == 0) return mk_err(ERR_PARAM_MIS);
	else if(params.size() == 1) return params[0];

	std::vector<dagc *> new_params;

	for (unsigned int i = 0; i < params.size(); i++) {

		if (params[i]->iserr()) {
			return params[i];
		}
		else if (params[i]->isconst()) {
			if (!params[i]->v) {
				// false: one of params is false
				return mk_false();
			}
			else {
				// true: do nothing
			}
		}
		else {
			// insert uncertain param
			new_params.emplace_back(params[i]);
		}
	}

	if (new_params.size() == 0) {
		// all true constant
		return mk_true();
	}
	else if (new_params.size() == 1) {
		// only one uncertain param
		return new_params[0];
	}
	else {
		// make new AND operator
		return mk_oper(NT_AND, new_params, "and");
	}
}


/*
(or Bool Bool+ :left-assoc), return Bool
*/
dagc * Parser::mk_or(const std::vector<dagc *> &params) {

	// support one atom 05/28
	// // check the number of params
	// if (params.size() < 2)
	// 	return mk_err(ERR_PARAM_MIS);
	if(params.size() == 0) return mk_err(ERR_PARAM_MIS);
	else if(params.size() == 1) return params[0];

	std::vector<dagc *> new_params;

	for (unsigned int i = 0; i < params.size(); i++) {

		if (params[i]->iserr()) {
			return params[i];
		}
		else if (params[i]->isconst()) {
			if (params[i]->v) {
				// true: one of params is true
				return mk_true();
			}
			else {
				// false: do nothing
			}
		}
		else {
			// insert uncertain param
			new_params.emplace_back(params[i]);
		}
	}

	if (new_params.size() == 0) {
		// all false constant
		return mk_false();
	}
	else if (new_params.size() == 1) {
		// only one uncertain param
		return new_params[0];
	}
	else {
		// make new OR operator
		return mk_oper(NT_OR, new_params, "or");
	}
}

/*
(not Bool), return Bool
*/
dagc * Parser::mk_not(dagc * param) {

	if (param->iserr())	return param;

	if (param->isnot()) {
		// reduce nested not
		assert(param->children.size() == 1);
		return param->children[0];
	}
	else if (param->isconst()) {
		// param is a const
		if (param->isconst() && param->v) {
			return mk_false();
		}
		else {
			assert(param->isconst() && !param->v);
			return mk_true();
		}
	}
	else {
		return mk_oper(NT_NOT, param, "not");
	}
}

dagc * Parser::mk_not(const std::vector<dagc *> &params) {

	if (params.size() != 1)
		return mk_err(ERR_PARAM_MIS);

	return mk_not(params[0]);
}

/*
(=> Bool Bool+ :right-assoc), return Bool
(=> a b c d) <=> (or -a -b -c d)
*/
dagc * Parser::mk_imply(const std::vector<dagc *> &params) {

	std::vector<dagc *> new_params(params);

	// transform imply into or
	// (=> a b c d) <=> (or -a -b -c d)
	for (unsigned int i = 0; i < new_params.size() - 1; i++) {
		new_params[i] = mk_not(params[i]);
		if (new_params[i]->iserr())	return new_params[i];
	}

	if (new_params.back()->iserr()) return new_params.back();

	return mk_or(new_params);

}


/*
(xor Bool Bool+ :left-assoc), return Bool
*/
dagc * Parser::mk_xor(const std::vector<dagc *> &params) {

	// check the number of params
	if (params.size() < 2)
		return mk_err(ERR_PARAM_MIS);

	dagc * res = params[0];

	if (res->iserr()) return res;

	// (xor a b c d) <=> (neq (neq (neq a b) c) d)
	for (unsigned int i = 1; i < params.size(); i++) {

		// (xor pi-1 pi) <=> (neq pi-1 pi)
		res = mk_not(mk_eq(res, params[i]));
		if (res->iserr()) return res;
	}

	return res;

}


/*
(= A A+ :chainable), return Bool
*/
// binary operation version
dagc * Parser::mk_eq(dagc * l, dagc * r) {

	if (l->iserr()) return l;
	if (r->iserr()) return r;

	if (l->isbool()) {
		return mk_eq_bool(l, r);
	}
	else {
		if(l == r){
			return mk_true(); // new simplification 5/27
		}
		return mk_oper(NT_EQ, l, r, "=");
	}

}

dagc * Parser::mk_eq(const std::vector<dagc *> &params) {

	// check the number of params
	if (params.size() < 2)
		return mk_err(ERR_PARAM_MIS);

	// call binary operation version
	if (params.size() == 2)
		return mk_eq(params[0], params[1]);

	// (= pivot b c d) <=> (and (= pivot b) (= pivot c) (= pivot d))
	std::vector<dagc *> new_params;

	for (unsigned int i = 1; i < params.size(); i++) {
		dagc * res = mk_eq(params[0], params[i]);
		if (res->iserr()) return res;
		else new_params.emplace_back(res);
	}

	return mk_and(new_params);

}

// core of equal, inner use
// assume l and r are boolean-elements
dagc * Parser::mk_eq_bool(dagc * l, dagc * r) {

	if (l->isconst()) {
		if (r->isconst()) {
			// left and right are either constants
			if (l->v == r->v) return mk_true();
			else return mk_false();
		}
		else if (l->v) {
			// (= true r) <=> r
			return r;
		}
		else {
			// (= false r) <=> (not r)
			return mk_not(r);
		}
	}
	else if (r->isconst()) {
		if (r->v) {
			// (= l true) <=> l
			return l;
		}
		else {
			// (= l false) <=> (not l)
			return mk_not(l);
		}
	}
	else if(l == r){
		return mk_true(); // new simplification 5/27
	}
	else {
		// make new EQ operator
		// debug: 2021-11-08, eq
		// return mk_and(mk_or(l, r), mk_or(mk_not(l), mk_not(r)));
		// return mk_not(mk_and(mk_or(l, r), mk_or(mk_not(l), mk_not(r))));
		// return mk_and(mk_or(mk_not(l), r), mk_or(l, mk_not(r)));
		return mk_oper(NT_EQ_BOOL, l, r, "=");
	}
}


/*
(distinct A A+ :std::pairwise), return Bool
*/

dagc * Parser::mk_neq(dagc * l, dagc * r) {

	if (l->iserr()) return l;
	if (r->iserr()) return r;

	if (l->isconst() && r->isconst()) {
		if (l->v != r->v) return mk_true();
		else return mk_false();
	}
	else {
		if(l == r){
			return mk_false(); // new simplification 5/27
		}
		return mk_oper(NT_NEQ, l, r, "!=");
	}

}
dagc * Parser::mk_distinct(const std::vector<dagc *> &params) {

	// check the number of params
	if (params.size() < 2)
		return mk_err(ERR_PARAM_MIS);

	// call binary operation version
	if (params.size() == 2)
		return mk_distinct(params[0], params[1]);

	// pairwise: (distinct a b c) <=> (and (neq a b) (neq a c) (neq b c))
	std::vector<dagc *> new_params;
	for (unsigned int i = 0; i < params.size() - 1; i++) {
		for (unsigned int j = i + 1; j < params.size(); j++) {
			dagc * res = mk_distinct(params[i], params[j]);
			if (res->iserr()) return res;
			else new_params.emplace_back(res);
		}
	}

	return mk_and(new_params);

}

/*
(<= rt rt+), return rt
*/
dagc * Parser::mk_le(dagc * l, dagc * r) {

	if (l->iserr()) return l;
	if (r->iserr()) return r;

	if (l->isconst() && r->isconst()) {
		if (l->v <= r->v) return mk_true();
		else return mk_false();
	}
	else {
		return mk_oper(NT_LE, l, r, "<=");
	}

}

dagc * Parser::mk_le(const std::vector<dagc *> &params) {

	// check the number of params
	if (params.size() < 2)
		return mk_err(ERR_PARAM_MIS);

	// call binary operation version
	if (params.size() == 2)
		return mk_le(params[0], params[1]);

	// pairwise: (<= a b c) <=> (and (<= a b) (<= b c))
	std::vector<dagc *> new_params;
	for (unsigned int i = 0; i < params.size() - 1; i++) {
		dagc * res = mk_le(params[i], params[i + 1]);
		if (res->iserr()) return res;
		else new_params.emplace_back(res);
	}

	return mk_and(new_params);
}

/*
(< rt rt+), return rt
*/
dagc * Parser::mk_lt(dagc * l, dagc * r) {

	if (l->iserr()) return l;
	if (r->iserr()) return r;

	if (l->isconst() && r->isconst()) {
		if (l->v < r->v) return mk_true();
		else return mk_false();
	}
	else {
		return mk_oper(NT_LT, l, r, "<");
	}

}
dagc * Parser::mk_lt(const std::vector<dagc *> &params) {

	// check the number of params
	if (params.size() < 2)
		return mk_err(ERR_PARAM_MIS);

	// call binary operation version
	if (params.size() == 2)
		return mk_lt(params[0], params[1]);

	// pairwise: (< a b c) <=> (and (< a b) (< b c))
	std::vector<dagc *> new_params;
	for (unsigned int i = 0; i < params.size() - 1; i++) {
		dagc * res = mk_lt(params[i], params[i + 1]);
		if (res->iserr()) return res;
		else new_params.emplace_back(res);
	}

	return mk_and(new_params);
}

/*
(>= rt rt+), return rt
*/
dagc * Parser::mk_ge(dagc * l, dagc * r) {

	if (l->iserr()) return l;
	if (r->iserr()) return r;

	if (l->isconst() && r->isconst()) {
		if (l->v >= r->v) return mk_true();
		else return mk_false();
	}
	else {
		return mk_oper(NT_GE, l, r, ">=");
	}

}
dagc * Parser::mk_ge(const std::vector<dagc *> &params) {

	// check the number of params
	if (params.size() < 2)
		return mk_err(ERR_PARAM_MIS);

	// call binary operation version
	if (params.size() == 2)
		return mk_ge(params[0], params[1]);

	// pairwise: (>= a b c) <=> (and (>= a b) (>= b c))
	std::vector<dagc *> new_params;
	for (unsigned int i = 0; i < params.size() - 1; i++) {
		dagc * res = mk_ge(params[i], params[i + 1]);
		if (res->iserr()) return res;
		else new_params.emplace_back(res);
	}

	return mk_and(new_params);
}

/*
(> rt rt+), return rt
*/
dagc * Parser::mk_gt(dagc * l, dagc * r) {

	if (l->iserr()) return l;
	if (r->iserr()) return r;

	if (l->isconst() && r->isconst()) {
		if (l->v > r->v) return mk_true();
		else return mk_false();
	}
	else {
		return mk_oper(NT_GT, l, r, ">");
	}

}
dagc * Parser::mk_gt(const std::vector<dagc *> &params) {

	// check the number of params
	if (params.size() < 2)
		return mk_err(ERR_PARAM_MIS);

	// call binary operation version
	if (params.size() == 2)
		return mk_gt(params[0], params[1]);

	// pairwise: (> a b c) <=> (and (> a b) (> b c))
	std::vector<dagc *> new_params;

	for (unsigned int i = 0; i < params.size() - 1; i++) {
		dagc * res = mk_gt(params[i], params[i + 1]);
		if (res->iserr()) return res;
		else new_params.emplace_back(res);

	}

	return mk_and(new_params);
}

/*
(ite Bool A A), return A
*/
dagc * Parser::mk_ite(dagc * c, dagc * l, dagc * r) {

	if (c->iserr()) return c;
	if (l->iserr()) return l;
	if (r->iserr()) return r;

	bool isbool = l->isbool() || l->iscbool();

	// check returned type
	// debug: 2021.08.22, (isbool != r->isbool()) -> (isbool != (r->isbool()||r->iscbool()))
	if(isbool != (r->isbool()||r->iscbool()))
		return mk_err(ERR_PARAM_NSAME);

	// condition is constant
	if (c->isconst()) {
		if (c->v==1) return l;
		else return r;
	}

	// make new ITE operator
	std::vector<dagc *> new_params{ c, l, r };
	if (isbool)
		return mk_oper(NT_ITE_BOOL, new_params, "ite");
	else
		return mk_oper(NT_ITE_NUM, new_params, "ite");
}

dagc * Parser::mk_ite(const std::vector<dagc *> &params) {

	if (params.size() != 3)
		return mk_err(ERR_PARAM_MIS);

	return mk_ite(params[0], params[1], params[2]);
}

/*
(+ rt rt+), return rt
*/
dagc * Parser::mk_add(const std::vector<dagc *> &params) {

	// check the number of params
	if (params.size() < 2)
		return mk_err(ERR_PARAM_MIS);

	std::vector<dagc *> new_params;
	vType value = 0;
	for (unsigned int i = 0; i < params.size(); i++) {
		if (params[i]->iserr()) {
			return params[i];
		}
		else if (params[i]->isconst()) {
			//merge constants
			value += params[i]->v;
		}
		else {
			new_params.emplace_back(params[i]);
		}
	}

	if (new_params.size() == 0) {
		// all constants
		return mk_const(value);
	}
	else if (new_params.size() == 1 && value == 0) {
		return new_params[0];
	}
	else {
		// push the constant value at last
		if (value != 0) new_params.emplace_back(mk_const(value));
		return mk_oper(NT_ADD, new_params, "+");
	}
}


/*
(- rt), return rt
(- rt rt+), return rt
*/
dagc * Parser::mk_neg(dagc * param) {

	if (param->iserr()) {
		return param;
	}
	else if (param->isconst()) {
		return mk_const(-param->v);
	}
	else {
		std::vector<dagc *> new_params{ param, mk_const(-1) };
		return mk_mul(new_params);
	}
}

dagc * Parser::mk_minus(const std::vector<dagc *> &params) {

	// check the number of params
	if (params.size() == 1)
		return mk_neg(params[0]);
	else if (params.size() < 1)
		return mk_err(ERR_PARAM_MIS);

	std::vector<dagc *> new_params(params);

	// (- a b c d) <=> (+ a -b -c -d)
	// 
	if (new_params[0]->iserr())
		return new_params[0];
	for (unsigned int i = 1; i < new_params.size(); i++) {
		dagc * res = mk_neg(new_params[i]);
		if (res->iserr()) return res;
		else new_params[i] = res;
	}

	// all syntax checks rely on mk_add func	
	return mk_add(new_params);
}

/*
(* rt rt+), return rt
*/
dagc * Parser::mk_mul(const std::vector<dagc *> &params) {
	// check the number of params
	// if (params.size() < 2)
	// 	return mk_err(ERR_PARAM_MIS);
	if(params.size() == 1){
		// (* rt rt*), return rt
		return params[0];
	}
	if (params.size() < 2)
		return mk_err(ERR_PARAM_MIS);

	std::vector<dagc *> new_params;
	vType value = 1;

	for (unsigned int i = 0; i < params.size(); i++) {
		if (params[i]->iserr()) {
			// TODO
			return params[i];
		}
		else if (params[i]->isconst()) {
			// merge constants
			value *= params[i]->v;
			if (value == 0) return mk_const(0);
		}
		else {
			new_params.emplace_back(params[i]);
		}
	}

	if (new_params.size() == 0) {
		// all constants
		return mk_const(value);
	}
	else {
		// make new MUL operator
		if (value != 1) new_params.emplace_back(mk_const(value));
		else if(new_params.size() == 1){
			// (* x)
			return new_params[0];
		}
		return mk_oper(NT_MUL, new_params, "*");
	}
}


/*
(div Int Int), return Int
*/
dagc * Parser::mk_div_int(dagc * l, dagc * r) {

	if (r->isconst()) {
		if (l->isconst()) {
			// all constants
			return mk_const(l->v / r->v);
		}
		// debug: 2021.11.27, integer cannot do like this.
		// else {
		// 	std::vector<dagc *> new_params{ l, mk_const(1.0 / r->v) };
		// 	return mk_mul(new_params);
		// }
	}
	// make new DIV operator
	return mk_oper(NT_DIV_INT, l, r, "div");

}

dagc * Parser::mk_div_int(const std::vector<dagc *> &params) {

	if (params.size() != 2)
		return mk_err(ERR_PARAM_MIS);

	return mk_div_int(params[0], params[1]);
}

/*
(/ Real Real), return Real
*/
dagc * Parser::mk_div_real(dagc * l, dagc * r) {

	if (r->isconst()) {
		if (l->isconst()) {
			// all constants
			return mk_const(l->v / r->v);
		}
		else {
			std::vector<dagc *> new_params{ l, mk_const(1.0 / r->v) };
			return mk_mul(new_params);
		}
	}
	else {
		// make new DIV operator
		return mk_oper(NT_DIV_REAL, l, r, "/");
	}

}

dagc * Parser::mk_div_real(const std::vector<dagc *> &params) {

	if (params.size() != 2)
		return mk_err(ERR_PARAM_MIS);

	return mk_div_real(params[0], params[1]);
}

/*
(mod Int Int), return Int
*/
dagc * Parser::mk_mod(dagc * l, dagc * r) {

	if (r->isconst() && l->isconst()) {
		// all constants
		return mk_const(l->v % r->v);
	}
	else {
		// make new MOD operator
		return mk_oper(NT_MOD, l, r, "mod");
	}

}

dagc * Parser::mk_mod(const std::vector<dagc *> &params) {

	if (params.size() != 2)
		return mk_err(ERR_PARAM_MIS);

	return mk_mod(params[0], params[1]);

}

/*
(abs Int), return Int
*/
dagc * Parser::mk_abs(dagc * param) {

	if (param->iserr()) {
		return param;
	}
	else if (param->isconst()) {
		if (param->v >= 0) return param;
		else return mk_const(-param->v);
	}
	else {
		// make new ABS operator
		return mk_oper(NT_ABS, param, "abs");
	}

}

dagc * Parser::mk_abs(const std::vector<dagc *> &params) {

	if (params.size() != 1)
		return mk_err(ERR_PARAM_MIS);

	return mk_abs(params[0]);

}


/*
(to_real Int), return Real
*/
dagc * Parser::mk_toreal(dagc * param) {

	if (param->isconst()) {
		return param;
	}
	else {
		// make new DIV operator
		return mk_oper(NT_TO_REAL, param, "to_real");
	}

}

dagc * Parser::mk_toreal(const std::vector<dagc *> &params) {

	if (params.size() != 1)
		return mk_err(ERR_PARAM_MIS);

	return mk_toreal(params[0]);

}

/*
(to_int Real), return Int
*/
dagc * Parser::mk_toint(dagc * param) {

	if (param->isconst()) {
		// return mk_const(trunc(param->v));
		return mk_const((param->v));
	}
	else {
		// make new DIV operator
		return mk_oper(NT_TO_INT, param, "to_int");
	}

}

dagc * Parser::mk_toint(const std::vector<dagc *> &params) {

	if (params.size() != 1)
		return mk_err(ERR_PARAM_MIS);

	return mk_toint(params[0]);

}

/*
(is_int Real), return Bool
*/
dagc * Parser::mk_isint(dagc * param) {

	if (param->isconst()) {
		// if (param->v == trunc(param->v))
		if (param->v == (param->v))
			return mk_true();
		else
			return mk_false();
	}
	else {
		// make new DIV operator
		return mk_oper(NT_IS_INT, param, "is_int");
	}

}

dagc * Parser::mk_isint(const std::vector<dagc *> &params) {

	if (params.size() != 1)
		return mk_err(ERR_PARAM_MIS);

	return mk_isint(params[0]);

}

/*
make declared bool
*/
dagc * Parser::mk_bool_decl(const std::string &name) {

	dagc * expr = new dagc(NT_VAR_BOOL, dag.vbool_list.size(), name);

	std::pair<boost::unordered_map<std::string, dagc *>::iterator, bool>
		p = key_map.insert(std::pair<std::string, dagc *>(name, expr));

	if (!p.second) {
		// multiple declarations
		delete expr;
		return mk_err(ERR_MUL_DECL);
	}
	else {
		// insertion took place
		dag.vbool_list.emplace_back(expr);

		return expr;
	}
}


/*
make declared var
*/
dagc * Parser::mk_var_decl(const std::string &name, bool isInt) {

	dagc * expr = NULL;
	if (isInt) {
		expr = new dagc(NT_VAR_NUM, dag.vnum_int_list.size(), name);
	}
	else {
		expr = new dagc(NT_VAR_NUM, dag.vnum_real_list.size(), name);
	}

	std::pair<boost::unordered_map<std::string, dagc *>::iterator, bool>
		p = key_map.insert(std::pair<std::string, dagc *>(name, expr));

	if (!p.second) {
		// multiple declarations
		delete expr;
		return mk_err(ERR_MUL_DECL);
	}
	else {
		// insertion took place
		if (isInt) {
			dag.vnum_int_list.emplace_back(expr);
		}
		else {
			dag.vnum_real_list.emplace_back(expr);
		}
		return expr;
	}
}


/*
make key binding
*/
dagc * Parser::mk_fun_bind(const std::string &key, dagc * expr) {

	std::pair<boost::unordered_map<std::string, dagc *>::iterator, bool>
		p = key_map.insert(std::pair<std::string, dagc *>(key, expr));

	if (!p.second) {
		// multiple key bindings
		return mk_err(ERR_MUL_DECL);
	}
	else {
		// insertion took place
		return expr;
	}
}
dagc * Parser::mk_let_bind(const std::string &key, dagc * expr) {
	bool isbool = expr->isbool() || expr->iscbool() || expr->iseqbool();
	std::vector<dagc *> new_params{expr};
	dagc* newlet;
	if(isbool){
		newlet = mk_let(NT_VAR_LET_BOOL, new_params, key);
	}
	else{
		newlet = mk_let(NT_VAR_LET_NUM, new_params, key);
	}
	std::pair<boost::unordered_map<std::string, dagc *>::iterator, bool> 
		p = key_map.insert(std::pair<std::string, dagc *>(key, newlet));
	if (!p.second) {
		// multiple key bindings
		return mk_err(ERR_MUL_DECL);
	}
	else {
		// dag.vlet_list.emplace_back(newlet);
		return newlet;
	}
}

// error operations
void Parser::err_all(const ERROR_TYPE e, const std::string s, const unsigned int ln) const {

	switch (e) {
	case ERR_UNEXP_EOF:
		err_unexp_eof();
		break;
	case ERR_SYM_MIS:
		err_sym_mis(s, ln);
		break;
	case ERR_UNKWN_SYM:
		err_unkwn_sym(s, ln);
		break;
	case ERR_PARAM_MIS:
		err_param_mis(s, ln);
		break;
	case ERR_PARAM_NBOOL:
		err_param_nbool(s, ln);
		break;
	case ERR_PARAM_NNUM:
		err_param_nnum(s, ln);
		break;
	case ERR_PARAM_NSAME:
		err_param_nsame(s, ln);
		break;
	case ERR_LOGIC:
		err_logic(s, ln);
		break;
	case ERR_MUL_DECL:
		err_mul_decl(s, ln);
		break;
	case ERR_MUL_DEF:
		err_mul_def(s, ln);
		break;
	case ERR_NLINEAR:
		err_nlinear(s, ln);
		break;
	case ERR_ZERO_DIVISOR:
		err_zero_divisor(ln);
		break;
	case ERR_FUN_LOCAL_VAR:
		err_param_nsame(s, ln);
		break;
	}
}

void Parser::err_all(const dagc * e, const std::string s, const unsigned int ln) const {
	err_all((ERROR_TYPE)e->id, s, ln);
}

// unexpected end of file
void Parser::err_unexp_eof() const {
	std::cout << "error: Unexpected end of file found." << std::endl;
	exit(0);
}

// symbol missing
void Parser::err_sym_mis(const std::string mis, const unsigned int ln) const {
	std::cout << "error: \"" << mis << "\" missing in line " << ln << '.' << std::endl;
	exit(0);
}

void Parser::err_sym_mis(const std::string mis, const std::string nm, const unsigned int ln) const {
	std::cout << "error: \"" << mis << "\" missing before \"" << nm << "\" in line " << ln << '.' << std::endl;
	exit(0);
}

// unknown symbol
void Parser::err_unkwn_sym(const std::string nm, const unsigned int ln) const {
	if (nm == "") err_unexp_eof();
	std::cout << "error: Unknown or unexptected symbol \"" << nm << "\" in line " << ln << '.' << std::endl;
	exit(0);
}

// wrong number of parameters
void Parser::err_param_mis(const std::string nm, const unsigned int ln) const {
	std::cout << "error: Wrong number of parameters of \"" << nm << "\" in line " << ln << '.' << std::endl;
	exit(0);
}

// paramerter type error
void Parser::err_param_nbool(const std::string nm, const unsigned int ln) const {
	std::cout << "error: Invalid command \"" << nm << "\" in line "
		<< ln << ", paramerter is not a boolean." << std::endl;
	exit(0);
}

void Parser::err_param_nnum(const std::string nm, const unsigned int ln) const {
	std::cout << "error: Invalid command \"" << nm << "\" in line "
		<< ln << ", paramerter is not an integer or a real." << std::endl;
	exit(0);
}

// paramerters are not in same type
void Parser::err_param_nsame(const std::string nm, const unsigned int ln) const {
	std::cout << "error: Invalid command \"" << nm << "\" in line "
		<< ln << ", paramerters are not in same type." << std::endl;
	exit(0);
}

// logic doesnt support
void Parser::err_logic(const std::string nm, const unsigned int ln) const {
	std::cout << "error: Logic does not support \"" << nm << "\" in line " << ln << '.' << std::endl;
	exit(0);
}

// multiple declaration
void Parser::err_mul_decl(const std::string nm, const unsigned int ln) const {
	std::cout << "error: Multiple declarations of \"" << nm << "\" in line " << ln << '.' << std::endl;
	exit(0);
}

// multiple definition
void Parser::err_mul_def(const std::string nm, const unsigned int ln) const {
	std::cout << "error: Multiple definitions or keybindings of \""
		<< nm << "\" in line " << ln << '.' << std::endl;
	exit(0);
}

// nonlinear arithmetic
void Parser::err_nlinear(const std::string nm, const unsigned int ln) const {
	std::cout << "error: Logic does not support nonlinear arithmetic of command \""
		<< nm << "\" in line " << ln << '.' << std::endl;
	exit(0);
}

// divisor is zero
void Parser::err_zero_divisor(const unsigned int ln) const {
	std::cout << "error: Divisor is zero in line " << ln << '.' << std::endl;
	exit(0);
}


/*
global errors
*/
// cannot open file
void Parser::err_open_file(const std::string filename) const {
	std::cout << "error: Cannot open file \"" << filename << "\"." << std::endl;
	exit(0);
}


/*
warnings
*/
// command not support
void Parser::warn_cmd_nsup(const std::string nm, const unsigned int ln) const {
	std::cout << "warning: \"" << nm << "\" command is safely ignored in line " << ln << "." << std::endl;
}
