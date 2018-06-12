#include <fnmatch.h>
#include <dirent.h>
#include <sys/types.h>
#include <cerrno>
#include <vector>
#include <string>
#include <iostream>

#include "latticetester/Types.h"
#include "latticetester/Util.h"
#include "latticetester/Const.h"
#include "latmrg/LatConfig.h"
#include "latmrg/ParamReaderLat.h"
#include "latmrg/KorobovLattice.h"
#include "latticetester/Rank1Lattice.h"
#include "latmrg/MRGLatticeFactory.h"
#include "latmrg/MRGLattice.h"
#include "latmrg/MRGLatticeLac.h"
#include "latmrg/MMRGLattice.h"
#include "latmrg/LatTestAll.h"
#include "latmrg/LatTestBeyer.h"
#include "latmrg/LatTestSpectral.h"
#include "latmrg/LatTestPalpha.h"
#include "latmrg/Formatter.h"
#include "latmrg/WriterRes.h"
#include "latmrg/ReportHeaderLat.h"
#include "latmrg/ReportFooterLat.h"
#include "latmrg/ReportLat.h"
#include "latticetester/Normalizer.h"
#include "latticetester/NormaPalpha.h"
#include "latmrg/TestProjections.h"

using namespace std;
// using namespace NTL;
using namespace LatMRG;
using namespace LatticeTester;


//==========================================================================

namespace
{

  int getDir (string dir, std::vector <string> & files)
  {
    DIR *dp;
    struct dirent *dirp;
    if ((dp = opendir (dir.c_str())) == NULL) {
      cerr << "Directory: " << dir << endl;
      perror ("Couldn't open the directory");
      return errno;
    }

    // Does directory name ends with /
    size_t j = dir.rfind('/');
    string SEP("");
    // if not, add one /
    if (dir.size() != (1 + j))
      SEP += "/";

    while ((dirp = readdir (dp)) != NULL) {
      if (0 == fnmatch("*.dat", dirp->d_name, 0))
        // keeps full name including directory name
        files.push_back (string (dir + SEP + dirp->d_name));
    }
    closedir (dp);
    return 0;
  }


  void eraseExtension (std::vector <string> & files)
  {
    for (unsigned int i = 0; i < files.size (); i++) {
      size_t j = files[i].rfind(".dat");
      if (j != string::npos)
        files[i].erase(j);
    }
  }


  void printFileNames (std::vector <string> & files)
  {
    cout << "----------------------------------------------" << endl;
    for (unsigned int i = 0; i < files.size (); i++) {
      cout << files[i] << endl;
    }
  }

}   // namespace


//==========================================================================

namespace LatMRG
{


  //==========================================================================

  Writer* LatTestAll::createWriter (const char *infile, OutputType ot)
  {
    Writer *rw = 0;
    string fname;

    switch (ot) {
      case RES:
        fname = infile;
        fname += ".res";
        rw = new WriterRes (fname.c_str ());
        break;

      case TEX:
        fname = infile;
        fname += ".tex";
        // rw = new WriterTex(fname.c_str()); //EB Ne permet pas d'écrire en Tex
        break;

      case TERMINAL:
        rw = new WriterRes (&cout);
        break;

      default:
        cerr << "\n*** outputType:   no such case" << endl;
        return 0;
    }
    return rw;
  }


  //==========================================================================

  /*
   * Reads the test parameters in infile; then do the test.
   * infile is the data file name without extension: if the data file is named
   * "poil.dat", then infile is "poil".
   * Data files must always have the extension "dat".
   */
  int LatTestAll::doTest (const char *infile)
  {
    // Lecture des paramètres
    string fname (infile);
    fname += ".dat";
    ParamReaderLat paramRdr (fname.c_str ());
    fname.clear ();

    LatConfig<MScal> config;
    paramRdr.read (config);
    //config.write();

    Writer* rw = createWriter (infile, config.outputType);

    LatticeTester::IntLattice<MScal, BScal, BVect, BMat, NScal, NVect, RScal> *lattice = 0;
    LatticeTester::IntLattice<MScal, BScal, BVect, BMat, NScal, NVect, RScal> *master = 0;
    Lacunary<BScal, BVect> *plac = 0;
    bool stationary = true;
    int toDim = config.td[1];
    int fromDim = config.td[0];
    bool memLacF = true; // Lacunary with only used lines-columns of bases
    //memLacF = false; // Lacunary with all lines-columns of bases

    if (config.J > 1) { //Several MRG
      lattice = MRGLatticeFactory<MScal>::fromCombMRG (config.comp, config.J,
          toDim, 0, config.latType, config.norm);

    } else {
      if (config.latType == PRIMEPOWER) {
        config.comp[0]->module.reduceM (config.comp[0]->a[0]);
        if (memLacF && config.lacunary)
          lattice = new MRGLatticeLac<MScal> (config.comp[0]->module.mRed,
              config.comp[0]->a, toDim, config.comp[0]->k, config.Lac,
              config.latType, config.norm);
        else{
          lattice = new MRGLattice<MScal> (config.comp[0]->module.mRed,
              config.comp[0]->a, toDim, config.comp[0]->k,
              config.latType, config.norm);

        }

      } else if (config.genType[0] == MRG || config.genType[0] == LCG) {
        if (memLacF && config.lacunary){
          lattice = new MRGLatticeLac<MScal> (config.comp[0]->module.mRed,
              config.comp[0]->a, toDim, config.comp[0]->k, config.Lac,
              config.latType, config.norm);
        }
        else{
          lattice = new MRGLattice<MScal> (config.comp[0]->module.mRed,
              config.comp[0]->a, toDim, config.comp[0]->k,
              config.latType, config.norm);
        }

      } else if (config.genType[0] == KOROBOV) {
        lattice = new KorobovLattice<MScal> (config.comp[0]->getM (),
            config.comp[0]->a[1], toDim, config.norm);
      } else if (config.genType[0] == RANK1) {
        stationary = false;
        lattice = new Rank1Lattice<MScal, MVect, BScal, BVect, BMat, NScal, NVect, RScal> (config.comp[0]->getM (),
            config.comp[0]->a, config.comp[0]->k, config.norm);


      } else if (config.genType[0] == MMRG) {

        if (memLacF && config.lacunary) {
          lattice = new MMRGLattice<MScal> (config.comp[0]->getM(), config.comp[0]->A,
              toDim,config.comp[0]->k, config.lacunaryType,
              config.Lac, config.norm);           
        } else {
          lattice = new MMRGLattice<MScal> (config.comp[0]->getM(), config.comp[0]->A,
              toDim,config.comp[0]->k, config.norm);
        }
      }
    }

    ReportHeaderLat header (rw, &config, lattice);
    ReportFooterLat footer (rw);
    ReportLat report (rw, &config, &header, &footer);

    double minVal[1 + toDim];
    SetZero (minVal, toDim);

    Normalizer<RScal> *normal = 0;

    if (config.criter == SPECTRAL) {
      normal = lattice->getNormalizer (config.norma, 0, config.dualF);
      // creates and returns the normalizer corresponding to config.norma
      normal->setNorm (config.norm);
    } else if (config.criter == PALPHA &&
        (config.calcPalpha == NORMPAL || config.calcPalpha == BAL)) {
      normal = new NormaPalpha<MScal, RScal> (lattice->getModulo(), config.alpha, toDim);
    }

    if (!memLacF && config.lacunary) {
      plac = new Lacunary<BScal, BVect> (config.Lac, toDim);
      lattice->setLac (*plac);
    }

    switch (config.criter) {
      case SPECTRAL: {
                       LatTestSpectral spectralTest (normal, lattice);
                       lattice->buildBasis (fromDim - 1);
                       spectralTest.attach (&report);

                       report.printHeader ();

                       spectralTest.setDualFlag (config.dualF);
                       spectralTest.setInvertFlag (config.invertF);
                       spectralTest.setDetailFlag (config.detailF);
                       spectralTest.setMaxAllDimFlag (true);
                       spectralTest.setMaxNodesBB (config.maxNodesBB);
                       if (1 == config.d) {
                         spectralTest.test (fromDim, toDim, minVal);
                         // lattice->write();
                         footer.setLatticeTest (&spectralTest);
                         report.printTable ();
                         report.printFooter ();

                       } else {
                         if (config.genType[0] == MRG || config.genType[0] == LCG)
                           master = new MRGLattice<MScal> (*(MRGLattice<MScal> *) lattice);
                         else if (config.genType[0] == KOROBOV)
                           master = new KorobovLattice<MScal> (*(KorobovLattice<MScal> *) lattice);
                         else if (config.genType[0] == RANK1)
                           master = new Rank1Lattice<MScal, MVect, BScal, BVect, BMat, NScal, NVect, RScal> (*(Rank1Lattice<MScal, MVect, BScal, BVect, BMat, NScal, NVect, RScal> *) lattice);

                         master->buildBasis (toDim);
                         TestProjections proj (master, lattice, &spectralTest, config.td,
                             config.d);
                         proj. setOutput (rw);
                         // proj.setDualFlag (config.dualF);
                         proj.setPrintF (true);
                         double merit = proj.run (stationary, false, minVal);
                         int nbProj = proj.getNumProjections ();
                         rw->writeString ("\nMin merit:   ");
                         rw->writeDouble (sqrt (merit));
                         rw->newLine ();
                         rw->writeString ("Num projections:   ");
                         rw->writeInt (nbProj);
                         rw->newLine ();
                         // nbProj = proj.calcNumProjections(stationary, false);
                         // cout << "Num projections2:  " << nbProj << endl << endl;
                         delete master;
                       }
                     }
                     break;

      case BEYER: {
                    LatTestBeyer<MScal, BScal, BVect, BMat, NScal, NVect, RScal> beyerTest (lattice);
                    lattice->buildBasis (fromDim - 1);
                    beyerTest.attach (&report);
                    report.printHeader ();
                    beyerTest.setDualFlag (config.dualF);
                    beyerTest.setMaxAllDimFlag (true);
                    beyerTest.setMaxNodesBB (config.maxNodesBB);
                    beyerTest.setDetailFlag (config.detailF);
                    beyerTest.test (fromDim, toDim, minVal);
                    footer.setLatticeTest (&beyerTest);
                    report.printTable ();
                    report.printFooter ();
                    //rw->writeString (lattice->toStringDualBasis ());
                  }
                  break;

      case PALPHA: {
                     LatTestPalpha<MScal> palphaTest (normal, lattice);
                     palphaTest.setConfig (&config);
                     palphaTest.attach (&report);
                     report.printHeader ();
                     if (1 == config.d) {
                       palphaTest.test (fromDim, toDim, minVal);
                       footer.setLatticeTest (&palphaTest);
                       report.printTable ();
                       report.printFooter ();
                     } else {
                       MRGLattice<MScal> master = MRGLattice<MScal> (*(MRGLattice<MScal> *) lattice);
                       master.buildBasis (toDim);
                       TestProjections proj (&master, lattice, &palphaTest, config.td,
                           config.d);
                       proj. setOutput (rw);
                       double merit = proj.run (true, false, minVal);
                       int nbProj = proj.getNumProjections ();
                       rw->writeString ("\n\nMin merit:   ");
                       rw->writeDouble (sqrt (merit));
                       rw->newLine ();
                       rw->writeString ("Num projections:   ");
                       rw->writeInt (nbProj);
                       rw->newLine ();
                       rw->newLine ();
                     }
                   }
                   break;

      default:
                   cerr << "Default case for config.criter" << endl;
                   return -1;
    }

    if (normal != 0)
      delete normal;
    if (!memLacF && config.lacunary)
      delete plac;
    delete lattice;
    delete rw;
    return 0;
  }


  //==========================================================================

  int LatTestAll::doTestDir (const char *dirname)
  {
    string dir = string (dirname);
    std::vector <string> files = std::vector <string> ();

    getDir (dir, files);
    printFileNames (files);
    eraseExtension (files);

    int flag = 0;
    for (unsigned int i = 0; i < files.size (); i++)
      flag |= doTest (files[i].c_str());

    return flag;
  }


  //==========================================================================

}
