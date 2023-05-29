# BLAN
Bit-bLAsting solving Non-linear integer constraints.

### Getting Started

##### Setup

There are several dependence packages:
- GMP (https://gmplib.org/)
- BOOST (https://www.boost.org/)
- LIBPOLY (https://github.com/SRI-CSL/libpoly)
- CAIDICAL (https://github.com/arminbiere/cadical)

Now, one can compile BLAN via:

`./configure.sh`

The command line will automatically build the tool (maybe `chmod +x configure.sh` first). It first build CADICAL, the backend SAT solver, and then LIBPOLY, a library for manipulating polynomials, and finally build BLAN.
