# BLAN
BLAN: Bit-bLAsting solving Non-linear integer constraints.

### Getting Started

##### Setup
Environment setup:
- gcc/clang (support C++11/C++17)
- cmake
- make

There are several dependence packages:
- GMP (https://gmplib.org/)
- BOOST (https://www.boost.org/)
- LIBPOLY (https://github.com/SRI-CSL/libpoly)
- CAIDICAL (https://github.com/arminbiere/cadical)

Now, one can compile BLAN via:

`./configure.sh`

The command line will automatically build the tool (maybe `chmod +x configure.sh` first). It first build CADICAL, the backend SAT solver, and then LIBPOLY, a library for manipulating polynomials, and finally build BLAN.

One can check parameters supported by the tool via
```
./BLAN -h
```
The output is 
```
BLAN 	test/1.smt2 (the smt-lib file to solve)
	-T:1200 (time limit (s), default 1200)
	-GA:true (Greedy Addtion, default true)
	-MA:true (Multiplication Adaptation, default true)
	-VO:true (Vote, default true)
	-Alpha:1536 (Alpha of Multiplication Adaptation, default 1536)
	-Beta:7 (Beta of Multiplication Adaptation, default 7)
	-K:16 (K of Clip, default 16)
	-Gamma:0.5 (Gamma of Vote, default 0.5)
	-DG:true (Distinct Graph, default true)
	-CM:true (Coefficient Matching, default true)
	-MaxBW:128 (Maximum Bit-Width, default 128)
```

Paper DOI
```
https://doi.org/10.1145/3597926.3598034
```
