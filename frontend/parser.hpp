/* parser.hpp
*
*  Copyright (C) 2023-2026 Cunjing Ge & Minghao Liu & Fuqi Jia.
*
*  All rights reserved.
*/

#ifndef PARSER_HEADER
#define PARSER_HEADER

#include "frontend/dag.hpp"

#define NDEBUG

// internal string length, name of inequalities and constants
#define STRLEN 30


namespace ismt {


	enum SCAN_MODE {
		SM_COMMON,
		SM_SYMBOL,
		SM_COMP_SYM,
		SM_COMMENT,
		SM_STRING
	};


	enum CMD_TYPE {
		CT_UNKNOWN, CT_EOF,
		// COMMANDS
		CT_ASSERT, CT_CHECK_SAT, CT_CHECK_SAT_ASSUMING,
		CT_DECLARE_CONST, CT_DECLARE_FUN, CT_DECLARE_SORT,
		CT_DEFINE_FUN, CT_DEFINE_FUN_REC, CT_DEFINE_FUNS_REC, CT_DEFINE_SORT,
		CT_ECHO, CT_EXIT,
		CT_GET_ASSERTIONS, CT_GET_ASSIGNMENT, CT_GET_INFO,
		CT_GET_MODEL, CT_GET_OPTION, CT_GET_PROOF,
		CT_GET_UNSAT_ASSUMPTIONS, CT_GET_UNSAT_CORE, CT_GET_VALUE,
		CT_POP, CT_PUSH, CT_RESET, CT_RESET_ASSERTIONS,
		CT_SET_INFO, CT_SET_LOGIC, CT_SET_OPTION,
	};

	enum ERROR_TYPE {
		ERR_UNEXP_EOF,
		ERR_SYM_MIS,
		ERR_UNKWN_SYM,
		ERR_PARAM_MIS,
		ERR_PARAM_NBOOL,
		ERR_PARAM_NNUM,
		ERR_PARAM_NSAME,
		ERR_LOGIC,
		ERR_MUL_DECL,
		ERR_MUL_DEF,
		ERR_NLINEAR,
		ERR_ZERO_DIVISOR,
		ERR_FUN_LOCAL_VAR
	};


	/*
	Parser
	*/


	class Parser {

	public:

		DAG & dag;
		
		// Jia: store the DAG in target.
		Parser(std::string filename, DAG & target) : dag(target) {

			err_node = new dagc(NT_ERROR);

			dag.logic = UNKNOWN_LOGIC;
			dag.check_sat = false;
			dag.get_model = false;
			parse_smtlib2_file(filename);
			// for(size_t i=0;i<target.assert_list.size();i++){
			// 	dag.printAST(target.assert_list[i]);
			// }
		};
		~Parser() { delete err_node; err_node = nullptr; };

		// mk
		dagc *	mk_true() { return dag.mk_true(); }
		dagc *	mk_false() { return dag.mk_false(); }
		// dagc *	mk_const(const vType v, const std::string &s = "") {
		dagc *	mk_const(const vType v) {
			std::string name = v.get_str();
			if(constants.find(name)!=constants.end()){
				return dag.const_list[constants[name]];
			}
			else{
				dagc * newconst = new dagc(NT_CONST_NUM, dag.const_list.size(), name, v);
				newconst->assign(v);
				constants.insert(std::pair<std::string, unsigned>(name, dag.const_list.size()));
				dag.const_list.emplace_back(newconst);
				return newconst;
			}
		};

		dagc *	mk_bool_decl(const std::string &name);
		dagc *	mk_var_decl(const std::string &name, bool isInt);

		// dagc *	mk_key_bind(const std::string &key, dagc * expr);
		dagc *	mk_let_bind(const std::string &key, dagc * expr);
		dagc *	mk_fun_bind(const std::string &key, dagc * expr);

		dagc *	mk_fun(dagc * fun, const std::vector<dagc* > & params);
		dagc *	mk_fun_post_order(dagc * fun, std::vector<dagc* >& record, const std::vector<dagc* > & params);

		dagc *	mk_and(dagc * l, dagc *r) { std::vector<dagc *> params = { l, r }; return mk_and(params); };
		dagc *	mk_and(const std::vector<dagc *> &params);
		dagc *	mk_or(dagc * l, dagc *r) { std::vector<dagc *> params = { l, r }; return mk_or(params); };
		dagc *	mk_or(const std::vector<dagc *> &params);
		dagc *	mk_not(dagc * param);
		dagc *	mk_not(const std::vector<dagc *> &params);
		dagc *	mk_imply(dagc * l, dagc *r) { std::vector<dagc *> params = { l, r }; return mk_imply(params); };
		dagc *	mk_imply(const std::vector<dagc *> &params);
		dagc *	mk_xor(dagc * l, dagc *r) { return mk_not(mk_eq(l, r)); };
		dagc *	mk_xor(const std::vector<dagc *> &params);

		dagc *	mk_eq(dagc * l, dagc * r);
		dagc *	mk_eq(const std::vector<dagc *> &params);
		dagc *	mk_neq(dagc * l, dagc * r);
		dagc *	mk_distinct(dagc * l, dagc * r) { return mk_neq(l, r); };
		dagc *	mk_distinct(const std::vector<dagc *> &params);

		dagc *	mk_ite(dagc * c, dagc * l, dagc * r);
		dagc *	mk_ite(const std::vector<dagc *> &params);

		dagc *	mk_add(dagc * l, dagc *r) { std::vector<dagc *> params = { l, r }; return mk_add(params); };
		dagc *	mk_add(const std::vector<dagc *> &params);
		dagc *	mk_neg(dagc * param);
		dagc *	mk_minus(dagc * l, dagc *r) { std::vector<dagc *> params = { l, r }; return mk_minus(params); };
		dagc *	mk_minus(const std::vector<dagc *> &params);
		dagc *	mk_mul(dagc * l, dagc *r) { std::vector<dagc *> params = { l, r }; return mk_mul(params); };
		dagc *	mk_mul(const std::vector<dagc *> &params);

		dagc *	mk_div_int(dagc * l, dagc * r);
		dagc *	mk_div_int(const std::vector<dagc *> &params);
		dagc *	mk_div_real(dagc * l, dagc * r);
		dagc *	mk_div_real(const std::vector<dagc *> &params);
		dagc *	mk_mod(dagc * l, dagc * r);
		dagc *	mk_mod(const std::vector<dagc *> &params);

		dagc *	mk_abs(dagc * param);
		dagc *	mk_abs(const std::vector<dagc *> &params);

		dagc *	mk_le(dagc * l, dagc * r);
		dagc *	mk_lt(dagc * l, dagc * r);
		dagc *	mk_ge(dagc * l, dagc * r);
		dagc *	mk_gt(dagc * l, dagc * r);
		dagc *	mk_le(const std::vector<dagc *> &params);
		dagc *	mk_lt(const std::vector<dagc *> &params);
		dagc *	mk_ge(const std::vector<dagc *> &params);
		dagc *	mk_gt(const std::vector<dagc *> &params);

		dagc *	mk_toreal(dagc * param);
		dagc *	mk_toreal(const std::vector<dagc *> &params);
		dagc *	mk_toint(dagc * param);
		dagc *	mk_toint(const std::vector<dagc *> &params);
		dagc *	mk_isint(dagc * param);
		dagc *	mk_isint(const std::vector<dagc *> &params);

		// parse smt-lib2 file
		void 	parse_smtlib2_file(const std::string filename);
		// // parse model file
		void 	parseModel(std::string filename, boost::unordered_map<std::string, vType>& recs);

	private:

		// attributes

		// vars for parser
		char 			*buffer;
		unsigned long	buflen;
		char			*bufptr;
		unsigned int 	line_number;
		SCAN_MODE 		scan_mode;

		boost::unordered_map<std::string, dagc *> key_map;
		std::vector<std::string> vlet_key_list;

		// error node
		dagc *			err_node;
		// const node
		boost::unordered_map<std::string, unsigned> constants;
		
		//methods

		//mk
		dagc *	mk_err(const ERROR_TYPE t) {
			err_node->t = NT_ERROR; err_node->id = t; return err_node;
		};
		dagc * 	mk_oper(const NODE_TYPE t, dagc * p, const std::string &s = "", vType val = 0);
		dagc * 	mk_oper(const NODE_TYPE t, dagc * l, dagc * r, const std::string &s = "", vType  val = 0);
		dagc *	mk_oper(const NODE_TYPE t, std::vector<dagc *> &p, const std::string &s = "", vType val = 0);
		
		dagc *	mk_let(const NODE_TYPE t, std::vector<dagc *> &p, const std::string &s = "", vType val = 0);

		dagc *	mk_eq_bool(dagc * l, dagc * r);

		// parse smtlib2 file
		std::string	get_symbol();
		void 		scan_to_next_symbol();
		void		parse_lpar();
		void 		parse_rpar();
		void		skip_to_rpar();


		CMD_TYPE	parse_command();
		dagc *		parse_expr();
		std::vector<dagc *>	parse_params();
		dagc *		parse_let();

		//errors & warnings
		void 		err_all(const ERROR_TYPE e, const std::string s = "", const unsigned int ln = 0) const;
		void 		err_all(const dagc * e, const std::string s = "", const unsigned int ln = 0) const;

		void 		err_unexp_eof() const;
		void 		err_sym_mis(const std::string mis, const unsigned int ln) const;
		void 		err_sym_mis(const std::string mis, const std::string nm, const unsigned int ln) const;
		void 		err_unkwn_sym(const std::string nm, const unsigned int ln) const;
		void 		err_param_mis(const std::string nm, const unsigned int ln) const;
		void 		err_param_nbool(const std::string nm, const unsigned int ln) const;
		void 		err_param_nnum(const std::string nm, const unsigned int ln) const;
		void 		err_param_nsame(const std::string nm, const unsigned int ln) const;
		void 		err_logic(const std::string nm, const unsigned int ln) const;
		void 		err_mul_decl(const std::string nm, const unsigned int ln) const;
		void 		err_mul_def(const std::string nm, const unsigned int ln) const;
		void 		err_nlinear(const std::string nm, const unsigned int ln) const;
		void 		err_zero_divisor(const unsigned int ln) const;

		void		err_open_file(const std::string) const;

		void 		warn_cmd_nsup(const std::string nm, const unsigned int ln) const;

	};

	
	

}

#endif
