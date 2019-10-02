#ifndef LATMRG_SEEK_H
#define LATMRG_SEEK_H

// Number of generators to generate before testing time in nextGenerator
#define DELAY 1000

template<typename Int, typename Dbl> struct SeekMain {
  typedef NTL::vector<Int> IntVec;
  typedef NTL::matrix<Int> IntMat;

  ConfigSeek<Int, Dbl> conf;

  // Program global objects
  Chrono timer; // program timer

  /**
   * A small class to search for modulus for MWC generators.
   * */
  class Modulus {
    public:

      /**
       * Search will begin at `2^e±c` and will increase of decrease depending on
       * `increase`.
       * */
      Modulus(long e, Int c, bool increase) {
        m = (Int(1)<<e) - 1;
        this->increase = increase;
        if (increase) m+=c;
        else m-=c;
      }

      /**
       * This function finds the next value for `m`. This can return 1 if
       * `increase` is `false` and `m` gets to 1.
       * */
      Int next() {
        while (m > 1) {
          nextM();
          LatticeTester::PrimeType status = LatticeTester::IntFactor<Int>::isPrime (m, KTRIALS);
          if (status == LatticeTester::PRIME || status == LatticeTester::PROB_PRIME) {
            if (1 == m % 4) continue;
            Int m1 = (m - 1)/2;
            status = LatticeTester::IntFactor<Int>::isPrime (m1, KTRIALS);
            if (status != LatticeTester::PRIME && status != LatticeTester::PROB_PRIME) continue;
            // For MWC we check 2^{(m-1)/2} \neq 1 mod m.
            NTL::ZZ_p::init(NTL::ZZ(m));
            NTL::ZZ_p a = NTL::power(NTL::ZZ_p(2), m1);
            if (NTL::IsOne(a)) {
              continue;
            }
            return m;
          }
        }
        return Int(-1);
      }

    private:

      static const long KTRIALS = 200;

      /**
       * Last valid modulus found (or 2^e).
       * */
      Int m;

      /**
       * As passed to constructor.
       * */
      bool increase;

      /**
       * Increment/decrement m
       * */
      void nextM () {
        if (increase) m += 2;
        else m -= 2;
        if (0 == m % 5){
          if (increase) m += 2;
          else m -= 2;
        }
      }
  } /**mod*/;

  /*
   * The goal is to create this overload and to use it to switch generators
   * without requiring the use of switch statements.
   * */
  MRGLattice<Int, Dbl>* nextGenerator(MRGLattice<Int, Dbl>* lattice) {
    // Setting up two vectors. MRGComponent and MRGLattice do not use the same
    // vector format
    IntVec A;
    A.SetLength(conf.fact[0]->getK()+1);
    NTL::clear(A);
    int delay = 0;
    // The program will not run the maxPeriod function if it is not wanted with
    // this condition
    do {
      if (delay >= DELAY) {
        if (timer.timeOver(conf.timeLimit)) return NULL;
        else delay = 0;
      }
      for (long i = 0; i<conf.fact[0]->getK(); i++) A[i+1] = conf.coeff[0][i] * randInt(Int(0), conf.fact[0]->getM());
      delay++;
    } while ((A[conf.fact[0]->getK()] == 0) || (conf.period[0] && !conf.fact[0]->maxPeriod(A)));
    if (lattice) delete lattice;
    return new MRGLattice<Int, Dbl>(conf.fact[0]->getM(), A, conf.max_dim, conf.fact[0]->getK(), FULL);
  }

  MRGLattice<Int, Dbl>* nextGeneratorPow2(MRGLattice<Int, Dbl>* lattice) {
    // Setting up two vectors. MRGComponent and MRGLattice do not use the same
    // vector format
    IntVec A;
    A.SetLength(conf.fact[0]->getK()+1);
    NTL::clear(A);
    IntVec coefficients(2*conf.fact[0]->getK());
    int sign;
    int delay = 0;
    // The program will not run the maxPeriod function if it is not wanted with
    // this condition
    do {
      if (delay >= DELAY) {
        if (timer.timeOver(conf.timeLimit)) return NULL;
        else delay = 0;
      }
      for (long i = 0; i<conf.fact[0]->getK(); i++) {
        if (conf.coeff[0][2*i] < 0) {
          // This is a placeholder value for a zero coefficient
          coefficients[2*i] = coefficients[2*i+1] = 2004012;
          A[i+1] = 0;
          continue;
        }
        coefficients[2*i] = randInt(Int(LatticeTester::Lg(conf.fact[0]->getR()))+1, conf.coeff[0][2*i]);
        sign = randInt(0,1);
        {
          Int tmp;
          NTL::power2(tmp, coefficients[2*i]);
          A[i+1] = Int(sign?1:-1) * tmp;
        }
        coefficients[2*i] ^= sign<<30;
        if (!(conf.coeff[0][2*i+1] < 0)) {
          coefficients[2*i+1] = randInt(Int(LatticeTester::Lg(conf.fact[0]->getR()))+1, conf.coeff[0][2*i+1]);
          sign = randInt(0,1);
          Int tmp;
          NTL::power2(tmp, coefficients[2*i+1]);
          A[i+1] += Int(sign?1:-1) * tmp;
          coefficients[2*i+1] ^= (sign<<30);
        }
        else coefficients[2*i+1] = 2004012;
      }
      delay++;
    } while ((A[conf.fact[0]->getK()] == 0) || (conf.period[0] && !conf.fact[0]->maxPeriod(A)));
    if (lattice) delete lattice;
    MRGLattice<Int, Dbl>* lat = new MRGLattice<Int, Dbl>(conf.fact[0]->getM(), A, conf.max_dim, conf.fact[0]->getK(), FULL);
    lat->setPower2(coefficients);
    return lat;
  }

  MWCLattice<Int, Dbl>* nextGenerator(MWCLattice<Int, Dbl>* lattice) {
    Int m(0);
    long exp = conf.fact[0]->getE()-1;
    // 63 bits at a time because NTL converts from SIGNED long
    while(exp > 0) {
      if (exp < 63) {
        m = (m << exp) + LatticeTester::RandBits(exp);
        exp -= exp;
      }
      m = (m << 63) + LatticeTester::RandBits(63);
      exp -= 63;
    }
    if ((m&1) == 1) m+=1;
    Modulus mod(conf.fact[0]->getE(), m, true);
    if (lattice) delete lattice;
    return new MWCLattice<Int, Dbl>(conf.b, mod.next());
  }

  /*
   * Goin' full random for now
   * */
  MMRGLattice<Int, Dbl>* nextGenerator(MMRGLattice<Int, Dbl>* lattice) {
    IntMat A;
    A.SetDims(conf.fact[0]->getK(), conf.fact[0]->getK());
    NTL::clear(A);
    int delay = 0;
    do {
      if (delay >= DELAY) {
        if (timer.timeOver(conf.timeLimit)) return NULL;
        else delay = 0;
      }
      for (long i = 0; i<conf.fact[0]->getK()-1; i++) {
        A[i][i+1] = Int(1);
      }
      for (long i = 0; i<conf.fact[0]->getK(); i++) {
        A[conf.fact[0]->getK()-1][i] = randInt(Int(0), conf.fact[0]->getM());
      }
      delay++;
    } while ((NTL::determinant(A) == 0) || (conf.period[0] && !conf.fact[0]->maxPeriod(A)));
    // Correcting the matrix to a full matrix
    //for (int i = 0; i<conf.fact[0]->getK(); i++) A *= A;
    for (int i = 0; i<conf.fact[0]->getK(); i++)
      for (int j = 0; j<conf.fact[0]->getK(); j++)
        A[i][j] = A[i][j]%conf.fact[0]->getM();
    if (lattice) delete lattice;
    return new MMRGLattice<Int, Dbl>(conf.fact[0]->getM(), A, conf.max_dim, conf.fact[0]->getK());
  }

  ComboLattice<Int, Dbl>* nextGenerator(ComboLattice<Int, Dbl>* lattice) {
    // Setting up two vectors. MRGComponent and MRGLattice do not use the same
    // vector format
    for (int k = 0; k < conf.num_comp; k++) {
      IntVec A;
      A.SetLength(conf.fact[k]->getK()+1);
      NTL::clear(A);
      IntVec coefficients(2*conf.fact[k]->getK());
      int delay = 0;
      // The program will not run the maxPeriod function if it is not wanted with
      // this condition
      do {
        if (delay >= DELAY) {
          if (timer.timeOver(conf.timeLimit)) return NULL;
          else delay = 0;
        }
        for (long i = 0; i<conf.fact[k]->getK(); i++) A[i+1] = conf.coeff[k][i] * randInt(Int(0), conf.fact[k]->getM());
        delay++;
      } while ((A[conf.fact[k]->getK()] == 0) || (conf.period[k] && !conf.fact[k]->maxPeriod(A)));
      IntVec B;
      B.SetLength(conf.fact[k]->getK());
      for (int i = 0; i < conf.fact[k]->getK(); i++) B[i] = A[i+1];
      conf.fact[k]->setA(B);
    }
    if (lattice) delete lattice;
    MRGLattice<Int, Dbl>* mrg_lat = getLatCombo<Int, Dbl>(conf.fact, conf.max_dim);
    ComboLattice<Int, Dbl>* new_lat = new ComboLattice<Int, Dbl>(conf.fact, *mrg_lat);
    delete mrg_lat;
    return new_lat;
  }

  /*
   * These next function add the tested lattices to the list of the best ones.
   * This only add the lattices that are good enough.
   * */
  template<typename Lat>
    void printResults(MeritList<Lat>& bestLattice) {
      std::cout << "\nSeek: A search program for Random Number Generators\n";
      std::cout << delim;
      std::cout << ((conf.num_comp>1)?"Combined generators":"Simple generator")
        << " configuration" << ((conf.num_comp>1)?"s":"") << "\n\n";
      for (int k = 0; k < conf.num_comp; k++) {
        if (k > 0) std::cout << "\n";
        if (conf.num_comp >1) std::cout << "Component " << k+1 << ":\n";
        std::cout << "Generator type: " << toStringGen(conf.fact[k]->get_type()) << "\n";
        if (conf.fact[k]->get_type() == MRG) {
          std::cout << "Modulus:        m = " << conf.fact[k]->getM() << " = " << conf.fact[k]->getB() << "^"
            << conf.fact[k]->getE();
          if (conf.fact[k]->getR() > 0) std::cout << "+" << conf.fact[k]->getR();
          if (conf.fact[k]->getR() < 0) std::cout << conf.fact[k]->getR();
          std::cout << "\n";
          std::cout << "Order:          k = " << conf.fact[k]->getK() << "\n";
        } else if (conf.fact[k]->get_type() == MWC) {
        } else if (conf.fact[k]->get_type() == MMRG) {
        }
        std::cout << (conf.period[0]?"Full":"Any") << " period length\n";
      }
      std::cout << "\nTest:\n" << (conf.best?"Best":"Worst") << " generators "
        "ranked by ";
      if(conf.criterion == LatticeTester::SPECTRAL) std::cout
        << (conf.normaType==LatticeTester::NONE?"minimal inverse shortest "
            "non-zero vector length":"Spectral Test") << "\n";
      else if (conf.criterion == LatticeTester::LENGTH) std::cout << "minimal"
        << " shortest non-zero vector length\n";
      else if (conf.criterion == LatticeTester::BEYER) std::cout << "their Beyer quotient\n";
      if (conf.normaType != LatticeTester::NONE) {
        std::cout << "Normalizer used: "
          << LatticeTester::toStringNorma(conf.normaType) << "\n";
      }
      std::cout << "\nDimensions and projections:\n";
      std::cout << conf.proj->toString();
      std::cout << delim;
      std::cout << "Allowed running time: " << conf.timeLimit << "s.\n";
      std::cout << "Actual CPU time: " << timer.toString() << "\n";
      std::cout << "Number of generators kept: " << conf.max_gen << "\n";
      std::cout << "Number of generators tested: " << conf.num_gen << "\n\n";
      std::cout << "Retained generators (from best to worst):\n";
      for (auto it = bestLattice.getList().begin(); it!= bestLattice.getList().end(); it++) {
        std::cout << delim;
        std::cout << (*it).getLattice() << "\n";
        std::cout << (*it).toStringMerit() << "\n";
      }
    }

  int print_progress(int old) {
    int per_80 = timer.val(Chrono::SEC)/conf.timeLimit * 80;
    if (per_80 > 80) per_80 = 80;
    if (per_80 < 0) per_80 = 0;
    // We do not print for no reason as this slows the program a lot.
    if (per_80 <= old) return old;
    std::cout << "[";
    for (int i = 0; i < per_80; i++) std::cout << "#";
    for (int i = per_80; i < 80; i++) std::cout << " ";
    std::cout << "] ";
    std::cout << std::setw(2) << int(per_80/80.0*100) << " %\r" << std::flush;
    return per_80;
  }

  int Seek ()
  {
    if (!conf.gen_set) {
      std::cerr << "No generator set for in seek tag. Aborting.\n";
      return 1;
    }
    if (!(conf.test_set)) {
      std::cerr << "No test set for in seek tag. Aborting.\n";
      return 1;
    }
    if (!conf.proj_set) {
      std::cerr << "No projections set for in seek tag. Aborting.\n";
      return 1;
    }
    // Initializing values
    // Dynamically allocated objects
    timer.init();

    // Launching the tests
    std::cout << "Program progress:\n";
    int old = print_progress(-1);
    if (conf.num_comp > 1) {
      ComboLattice<Int, Dbl>* combolat=0;
      MeritList<ComboLattice<Int, Dbl>> bestLattice(conf.max_gen, conf.best);
      while (!timer.timeOver(conf.timeLimit)) {
        combolat = nextGenerator(combolat);
        if (combolat == NULL) continue;
        bestLattice.add(test_seek(*combolat, conf));
        conf.num_gen++;
        conf.currentMerit = bestLattice.getMerit();
        old = print_progress(old);
      }
      if (combolat) delete combolat;
      printResults(bestLattice);
    } else if (conf.fact[0]->get_type() == MRG) {
      MRGLattice<Int, Dbl>* mrglat = 0;
      MeritList<MRGLattice<Int, Dbl>> bestLattice(conf.max_gen, conf.best);
      while (!timer.timeOver(conf.timeLimit)) {
        if (conf.construction == "POW2") mrglat = nextGeneratorPow2(mrglat);
        else if (conf.construction == "RANDOM") mrglat = nextGenerator(mrglat);
        if (mrglat == NULL) continue;
        bestLattice.add(test_seek(*mrglat, conf));
        conf.num_gen++;
        conf.currentMerit = bestLattice.getMerit();
        old = print_progress(old);
      }
      if (mrglat) delete mrglat;
      printResults(bestLattice);
    } else if (conf.fact[0]->get_type() == MWC) {
      MWCLattice<Int, Dbl>* mwclat = 0;
      MeritList<MWCLattice<Int, Dbl>> bestLattice(conf.max_gen, conf.best);
      while (!timer.timeOver(conf.timeLimit)) {
        mwclat = nextGenerator(mwclat);
        if (mwclat == NULL) continue;
        bestLattice.add(test_seek(*mwclat, conf));
        conf.num_gen++;
        conf.currentMerit = bestLattice.getMerit();
        old = print_progress(old);
      }
      if (mwclat) delete mwclat;
      printResults(bestLattice);
    } else if (conf.fact[0]->get_type() == MMRG) {
      MMRGLattice<Int, Dbl>* mmrglat = 0;
      MeritList<MMRGLattice<Int, Dbl>> bestLattice(conf.max_gen, conf.best);
      while (!timer.timeOver(conf.timeLimit)) {
        mmrglat = nextGenerator(mmrglat);
        if (mmrglat == NULL) continue;
        bestLattice.add(test_seek(*mmrglat, conf));
        conf.num_gen++;
        conf.currentMerit = bestLattice.getMerit();
        old = print_progress(old);
      }
      if (mmrglat) delete mmrglat;
      printResults(bestLattice);
    }
    for (int i = 0; i < conf.num_comp; i++) {
      if (conf.fact[i]) delete conf.fact[i];
    }
    delete conf.proj;
    return 0;
  }
}; // end struct SeekMain
#endif
