/* blaster_solver.hpp
*
*  Copyright (C) 2023-2026 Fuqi Jia.
*
*  All rights reserved.
*/

#ifndef _BLASTER_H
#define _BLASTER_H


#include "solvers/sat/sat_solver.hpp"
#include "solvers/blaster/blaster_types.hpp"

#include <vector>
#include <string>
#include <boost/unordered_map.hpp>

using std::to_string;
namespace ismt
{
    typedef enum {Slack, Recursion, Transposition} ConstraintsMode;
    class blaster_solver
    {
    private:
        // solver
        std::string                     sname;
        std::string                     benchmark;
        sat_solver*                     solver = nullptr;
        model                           mdl;
        
        // map from name get variables
        boost::unordered_map<std::string, bvar>     n2var;
        boost::unordered_map<std::string, bvar>     n2int; // int uses its Integer as key, NEW
        boost::unordered_map<std::string, bool_var> n2bool;
        boost::unordered_map<bvar, bool_var>        EqZeroMap;
        boost::unordered_map<bvar, int>             addIntScore;
        boost::unordered_map<bvar, int>             initIntScore;
        boost::unordered_map<int, int>              addBoolScore;
        boost::unordered_map<int, int>              initBoolScore;

        // cache for storing problem variables
        std::vector<bvar>               ProblemVars;
        
        // bit len
        unsigned                        default_len;

        // coding mode
        bool                            add2i;
        ConstraintsMode                 vvEqMode;
        ConstraintsMode                 vvCompMode;
        ConstraintsMode                 viEqMode;
        ConstraintsMode                 viCompMode;

        // eq and comp function pointer
        literal vvEq            (bvar var1, bvar var2);
        literal vvGt            (bvar var1, bvar var2);
        literal vvGe            (bvar var1, bvar var2);
        literal vvNeq           (bvar var1, bvar var2);
        literal viEq            (bvar var, bvar v);
        literal viGt            (bvar var, bvar v);
        literal viGe            (bvar var, bvar v);
        literal viNeq           (bvar var, bvar v);

        // auxiliary number
        unsigned                        auxSize;
        unsigned                        slackSize;

    public:
        blaster_solver          (std::string n = "cadical");
        ~blaster_solver         ();


        // make blast_variables
        bvar mkVar              (const std::string& name);
        bvar mkVar              (const std::string& name, unsigned len);
        bvar mkInnerVar         (const std::string& name, unsigned len); // inner var, not appear in model.
        bvar mkInt              (Integer v);
        bvar mkHolder           (const std::string& name, unsigned len); // NEW, a placeholder.
        bvar blast              (bvar var); // NEW, holder blast to the solver.
        bool_var mkBool         (const std::string& name);
        void addVar             (bvar var); // add a variable to the vector.
        void addVarMap          (bvar var); // add a variable to the map.
        void copy               (bvar var, std::string& name, bvar& target); // deep copy for var.
        void copyInt            (bvar var, bvar& target); // deep copy for var.

        // logic operations
        literal Not             (const literal&  l);
        literal And             (const literals& lits);
        literal And             (const literal& lit1, const literal& lit2);
        literal Or              (const literals& lits);
        literal Or              (const literal& lit1, const literal& lit2);
        literal Ite_bool        (literal cond, literal ifp, literal elp);
        literal EqualBool       (const literal& lit1, const literal& lit2);
        literal NotEqualBool    (const literal& lit1, const literal& lit2);

        // math atom operations
        literal Less            (bvar var1, bvar var2);
        literal Equal           (bvar var1, bvar var2);
        literal Greater         (bvar var1, bvar var2);
        literal NotEqual        (bvar var1, bvar var2);
        literal LessEqual       (bvar var1, bvar var2);
        literal GreaterEqual    (bvar var1, bvar var2);

        // math operations
        bvar Negate             (bvar var);
        bvar Absolute           (bvar var);
        bvar Add                (bvar var1, bvar var2);
        bvar Divide             (bvar var1, bvar var2);
        bvar Modulo             (bvar var1, bvar var2);
        bvar Subtract           (bvar var1, bvar var2);
        bvar Multiply           (bvar var1, bvar var2);
        bvar Ite_num            (literal cond, bvar ifp, bvar elp);

        // solve operations
        int Assert              (literal lit);
        bool solve              ();
        bool solve              (const literals& assumptions);
        bool simplify           ();
        bool simplify           (const literals& assumptions);

        // get operations
        void printModel         ();
        void printStatus        ();
        Integer getValue        (const bvar& var);
        Integer getValue        (const std::string& name);

        // set operations
        void setDeaultLen       (unsigned len);
        void setSlackAddi       (bool s);
        void setvvEqMode        (ConstraintsMode c);
        void setvvCompMode      (ConstraintsMode c);
        void setviEqMode        (ConstraintsMode c);
        void setviCompMode      (ConstraintsMode c);

        // reset: clear sat solver, and blaster variables.
        void reset              ();
        void setBenchmark       (std::string file);

    private:
        // auxiliary make variable functions
        bvar mkInvertedVar          (bvar var);
        bvar mkShiftedVar           (bvar var, int len);
        bool_var newSatVar          (); // NEW
        
        // escape 10000... -2^n
        // variable has range of [1-2^n, 2^n-1]
        void Escape                 (bvar var);

        void swap                   (bvar& a, bvar& b);

        // auxiliary functions
        literal GtZero              (bvar var);
        literal GeZero              (bvar var);
        literal LtZero              (bvar var);
        literal LeZero              (bvar var);
        literal EqZero              (bvar var);
        literal NeqZero             (bvar var);
        // Slack Abstraction: e.g. a > b <=> a == b + t /\ t > 0.
        literal SEqual              (bvar var1, bvar var2);
        literal SNotEqual           (bvar var1, bvar var2);
        literal SGreater            (bvar var1, bvar var2);
        literal SGreaterEqual       (bvar var1, bvar var2);
        literal SEqualInt           (bvar var1, bvar var2);
        literal SNotEqualInt        (bvar var1, bvar var2);
        literal SGreaterInt         (bvar var1, bvar var2);
        literal SGreaterEqualInt    (bvar var1, bvar var2);

        // Recursion Abstraction: e.g. a > b <=> a[1] > a[2] or a[1] == a[2] /\ a[2:] > b[2:]
        literal REqual              (bvar var1, bvar var2);
        literal RNotEqual           (bvar var1, bvar var2);
        literal RGreater            (bvar var1, bvar var2);
        literal RGreaterEqual       (bvar var1, bvar var2);
        literal REqualInt           (bvar var1, bvar var2);
        literal RNotEqualInt        (bvar var1, bvar var2);
        literal RLessInt            (bvar var, bvar v);
        literal RGreaterInt         (bvar var, bvar v);
        literal RCompIntLoop        (bvar var, bvar v, bool greater);
        literal RGreaterEqualInt    (bvar var, bvar v);
        literal RGreaterBit         (const literal& var, const literal& v);
        literal RLessBitInt         (const literal& var, const literal& v);
        literal RGreaterBitInt      (const literal& var, const literal& v);

        // Transposition Abstraction: e.g. a > b <=> a - b > 0
        literal TEqual              (bvar var1, bvar var2);
        literal TNotEqual           (bvar var1, bvar var2);
        literal TGreater            (bvar var1, bvar var2);
        literal TGreaterEqual       (bvar var1, bvar var2);
        literal TEqualInt           (bvar var1, bvar var2);
        literal TNotEqualInt        (bvar var1, bvar var2);
        literal TGreaterInt         (bvar var1, bvar var2);
        literal TGreaterEqualInt    (bvar var1, bvar var2);

        // equal operations
        void innerEqualVar          (bvar var1, bvar var2);
        void innerEqualBit          (literal var1, literal var2);
        void innerEqualInt          (bvar var, bvar v);
        void innerEqualBitInt       (literal var, literal v);
        literal EqualVar            (bvar var1, bvar var2);
        literal EqualBit            (literal var1, literal var2);
        literal EqualInt            (bvar var, bvar v);
        literal EqualBitInt         (literal var, literal v);
        literal NotEqualVar         (bvar var1, bvar var2);
        literal NotEqualBit         (literal var1, literal var2);
        literal NotEqualInt         (bvar var, bvar v);
        literal NotEqualBitInt      (literal var, literal v);

        // auxiliary functions for math operations
        bvar Add                    (bvar var1, bvar var2, bool addone);
        bvar Invert                 (bvar var);
        bvar AddOne                 (bvar var);
        bvar MultiplyInt            (bvar var, bvar v);
        bvar MultiplyBit            (bvar var, literal bit, unsigned len);
        bvar MultiplyBitInt         (bvar var, literal bit, unsigned len); // NEW
        bvar Shift                  (bvar var, int len);
        bvar ShiftAdd               (bvar subsum, bvar subans, bvar answer, unsigned idx);
        // bool_var MultiplySignBit    (bvar var1, bvar var2);

        // inner auxiliary functions for math operations
        void doCarry                (literal cry1, literal varmax, literal varmin, literal cry0);
        void doTarget               (literal target, literal varmax, literal varmin, literal cry);
        void doCarryInt             (literal cry1, literal var, literal ivar, literal cry0);
        void doTargetInt            (literal target, literal var, literal ivar, literal cry);


        // calculate value from bit-blasting
        Integer calculate          (bvar var); // after getModel, calculate real value.

        // get model 
        void getModel               ();

        // delete functions
        void clear                  ();
        void clearVars              ();

        // add clause to sat solver
        void addClause              (clause& c);

        // output error and exit
        void outputError            (const std::string& mesg);
    public:
        // ------------information operations------------
        literal Lit_True            ();
        literal Lit_False           ();
    };

    // ------------outer auxiliary functions------------
    inline int min(int a, int b)    { return a>b?b:a; }
    inline int max(int a, int b)    { return a>b?a:b; }
    inline int abs(int a)           { return a>0?a:-a; }
    
} // namespace ismt


#endif