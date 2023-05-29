/* dag.hpp
*
*  Copyright (C) 2023-2026 Cunjing Ge & Minghao Liu & Fuqi Jia.
*
*  All rights reserved.
*/

#ifndef DAG_HEADER
#define DAG_HEADER

#include "utils/logic.hpp"
#include "utils/types.hpp"

#include <iostream>
#include <fstream>
#include <cassert>

#include <string>
#include <vector>
#include <list>
#include <algorithm>
#include <ctime>
#include <cstdlib>
#include <boost/unordered_map.hpp>

namespace ismt {
typedef Integer vType;

// mask to get type
const int mask_assigned = 1;
const int mask_new_interval = 2;
const int mask_top_constraint = 4;
// const int mask_asserted = 8;

class DAG {
public:
	// DAG core 
	// <t, id, v ,m> --- <type, index, value, multiplier>
	// const:	id is disabled
	// var:		id is the index of vbool_list or vnum_list, v is disabled
	// ineq:	id is the index of ineq_list, v is disabled
	// oper:	id is the index of op_list, v is enabled only for addition
	class dagc {
	public:
		NODE_TYPE		t;
		unsigned int	id;
		std::string		name;
		vType			v;	// value, changed to int, because of nia, lia.
		int				just;

		std::vector<dagc *> children;

		dagc(NODE_TYPE type = NT_UNKNOWN,
			unsigned int index = 0,
			std::string str = "",
			vType value = 0) :
			t(type), id(index), name(str), v(value), just(0) {};
		~dagc() {};

		// overload operator ==
		// only compare type and index, work for boolean-variables and operators
		bool operator ==(const dagc elem) const { return (t == elem.t && id == elem.id); };

		bool iserr() 		const { return (t == NT_ERROR) ? true : false; };
		bool isAssigned()	const { return just & mask_assigned; };
		void assign(vType vv)	  { v = vv; just |= mask_assigned; }
		void unassign()			  { just &= ~mask_assigned; }
		// void assert()			  { just |= mask_asserted; }
		// void unassert()			  { just &= ~mask_asserted; }
		// void isAsserted()		  { return just & mask_asserted; }

		bool isInterval()	const { return just & mask_new_interval; }
		void setIntervalLit()	  { just |= mask_new_interval; }

		bool isTop()		const { return just & mask_top_constraint; }
		void setTop()			  { just |= mask_top_constraint; }
		void unsetTop()			  { just &= ~mask_top_constraint; }

		// check const
		bool iscbool() 		const { return (t == NT_CONST_BOOL) ? true : false; };
		bool istrue() 		const { return (t == NT_CONST_BOOL && v == 1) || ( isbool() && isAssigned() && v == 1 ) ? true : false; };
		bool isfalse() 		const { return (t == NT_CONST_BOOL && v == 0) || ( isbool() && isAssigned() && v == 0 ) ? true : false; };
		bool iscnum() 		const { return (t == NT_CONST_NUM) ? true : false; };
		bool isconst() 		const { return (iscbool() || iscnum()) ? true : false; };

		// check var
		bool isvbool() 		const { return (t == NT_VAR_BOOL) ? true : false; };
		bool isvnum() 		const { return (t == NT_VAR_NUM) ? true : false; };
		bool isvar() 		const { return (isvbool() || isvnum()) ? true : false; };

		// check comp
		bool iseq()			const { return (t == NT_EQ) ? true : false; }
		bool isneq()		const { return (t == NT_NEQ) ? true : false; }
		bool isle()			const { return (t == NT_LE) ? true : false; }
		bool islt()			const { return (t == NT_LT) ? true : false; }
		bool isge()			const { return (t == NT_GE) ? true : false; }
		bool isgt()			const { return (t == NT_GT) ? true : false; }
		bool iscomp() 		const { return (iseq() || isneq() || isle() || islt() || isge() || isgt()) ? true : false; };

		// check ite
		bool isitebool() 	const { return (t == NT_ITE_BOOL) ? true : false; };
		bool isitenum() 	const { return (t == NT_ITE_NUM) ? true : false; };
		bool isite() 		const { return (isitebool() || isitenum()) ? true : false; };

		// check other operators
		bool isand() 		const { return (t == NT_AND) ? true : false; };
		bool isor() 		const { return (t == NT_OR) ? true : false; };
		bool isnot() 		const { return (t == NT_NOT) ? true : false; };
		bool iseqbool()		const { return (t == NT_EQ_BOOL) ? true: false; }; // Ge: a = b -> not a or b and not b or a
		bool isneqbool()	const { return (t == NT_NEQ_BOOL) ? true: false; };

		// check add, mul, div
		bool isadd() 		const { return (t == NT_ADD) ? true : false; };
		bool issub() 		const { return (t == NT_SUB) ? true : false; };
		bool ismul() 		const { return (t == NT_MUL) ? true : false; };
		bool isdivint() 	const { return (t == NT_DIV_INT) ? true : false; };
		bool isdivreal() 	const { return (t == NT_DIV_REAL) ? true : false; };
		bool ismod()		const { return (t == NT_MOD) ? true : false; };
		bool isdiv() 		const { return (isdivint() || isdivreal() || ismod()) ? true : false; };

		// check abs
		bool isabs()		const { return (t == NT_ABS) ? true : false; };

		// check type of operators
		bool isop()			const { return !(isconst() || isvar()); };
		bool isnumop()		const { return (isadd() || issub() || ismul() || isdiv() || isabs() || ismod()) ? true : false; }; // ismod()
		bool isboolop()		const { return (isand() || isor() || iscomp() || isitebool() || isnot() || isletbool()) ? true : false; };

		// check returned type
		bool isbool() 		const { return (isvbool() || isboolop() || isletbool() || iseqbool() || isneqbool()) ? true : false; };

		// let and defined
		bool isletbool()	const {return (t == NT_VAR_LET_BOOL) ? true : false; }
		bool isletnum()		const {return (t == NT_VAR_LET_NUM) ? true : false; }
		bool isletvar()		const {return (isletbool() || isletnum()) ? true : false;}	
		bool islet()		const {return (t == NT_LET) ? true : false;}	
		bool isfunbool()	const {return (t == NT_FUN_BOOL) ? true : false; }
		bool isfunint()		const {return (t == NT_FUN_INT) ? true : false; }
		bool isfunpbool()	const {return (t == NT_FUN_PARAM_BOOL) ? true : false; }
		bool isfunpint()	const {return (t == NT_FUN_PARAM_INT) ? true : false; }
		bool isfunp()		const {return (isfunpbool() || isfunpint()) ? true : false; }
		bool isfun()		const {return (isfunbool()  || isfunint()) ? true : false; }

		void print() const { std::cout << t << ' ' << id << ' ' << v << ' ' << name << std::endl; };

		NODE_TYPE CompRev(){ 
            if(isneq() || iseq() || iseqbool() || isneqbool()){ }
            else if(isge()){ t = NT_LE; name="<="; }
            else if(isle()){ t = NT_GE; name=">="; }
            else if(isgt()){ t = NT_LT; name="<"; }
            else if(islt()){ t = NT_GT; name=">"; }

			return t;
		}
		NODE_TYPE CompNot(){ 
            if(isneq()){ t = NT_EQ; name="="; }
            else if(iseq()){ t = NT_NEQ; name="distinct"; }
            else if(iseqbool()){ t = NT_NEQ_BOOL; name="!="; }
            else if(isneqbool()){ t = NT_EQ_BOOL; name="="; }
            else if(isge()){ t = NT_LT; name="<"; }
            else if(isle()){ t = NT_GT; name=">"; }
            else if(isgt()){ t = NT_LE; name="<="; }
            else if(islt()){ t = NT_GE; name=">="; }

			return t;
		}

	private:
	};


	// methods

public:
	DAG() { 
		true_node = new dagc(NT_CONST_BOOL, 1, "true", true);
		true_node->assign(1);
		false_node = new dagc(NT_CONST_BOOL, 0, "false", false);
		false_node->assign(0);
	};
	~DAG() {
		delete true_node; delete false_node; true_node=nullptr; false_node=nullptr;
		std::vector<dagc *>::iterator it = vbool_list.begin();
		while(it!=vbool_list.end()){ if(*it!=nullptr){ delete *it; *it = nullptr; } ++it; }
		it = vnum_int_list.begin();
		while(it!=vnum_int_list.end()){ if(*it!=nullptr){ delete *it; *it = nullptr; } ++it; }
		it = vnum_real_list.begin();
		while(it!=vnum_real_list.end()){ if(*it!=nullptr){ delete *it; *it = nullptr; } ++it; }
		it = const_list.begin();
		while(it!=const_list.end()){ if(*it!=nullptr){ delete *it; *it = nullptr; } ++it; }
		it = op_list.begin();
		while(it!=op_list.end()){ if(*it!=nullptr){ delete *it; *it = nullptr; } ++it; }
		it = funp_list.begin();
		while(it!=funp_list.end()){ if(*it!=nullptr){ delete *it; *it = nullptr; } ++it; }
		it = fun_list.begin();
		while(it!=fun_list.end()){ if(*it!=nullptr){ delete *it; *it = nullptr; } ++it; }
		it = let_list.begin();
		while(it!=let_list.end()){ if(*it!=nullptr){ delete *it; *it = nullptr; } ++it; }
	};

	// logic
	bool		logic_not_set() const { return logic == UNKNOWN_LOGIC; };
	bool		islia() const { return logic == QF_LIA; };
	bool		isnia() const { return logic == QF_NIA; };
	bool		islra() const { return logic == QF_LRA; };
	bool		isnra() const { return logic == QF_NRA; };
	bool		isnira() const { return logic == QF_NIRA; };

	// print
	void printAST(dagc * root) const{

		if (root->iscbool()) {
			// boolean constant
			if (root->v) std::cout << "true";
			else std::cout << "false";
		}
		else if (root->iscnum()) {
			// numeric constant
			std::cout << root->v;
		}
		else if (root->isvbool()) {
			// boolean variable
			std::cout << root->name;
		}
		else if (root->isvnum()) {
			// numeric variable
			std::cout << root->name;
		}
		else if (root->isletbool()){
			std::cout << root->name <<" ";
		}
		else if (root->isletnum()){
			std::cout << root->name ;
			// printAST(root->children[0]);
		}
		else if (root->islet()) {
			// let variable
			std::cout << '(';
			std::cout << root->name <<" ";
			std::cout << '(';
			for(size_t i=1;i<root->children.size();i++){
				if (root->children[i]->isletbool()){
					std::cout << '(';
					std::cout << root->children[i]->name <<" ";
					printAST(root->children[i]->children[0]);
					std::cout << ')';
				}
				else if (root->children[i]->isletnum()){
					std::cout << '(';
					std::cout << root->children[i]->name <<" ";
					printAST(root->children[i]->children[0]);
					std::cout << ')';
				}
			}
			std::cout << ')';
			printAST(root->children[0]);
			std::cout << ')';
		}
		else if(root->isfun()) {
			// let variable
			std::cout << root->name;
		}
		else {
			// operators

			std::cout << '(';

			if (root->isand())
				std::cout << "and";
			else if (root->isor())
				std::cout << "or";
			else if (root->isnot())
				std::cout << "not";
			else if (root->iseq())
				std::cout << "=";
			else if (root->iseqbool())
				std::cout << "=";
			else if (root->isneq())
				std::cout << "!=";
			else if (root->isite())
				std::cout << "ite";
			else if (root->isadd())
				std::cout << "+";
			else if (root->issub())
				std::cout << "+";
			else if (root->ismul())
				std::cout << "*";
			else if (root->isdivint())
				std::cout << "div";
			else if (root->isdivreal())
				std::cout << "/";
			else if (root->ismod())
				std::cout << "mod";
			else if (root->isabs())
				std::cout << "abs";
			else if (root->isle())
				std::cout << "<=";
			else if (root->islt())
				std::cout << "<";
			else if (root->isge())
				std::cout << ">=";
			else if (root->isgt())
				std::cout << ">";
			else root->print();
			// else assert(false);

			const std::vector<dagc *> c = root->children;
			for (unsigned int i = 0; i < c.size(); i++) {
				std::cout << ' ';
				printAST(c[i]);
			}

			std::cout << ')';
		}
	}

	
	void toSMT2(dagc * root) const{
		if (root->iscbool()) {
			// boolean constant
			if (root->v) std::cout << "true";
			else std::cout << "false";
		}
		else if (root->iscnum()) {
			// numeric constant
			std::cout << std::string(root->v.get_str());
		}
		else if (root->isvbool()) {
			// boolean variable
			std::cout << root->name;
		}
		else if (root->isvnum()) {
			// numeric variable
			std::cout << root->name;
		}
		else if (root->isletvar()){
			toSMT2(root->children[0]);
		}
		else if (root->islet()) {
			// let variable
			std::cout << '(';
			std::cout << root->name + " ";
			std::cout << '(';
			for(size_t i=1;i<root->children.size();i++){
				if (root->children[i]->isletbool()){
					std::cout << '(';
					std::cout << root->children[i]->name + " ";
					toSMT2(root->children[i]->children[0]);
					std::cout << ") ";
				}
				else if (root->children[i]->isletnum()){
					std::cout << '(';
					std::cout << root->children[i]->name + " ";
					toSMT2(root->children[i]->children[0]);
					std::cout << ") ";
				}
			}
			std::cout << ") ";
			toSMT2(root->children[0]);
			std::cout << ')';
		}
		else if(root->isfun()) {
			// let variable
			std::cout << root->name;
		}
		else {
			// operators

			std::cout << '(';

			if (root->isand())
				std::cout << "and";
			else if (root->isor())
				std::cout << "or";
			else if (root->isnot())
				std::cout << "not";
			else if (root->iseq())
				std::cout << "=";
			else if (root->iseqbool())
				std::cout << "=";
			else if (root->isneqbool())
				std::cout << "!=";
			else if (root->isneq())
				std::cout << "distinct";
			else if (root->isite())
				std::cout << "ite";
			else if (root->isadd())
				std::cout << "+";
			else if (root->issub())
				std::cout << "+";
			else if (root->ismul())
				std::cout << "*";
			else if (root->isdivint())
				std::cout << "div";
			else if (root->isdivreal())
				std::cout << "/";
			else if (root->ismod())
				std::cout << "mod";
			else if (root->isabs())
				std::cout << "abs";
			else if (root->isle())
				std::cout << "<=";
			else if (root->islt())
				std::cout << "<";
			else if (root->isge())
				std::cout << ">=";
			else if (root->isgt())
				std::cout << ">";
			else root->print();
			// else assert(false);

			const std::vector<dagc *> c = root->children;
			for (unsigned int i = 0; i < c.size(); i++) {
				std::cout << ' ';
				toSMT2(c[i]);
			}

			std::cout << ')';
		}
	}

	void print_smtlib(){
		auto it = assert_list.begin();
		while(it!=assert_list.end()){
			std::cout<<"(assert ";
			toSMT2(*it);
			std::cout<<")"<<std::endl;
			++it;
		}
	}

	void print_constraints(){
		auto it = assert_list.begin();
		while(it!=assert_list.end()){
			printAST(*it);
			std::cout<<std::endl;
			++it;
		}
	}

	// constant bool node
	dagc * 			true_node;
	dagc *			false_node;

	// attritubes
public:
	// std::vector<dagc *> assert_list;	// assertions
	std::list<dagc *> assert_list;	// assertions
	std::vector<dagc *> vbool_list;		// boolean variables
	std::vector<dagc *> vnum_int_list;	// integer variables
	std::vector<dagc *> vnum_real_list;	// real number variables
	std::vector<dagc *> const_list;		// constants
	std::vector<dagc *> op_list;		// operators
	std::vector<dagc *> funp_list;		// function parameters
	std::vector<dagc *> fun_list;		// function list
	std::vector<dagc *> let_list;		// let list
	

	ENUM_LOGIC logic;
	bool get_model;
	bool check_sat;

	dagc* mk_true()		{return true_node;}
	dagc* mk_false()	{return false_node;}
};
typedef DAG::dagc dagc;

}
#endif