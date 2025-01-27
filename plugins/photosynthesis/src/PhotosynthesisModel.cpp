/** \file "PhotosynthesisModel.cpp" Primary source file for photosynthesis plug-in.
    \author Brian Bailey

    Copyright (C) 2018  Brian Bailey

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 2.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

*/

#include "PhotosynthesisModel.h"

using namespace std;
using namespace helios;

PhotosynthesisModel::PhotosynthesisModel( helios::Context* __context ){
  context = __context;

  //default values set here
  model_flag = 1; //empirical model
  
  i_PAR_default = 0;
  TL_default = 300;
  CO2_default = 390;
  gM_default = 1;
  
  
}

int PhotosynthesisModel::selfTest( void ){

  Context context_test;

  uint UUID = context_test.addPatch( make_vec3(0,0,0), make_vec2(1,1) );
  
  PhotosynthesisModel photomodel(&context_test);

  std::vector<float> A;
  
  float Qin[9] = {0, 50, 100, 200, 400, 800, 1200, 1500, 2000};
  A.resize(9);
  
  //Generate a light response curve using empirical model with default parmeters
  for( int i=0; i<9; i++ ){
    context_test.setPrimitiveData(UUID,"radiation_flux_PAR",Qin[i]);
    photomodel.run();
    context_test.getPrimitiveData(UUID,"net_photosynthesis",A[i]);
  }

  //Generate a light response curve using Farquhar model with default parameters

  photomodel.setModelType_Farquhar();
  
  for( int i=0; i<9; i++ ){
    context_test.setPrimitiveData(UUID,"radiation_flux_PAR",Qin[i]);
    photomodel.run();
    context_test.getPrimitiveData(UUID,"net_photosynthesis",A[i]);
    printf("light response: Q=%f; A=%f\n",Qin[i],A[i]);
  }

  //Generate an A vs Ci curve using empirical model with default parameters

  float CO2[9] = {100, 200, 300, 400, 500, 600, 700, 800, 1000};
  A.resize(9);

  context_test.setPrimitiveData(UUID,"radiation_flux_PAR",Qin[8]);
  
  photomodel.setModelType_Empirical();
  for( int i=0; i<9; i++ ){
    context_test.setPrimitiveData(UUID,"air_CO2",CO2[i]);
    photomodel.run();
    context_test.getPrimitiveData(UUID,"net_photosynthesis",A[i]);
  }

  //Generate an A vs Ci curve using Farquhar model with default parameters

  photomodel.setModelType_Farquhar();
  for( int i=0; i<9; i++ ){
    context_test.setPrimitiveData(UUID,"air_CO2",CO2[i]);
    photomodel.run();
    context_test.getPrimitiveData(UUID,"net_photosynthesis",A[i]);
    printf("A-Ci curve: Ci=%f; A=%f\n",CO2[i],A[i]);
  }

  //Generate an A vs temperature curve using empirical model with default parameters

  float TL[7] = {270, 280, 290, 300, 310, 320, 330};
  A.resize(7);

  context_test.setPrimitiveData(UUID,"air_CO2",CO2[3]);
  
  photomodel.setModelType_Empirical();
  for( int i=0; i<7; i++ ){
    context_test.setPrimitiveData(UUID,"temperature",TL[i]);
    photomodel.run();
    context_test.getPrimitiveData(UUID,"net_photosynthesis",A[i]);
  }

  //Generate an A vs temperature curve using Farquhar model with default parameters

  photomodel.setModelType_Farquhar();
  for( int i=0; i<7; i++ ){
    context_test.setPrimitiveData(UUID,"temperature",TL[i]);
    photomodel.run();
    context_test.getPrimitiveData(UUID,"net_photosynthesis",A[i]);
    printf("temperature response: TL=%f; A=%f\n",TL[i]-273,A[i]);
  }
  
  return 0;
}

void PhotosynthesisModel::setModelType_Empirical( void ){
  model_flag=1;
}

void PhotosynthesisModel::setModelType_Farquhar( void ){
  model_flag=2;
}

void PhotosynthesisModel::setModelCoefficients( const EmpiricalModelCoefficients modelcoefficients ){
  empiricalmodelcoeffs = modelcoefficients;
}

void PhotosynthesisModel::setModelCoefficients( const FarquharModelCoefficients modelcoefficients ){
  farquharmodelcoeffs = modelcoefficients;
}

void PhotosynthesisModel::run( void ){
  run(context->getAllUUIDs());
}

void PhotosynthesisModel::run( const std::vector<uint> lUUIDs ){

  for( size_t i=0; i<lUUIDs.size(); i++ ){

    size_t p = lUUIDs.at(i);

    float i_PAR;
    if( context->doesPrimitiveDataExist(p,"radiation_flux_PAR") ){
      context->getPrimitiveData(p,"radiation_flux_PAR",i_PAR);
      i_PAR = i_PAR*4.57; //umol/m^2-s (ref https://www.controlledenvironments.org/wp-content/uploads/sites/6/2017/06/Ch01.pdf)
      if( i_PAR<0 ){
	i_PAR = 0;
	std::cout << "WARNING (runPhotosynthesis): PAR flux value provided was negative.  Clipping to zero." << std::endl;
      }
    }else{
      i_PAR = i_PAR_default;
    }

    float TL;
    if( context->doesPrimitiveDataExist(p,"temperature") ){
      context->getPrimitiveData(p,"temperature",TL);
      if( TL<0 ){
	TL = 0;
	std::cout << "WARNING (runPhotosynthesis): Temperature value provided was negative. Clipping to zero. Are you using absolute temperature units?" << std::endl;
      }
    }else{
      TL = TL_default;
    }

    float CO2;
    if( context->doesPrimitiveDataExist(p,"air_CO2") ){
      context->getPrimitiveData(p,"air_CO2",CO2);
      if( CO2<0 ){
	CO2 = 0;
	std::cout << "WARNING (runPhotosynthesis): CO2 concentration value provided was negative. Clipping to zero." << std::endl;
      }
    }else{
      CO2 = CO2_default;
    }
    
    float gM;
    if( context->doesPrimitiveDataExist(p,"moisture_conductance") ){
      context->getPrimitiveData(p,"moisture_conductance",gM);
      if( gM<0 ){
	gM = 0;
	std::cout << "WARNING (runPhotosynthesis): Moisture conductance value provided was negative. Clipping to zero." << std::endl;
      }
    }else{
      gM = gM_default;
    }

    float A, Ci;
    int limitation_state;

    if( model_flag==2 ){ //Farquhar-von Caemmerer-Berry Model
      A = evaluateFarquharModel( i_PAR, TL, CO2, gM, Ci, limitation_state );
    }else{ //Empirical Model
      A = evaluateEmpiricalModel( i_PAR, TL, CO2, gM );
    }

    if( A==0 ){
      std::cout << "WARNING (PhotosynthesisModel): Solution did not converge for primitive " << p << "." << std::endl;
    }

    context->setPrimitiveData(p,"net_photosynthesis",HELIOS_TYPE_FLOAT,1,&A);

    for( int i=0; i<output_prim_data.size(); i++ ){
      if( output_prim_data.at(i).compare("Ci")==0 && model_flag==2 ){
	context->setPrimitiveData(p,"Ci",Ci);
      }else if( output_prim_data.at(i).compare("limitation_state")==0 && model_flag==2 ){
	context->setPrimitiveData(p,"limitation_state",limitation_state);
      }
    }

  }
  
  
}

float PhotosynthesisModel::evaluateCi_Empirical( const float Ci, const float CO2, const float fL, const float Rd, const float gM ){

    
  //--- CO2 Response Function --- //

  float fC = empiricalmodelcoeffs.kC*Ci/empiricalmodelcoeffs.Ci_ref;

    
  //--- Assimilation Rate --- //

  float A = empiricalmodelcoeffs.Asat*fL*fC-Rd;
      
  //--- Calculate error and update --- //
  
  float resid = 0.75*gM*(CO2-Ci) - A - Rd;


  return resid;

}

float PhotosynthesisModel::evaluateEmpiricalModel( const float i_PAR, const float TL, const float CO2, const float gM ){

  //initial guess for intercellular CO2
  float Ci = CO2;
  
  //--- Light Response Function --- //
  
  float fL = i_PAR/(empiricalmodelcoeffs.theta+i_PAR);
  
  assert( fL>=0 && fL<=1 );
  
  //--- Assimilation Temperature Response Function --- //
  
  float fT = fmax(0.f,pow((TL-empiricalmodelcoeffs.Tmin)/(empiricalmodelcoeffs.Tref-empiricalmodelcoeffs.Tmin),empiricalmodelcoeffs.q)*((1+empiricalmodelcoeffs.q)*empiricalmodelcoeffs.Topt-empiricalmodelcoeffs.Tmin-empiricalmodelcoeffs.q*TL)/((1+empiricalmodelcoeffs.q)*empiricalmodelcoeffs.Topt-empiricalmodelcoeffs.Tmin-empiricalmodelcoeffs.q*empiricalmodelcoeffs.Tref));
  
  //--- Respiration Rate --- //
  
  float Rd = empiricalmodelcoeffs.R*sqrt(TL-273.f)*exp(-empiricalmodelcoeffs.ER/TL);
  
  float Ci_old = Ci;
  float Ci_old_old = 0.95*Ci;
  
  float resid_old = evaluateCi_Empirical( Ci_old, CO2, fL, Rd, gM );
  float resid_old_old = evaluateCi_Empirical( Ci_old_old, CO2, fL, Rd, gM );
  
  float err = 10000, err_max = 0.01;
  int iter = 0, max_iter = 100;
  float resid;
  while( err>err_max && iter<max_iter ){
    
    if( resid_old==resid_old_old ){//this condition will cause NaN
      break;
    }
    
    Ci = fabs((Ci_old_old*resid_old-Ci_old*resid_old_old)/(resid_old-resid_old_old));
    
    resid = evaluateCi_Empirical( Ci, CO2, fL, Rd, gM );
    
    resid_old_old = resid_old;
    resid_old = resid;
    
    err = fabs(resid);
    
    Ci_old_old = Ci_old;
    Ci_old = Ci;
    
    iter++;
    
  }

  float A;
  if( err>err_max ){
    A = 0;
  }else{
    float fC = empiricalmodelcoeffs.kC*Ci/empiricalmodelcoeffs.Ci_ref;
    A = empiricalmodelcoeffs.Asat*fL*fC-Rd;
  }
    
  return A;
  
}

float PhotosynthesisModel::evaluateCi_Farquhar( const float Ci, const float CO2, const float i_PAR, const float TL, const float gM, float& A, int& limitation_state ){

  //molar gas constant (kJ/K/mol)
  float R = 0.0083144598;

  float Rd = farquharmodelcoeffs.Rd*exp(farquharmodelcoeffs.c_Rd-farquharmodelcoeffs.dH_Rd/(R*TL));
  float Vcmax = farquharmodelcoeffs.Vcmax*exp(farquharmodelcoeffs.c_Vcmax-farquharmodelcoeffs.dH_Vcmax/(R*TL));
  float Jmax = farquharmodelcoeffs.Jmax*exp(farquharmodelcoeffs.c_Jmax-farquharmodelcoeffs.dH_Jmax/(R*TL));

  float Gamma = exp(farquharmodelcoeffs.c_Gamma-farquharmodelcoeffs.dH_Gamma/(R*TL));
  float Kc = exp(farquharmodelcoeffs.c_Kc-farquharmodelcoeffs.dH_Kc/(R*TL));
  float Ko = exp(farquharmodelcoeffs.c_Ko-farquharmodelcoeffs.dH_Ko/(R*TL));
  
  float Kco = Kc*(1.f+farquharmodelcoeffs.O/Ko);

  float Wc = Vcmax*Ci/(Ci+Kco);

  float J = Jmax*i_PAR*farquharmodelcoeffs.alpha/(i_PAR*farquharmodelcoeffs.alpha+Jmax);
  float Wj = J*Ci/(4.f*Ci+8.f*Gamma);
  
  A = (1-Gamma/Ci)*fmin(Wc,Wj)-Rd;

  if( Wj<Wc ){ //light limited
    limitation_state = 0;
  }else{ //CO2 limited
    limitation_state = 1;
  }
  
  //--- Calculate error and update --- //
  
  float resid = 0.75*gM*(CO2-Ci) - A;

  return resid;
  
}

float PhotosynthesisModel::evaluateFarquharModel( const float i_PAR, const float TL, const float CO2, const float gM, float& Ci, int& limitation_state ){

  //initial guess for intercellular CO2
  Ci = 1.f;
  
  float A;
  
  float Ci_old = Ci;
  float Ci_old_old = 0.95*Ci;
  
  float resid_old = evaluateCi_Farquhar( Ci_old, CO2, i_PAR, TL, gM, A, limitation_state );
  float resid_old_old = evaluateCi_Farquhar( Ci_old_old, CO2, i_PAR, TL, gM, A, limitation_state );

  float err = 10000, err_max = 0.01;
  int iter = 0, max_iter = 100;
  float resid;
  while( err>err_max && iter<max_iter ){

    if( resid_old==resid_old_old ){//this condition will cause NaN
      err = 0.f;
      break;
    }
    
    Ci = fabs((Ci_old_old*resid_old-Ci_old*resid_old_old)/(resid_old-resid_old_old));

    resid = evaluateCi_Farquhar( Ci, CO2, i_PAR, TL, gM, A, limitation_state );

    resid_old_old = resid_old;
    resid_old = resid;
    
    //err = fabs(resid_old-resid_old_old)/fabs(resid_old_old);
    
    Ci_old_old = Ci_old;
    Ci_old = Ci;

    //err = fabs(Ci_old-Ci_old_old)/fabs(Ci_old_old);
    err = fabs(resid);
    
    iter++;
    
  }

  if( err>err_max && resid_old!=0 ){
    A=0;
  }
    
  return A;
  
}

EmpiricalModelCoefficients PhotosynthesisModel::getEmpiricalModelCoefficients( void ){
  return empiricalmodelcoeffs;
}

FarquharModelCoefficients PhotosynthesisModel::getFarquharModelCoefficients( void ){
  return farquharmodelcoeffs;
}

void PhotosynthesisModel::optionalOutputPrimitiveData( const char* label ){

  if( strcmp(label,"Ci")==0 || strcmp(label,"limitation_state")==0 ){
    output_prim_data.push_back( label );
  }else{
    std::cout << "WARNING (PhotosynthesisModel::optionalOutputPrimitiveData): unknown output primitive data " << label << std::endl;
  }
  
}
