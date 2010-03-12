/**************************************************************************
 * This file is property of and copyright by                              *
 * the Relativistic Heavy Ion Group (RHIG), Yale University, US, 2009     *
 *                                                                        *
 * Primary Author: Per Thomas Hille  <perthomas.hille@yale.edu>           *
 *                                                                        *
 * Contributors are mentioned in the code where appropriate.              *
 * Please report bugs to p.t.hille@fys.uio.no                             *
 *                                                                        *
 * Permission to use, copy, modify and distribute this software and its   *
 * documentation strictly for non-commercial purposes is hereby granted   *
 * without fee, provided that the above copyright notice appears in all   *
 * copies and that both the copyright notice and this permission notice   *
 * appear in the supporting documentation. The authors make no claims     *
 * about the suitability of this software for any purpose. It is          *
 * provided "as is" without express or implied warranty.                  *
 **************************************************************************/

// The Peak-Finder algorithm
// The amplitude is extracted  as a
// weighted sum of the samples using the 
// best possible weights.
// The wights is calculated only once and the
// Actual extraction of amplitude and peak position
// Is done with a simple vector multiplication, allowing for
// Extreemely fast computations. 

#include "AliCaloRawAnalyzerPeakFinder.h"
#include "AliCaloBunchInfo.h"
#include "AliCaloFitResults.h"
#include <iostream>
#include "unistd.h"
#include "TMath.h"
#include "AliLog.h"

using namespace std;

ClassImp( AliCaloRawAnalyzerPeakFinder )

AliCaloRawAnalyzerPeakFinder::AliCaloRawAnalyzerPeakFinder() :AliCaloRawAnalyzer("Peak-Finder", "PF")
//    fTof(0), 
//							      fAmp(0)
{
  //comment

  fNsampleCut = 5;

  for(int i=0; i < MAXSTART; i++)
    {
      for(int j=0; j < SAMPLERANGE; j++ )
	{
	  fPFAmpVectors[i][j] = new double[100];
	  fPFTofVectors[i][j] = new double[100];
	  fPFAmpVectorsCoarse[i][j] = new double[100];
	  fPFTofVectorsCoarse[i][j] = new double[100];

	  for(int k=0; k < 100; k++ )
	    {
	      fPFAmpVectors[i][j][k] = 0; 
	      fPFTofVectors[i][j][k] = 0;
	      fPFAmpVectorsCoarse[i][j][k] = 0;
	      fPFTofVectorsCoarse[i][j][k] = 0; 
	    }
	}
    }

  LoadVectors();

}


AliCaloRawAnalyzerPeakFinder::~AliCaloRawAnalyzerPeakFinder()
{
  //comment
  for(int i=0; i < MAXSTART; i++)
    {
      for(int j=0; j < SAMPLERANGE; j++ )
	{
	  delete[] fPFAmpVectors[i][j];
	  delete[] fPFTofVectors[i][j];
	  delete[] fPFAmpVectorsCoarse[i][j];
	  delete[] fPFTofVectorsCoarse[i][j];
	}
    }
}


Double_t  
AliCaloRawAnalyzerPeakFinder::ScanCoarse(const Double_t *const array, const int length ) const
{
  Double_t tmpTof = 0;
  Double_t tmpAmp= 0;

  for(int i=0; i < length; i++)
    {
      tmpTof += fPFTofVectorsCoarse[0][length][i]*array[i]; 
      tmpAmp += fPFAmpVectorsCoarse[0][length][i]*array[i]; 
    }
  
  tmpTof = tmpTof / tmpAmp ;
  return tmpTof;
}


AliCaloFitResults 
AliCaloRawAnalyzerPeakFinder::Evaluate( const vector<AliCaloBunchInfo> &bunchvector, const UInt_t altrocfg1,  const UInt_t altrocfg2 )
{
  // Extracting the amplitude using the Peak-Finder algorithm
  // The amplitude is a weighted sum of the samples using 
  // optimum weights.

  short maxampindex; //index of maximum amplitude
  short maxamp; //Maximum amplitude
  //  fAmp = 0;

  
  fAmpA[0] = 0;
  fAmpA[1] = 0;
  fAmpA[2] = 0;
  
  
  //  cout << __FILE__ << __LINE__ << "\tendbin = " << bunchvector.at(index).GetEndBin()  <<  "\tstartbin = " << bunchvector.at(index).GetStartBin()  << endl;
  
  int index = SelectBunch( bunchvector,  &maxampindex,  &maxamp );

  if( index >= 0)
    {
      Float_t ped = ReverseAndSubtractPed( &(bunchvector.at(index))  ,  altrocfg1, altrocfg2, fReversed  );
      Float_t maxf = TMath::MaxElement(   bunchvector.at(index).GetLength(),  fReversed );
      short maxrev = maxampindex  -  bunchvector.at(index).GetStartBin();
      short timebinOffset = maxampindex - (bunchvector.at( index ).GetLength()-1); 
	     
      if(  maxf < fAmpCut  ||  ( maxamp - ped) > 900  )	 // (maxamp - ped) > 900 = Close to saturation (use low gain then)
	{
	  //	  cout << __FILE__ << __LINE__ <<":, maxamp = " << maxamp << ", ped = "<< ped  << ",. maxf = "<< maxf << ", maxampindex = "<< maxampindex  << endl;
	  return  AliCaloFitResults( maxamp, ped, AliCaloFitResults::kNoFit, maxf, maxrev+timebinOffset, AliCaloFitResults::kNoFit, AliCaloFitResults::kNoFit );
 	}
      
      int first;
      int last;
      
      if ( maxf > fAmpCut )
	{	  
	  SelectSubarray( fReversed,  bunchvector.at(index).GetLength(), maxrev, &first, &last);
	  int nsamples =  last - first;
	  if( ( nsamples  )  >= fNsampleCut )
	    {
	      int startbin = bunchvector.at(index).GetStartBin();  
	      int n = last - first;  
	      int pfindex = n - fNsampleCut; 
	      pfindex = pfindex > SAMPLERANGE ? SAMPLERANGE : pfindex;

	      int dt =  maxampindex - startbin -2; 

	      //    cout << __FILE__ << __LINE__ <<"\t The coarse estimated t0 is " << ScanCoarse( &fReversed[dt] , n ) << endl;
	    
	 
	      //     Float_t tmptof = ScanCoarse( &fReversed[dt] , n );
	      
	      //	      cout << __FILE__ << __LINE__ << ",  dt = " << dt << ",\tmaxamindex = " << maxampindex << "\tstartbin = "<< startbin << endl;

	      for( int i=0; i < SAMPLERANGE; i++ )
		{
		  for( int j = 0; j < 3; j++ )
		    {
		      //    fAmpA[j] += fPFAmpVectors[0][pfindex][i]*tmp[j]; 
		      fAmpA[j] += fPFAmpVectors[0][pfindex][i]*fReversed[ dt  +i +j -1 ];
		    }
		}
	      
	      double diff = 9999;
	      int tmpindex = 0;

	      for(int k=0; k < 3; k ++)
		{
		  //		  cout << __FILE__ << __LINE__ << "amp[="<< k <<"] = " << fAmpA[k] << endl;
		  if(  TMath::Abs(fAmpA[k] - ( maxamp - ped) )  < diff)
		    {
		      diff = TMath::Abs(fAmpA[k] - ( maxamp - ped));
		      tmpindex = k; 
		    }
		}
	      
	      Float_t tmptof = ScanCoarse( &fReversed[dt] , n );
	      
	      if( tmptof < -1 )
		{
		  tmpindex = 0;
		}
	      else
		if( tmptof  > -1 && tmptof < 100 )
		  {
		    tmpindex =1;
		  }
		else
		  {
		    tmpindex = 2;
		  }
	      double tof = 0;
	      
	      for(int k=0; k < SAMPLERANGE; k++   )
		{
		  tof +=  fPFTofVectors[0][pfindex][k]*fReversed[ dt  +k + tmpindex -1 ];   
		}
	      
	      //	      cout << __FILE__ << __LINE__ <<  "tofRaw =   "<< tof /  fAmpA[tmpindex]  << endl;
	      
	      // tof = tof /  fAmpA[tmpindex] +  (dt + startbin)*100;
	      
	      if( TMath::Abs(  (maxf - fAmpA[tmpindex])/maxf )  >   0.1 )
		{
		  fAmpA[tmpindex] = maxf;
		}

	      //	      timebinOffset

		//      tof = (dt + startbin + tmpindex )*100 - tof/fAmpA[tmpindex];
		// tof = ( timebinOffset )*100 - tof/fAmpA[tmpindex]; // ns
	      tof = timebinOffset - 0.01*tof/fAmpA[tmpindex]; // clock ticks
	      
	      //      tof = tof/fAmpA[tmpindex];

	      
	      return AliCaloFitResults( maxamp, ped , AliCaloFitResults::kDummy, fAmpA[tmpindex], tof, AliCaloFitResults::kDummy, AliCaloFitResults::kDummy,
					AliCaloFitResults::kDummy, AliCaloFitSubarray(index, maxrev, first, last) );  
	    }
	  else
	    {
	      return AliCaloFitResults( maxamp, ped , AliCaloFitResults::kNoFit, maxf, maxrev+timebinOffset, AliCaloFitResults::kNoFit, AliCaloFitResults::kNoFit,
					AliCaloFitResults::kNoFit, AliCaloFitSubarray(index, maxrev, first, last) ); 
	    }
	}
      else 
	{
	  return AliCaloFitResults( maxamp , ped, AliCaloFitResults::kNoFit, maxf, maxrev+timebinOffset, AliCaloFitResults::kNoFit, AliCaloFitResults::kNoFit);
	}
    }
   //  cout << __FILE__ << __LINE__ <<  "WARNING, returning amp = -1 "  <<  endl;
  return  AliCaloFitResults(AliCaloFitResults::kInvalid, AliCaloFitResults::kInvalid);
}


void 
AliCaloRawAnalyzerPeakFinder::LoadVectors()
{
  //Read in the Peak finder vecors from file
  for(int i = 0;  i < MAXSTART ; i++)
    {
      for( int j=0; j < SAMPLERANGE; j++)
	{
	  char filenameCoarse[256];
	  char filename[256];
	  
	  int n = j+fNsampleCut;

	  //	  double start = (double)i+0.5;
	  double start = (double)i+0;

	  sprintf(filename,        "%s/EMCAL/vectors-emcal/start%.1fN%dtau0.235fs10dt1.0.txt", getenv("ALICE_ROOT"), start, n);
	  sprintf(filenameCoarse,  "%s/EMCAL/vectors-emcal/start%.1fN%dtau0.235fs10dt3.0.txt", getenv("ALICE_ROOT"), start, n);

	  FILE *fp  =  fopen(filename, "r");
	  FILE *fpc =  fopen(filenameCoarse, "r");

	  if( fp == 0 )
	    {
	      AliFatal( Form( "could not open file: %s", filename ) );
	    }
	
	  if(fpc == 0)
	    {
	      AliFatal( Form( "could not open file: %s", filenameCoarse ) );
	    }
	  else
	    {
	      for(int m = 0; m < n ; m++ )
		{
		  // cout << __FILE__ << __LINE__ << "i="<<i <<"\tj=" <<j << "\tm=" << m << endl;
 
		  fscanf(fp, "%lf\t", &fPFAmpVectors[i][j][m] );
		  //		  fPFAmpVectorsCoarse[i][j][m] = 1;
		  fscanf(fpc, "%lf\t", &fPFAmpVectorsCoarse[i][j][m] );
		}
	      
	      fscanf(fp,   "\n" );
	      fscanf(fpc,  "\n" );
	      
	      for(int m = 0; m < n ; m++ )
		{
		  //  fPFTofVectors[i][j][m] = 1;

		  fscanf(fp, "%lf\t",   &fPFTofVectors[i][j][m]  );
		  fscanf(fpc, "%lf\t",  &fPFTofVectorsCoarse[i][j][m]  );  
		  //  fPFTofVectorsCoarse[i][j][m] = 1;  
		}
	      
	     
	      fclose (fp);
	      fclose (fpc);
	    }
	}
    }
}



/*
void
AliCaloRawAnalyzerPeakFinder::PolTof( const double rectof ) const
//
{
  static Double_t p0 = -55.69;
  static Double_t p1 = 3.178;
  static Double_t p2 = -0.05587;
  static Double_t p3 = 0.0003185;
  static Double_t p4 = -7.91E-7;
  static Double_t p5 = 7.576E-10;
}
*/
