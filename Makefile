CXX = g++
CXXFLAGS = -Wall -Werror -Wextra -pedantic -std=c++11 -g -I. -fno-omit-frame-pointer -O3
LBLIBS = -L solvers/include -lcadical -lgmp -lgmpxx -lpoly -lpolyxx -lpicpoly -lpicpolyxx
SRC =	main.cpp \
		qfnia/qfnia.cpp qfnia/decider.cpp qfnia/searcher.cpp qfnia/resolver.cpp qfnia/collector.cpp qfnia/checker.cpp\
		frontend/parser.cpp midend/preprocessor.cpp \
		rewriters/comp_rewriter.cpp rewriters/nnf_rewriter.cpp rewriters/prop_rewriter.cpp \
		rewriters/poly_rewriter.cpp rewriters/eq_rewriter.cpp rewriters/logic_rewriter.cpp rewriters/let_rewriter.cpp \
		solvers/poly/poly_solver.cpp solvers/icp/icp_solver.cpp solvers/eq/eq_solver.cpp\
		solvers/blaster/blaster_solver.cpp solvers/blaster/blaster_transposition.cpp solvers/blaster/blaster_slack.cpp \
		solvers/blaster/blaster_recursion.cpp solvers/blaster/blaster_operations.cpp solvers/blaster/blaster_make.cpp \
		solvers/blaster/blaster_logic.cpp solvers/blaster/blaster_equal.cpp solvers/blaster/blaster_comp.cpp \
		solvers/blaster/blaster_transformer.cpp solvers/blaster/blaster_bits.cpp \
		solvers/sat/sat_solver.cpp
OBJ = $(SRC:.cpp=.o)
EXEC = BLAN

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CXX) -o $@ $(OBJ) $(LBLIBS)

clean:
	rm -rf $(OBJ) $(EXEC)