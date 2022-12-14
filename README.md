# About LatMRG

**LatMRG** is a sofware package intended as the most up to date and feature
complete tool to search and test **Multiple Recursive** and similar **Random
Number Generators**. The generators considered here are all linear and this
linearity means that they all have a **lattice structure** of some sort.

The goal of this software is to provide reliable theoretical tests of uniformity
for these generators, as well as tools to search for good and bad ones with
regards to these tests. These tests often rely on the length of short(est)
vectors in the lattice, hence the main feature of LatMRG is to be able to
efficiently reduce a lattice, be it via **Branch-and-Bound** to find the
shortest vector or with **LLL** reduction and **Block Korkine-Zolotarev**
reduction.

Once a a short vector has been found, it is possible to compute the **Spectral
Test** and various **Figures of Merit** on a generator. These can be used to
compare the uniformity of the generators.

The family of generators that LatMRG can study is large. You could say that we
cover multiple different families of generators, but all the generators
targeted by LatMRG share similar theoretical properties. Generators we target
are
- Linear Congruential Generators (LCG)
- Multiple Recursive Congruential Generators (MRG)
- Combined Multiple Recursive Congruential Generators
- Add-with-Carry (AWC) and Subtract-with-Burrow (SWB) Generators
- Multiply with Carry (MWC) Generators
- Matrix Multiple Recursive Generators (MMRG)

The documentation of this software is segmented in multiple locations that each
contain different information:
- You can easily [get started](https://umontreal-simul.github.io/LatMRG/usage.html) by
  intalling and using the executables
- [Survey the contents of LatMRG](https://umontreal-simul.github.io/LatMRG) in the full
  guide
- [Read the theory](https://umontreal-simul.github.io/LatMRG/background.html) behind LatMRG
- [Look at examples](https://umontreal-simul.github.io/LatMRG/tutorial.html) and learn
  how LatMRG can be used as a library
- Access the full [API documentation](https://umontreal-simul.github.io/LatMRG/annotated.html) for the library.

### Use appropriate RNGs

There are a plethora of pseudo-random engines described on the web, and even more
implementations of such engines in many programming languages.
These random number generators provide a seemingly good quality of randomness as they
usually pass standard statistical tests well and are quite fast. This makes them
great multi-purpose RNGs, but they should
be used with care. They usually do not have the theoretical background necessary
to assess the uniformity of high dimension vectors generated by taking points
they output sequentially. This can cause problems in Monte Carlo simulations,
notably in physics, finance and statistics. Generators built with LatMRG can be
parametrized as to not have this problem, making them a better choice for this
use case.

# Getting it to work

LatMRG tested to work on Linux only. It should also work without much hasle in
macOS. Since this program is a command line utility, Windows users will probably
be able to use it smoothly once the new
[terminal](https://github.com/microsoft/terminal) launches. Any user getting
the library to run on Windows or macOS is welcome to provide us with his
or her process so that it can be added to this guide.

## Dependencies

LatMRG currently depends on
* [LatticeTester](https://github.com/umontreal-simul/latcommon): a utility library
developped in our laboratory upon which LatMRG builds. This library is bundled
in the repository and automatically compiled with LatMRG.
* [NTL](http://www.shoup.net/ntl/index.html): LatticeTester heavily (and shamelesly)
uses the **Number Theory Library** developped by Victor Shoup. Make sure this is
installed with the NTL_THREADS=off option.
* [gmp](https://gmplib.org/): The GNU multiple precision library, needed by NTL.
This is packaged in most linux distrubtions. Be sure to also install the
library's header files.
* [yafu](https://sourceforge.net/projects/yafu/): this factorization
utility unlocks some of the functionnality of LatMRG! To do so,
simply download the program and extract the `yafu` executable in `./data`. The
makefile will then include a preprocessor definition that will allow factoring.

## Configuring the build

LatMRG currently only has a very simple makefile. If NTL is not installed in a
default prefix such as `/usr/local`, or if you use `clang` instead of `gcc` you
will need to modify it manually before building LatMRG. The following commands
will build the library and executables.
```
git clone --recursive https://github.com/umontreal-simul/LatMRG.git
cd LatMRG/latticetester
./waf configure
cd ..
make
```

This will pull and build the LatMRG library in `./LatMRG/lib`, and the executable
programs in `./LatMRG/bin`. There is currently no way to install LatMRG in
standard path to ease the usage of the library or invoke it via the command line.

### Current maintainer(s)

[Marc-Antoine Savard](https://github.com/savamarc)
