
#include "ThinTargetpCPionReweighter.h"
#include "CentralValuesAndUncertainties.h"
#include "ThinTargetMC.h"
#include "ThinTargetBins.h"

#include <iostream>
#include <cstdlib>
namespace NeutrinoFluxReweight{
  
  ThinTargetpCPionReweighter::ThinTargetpCPionReweighter(int iuniv, const ParameterTable& cv_pars, const ParameterTable& univ_pars):iUniv(iuniv),cvPars(cv_pars),univPars(univ_pars){
    
    ThinTargetBins*  Thinbins =  ThinTargetBins::getInstance();
    vbin_data_pip.reserve(Thinbins->GetNbins_pC_piX_NA49());
    vbin_data_pim.reserve(Thinbins->GetNbins_pC_piX_NA49());
    bart_vbin_data_pip.reserve(Thinbins->GetNbins_pC_piX_Barton());
    bart_vbin_data_pim.reserve(Thinbins->GetNbins_pC_piX_Barton());
    
    // const boost::interprocess::flat_map<std::string, double>& cv_table   = cvPars.getMap();
    // const boost::interprocess::flat_map<std::string, double>& univ_table = univPars.getMap();
    
    data_prod_xs = univPars.getParameterValue("prod_prtC_xsec");
    
    //the number of bins needs to be written from the xmls files 
    char namepar[100];
    for(int ii=0;ii<Thinbins->GetNbins_pC_piX_NA49();ii++){
      
      sprintf(namepar,"ThinTarget_pC_%s_sys_%d","pip",ii);
      double data_cv  = cvPars.getParameterValue(std::string(namepar));
      double data_sys = univPars.getParameterValue(std::string(namepar));
      sprintf(namepar,"ThinTarget_pC_%s_stats_%d","pip",ii);
      double data_sta = univPars.getParameterValue(std::string(namepar));       
      vbin_data_pip.push_back(data_sta + data_sys - data_cv);
      
      sprintf(namepar,"ThinTarget_pC_%s_sys_%d","pim",ii);
      data_cv  = cvPars.getParameterValue(std::string(namepar));
      data_sys = univPars.getParameterValue(std::string(namepar));
      sprintf(namepar,"ThinTarget_pC_%s_stats_%d","pim",ii);
      data_sta = univPars.getParameterValue(std::string(namepar));
      vbin_data_pim.push_back(data_sta + data_sys - data_cv);
      
    }    
    for(int ii=0;ii<Thinbins->GetNbins_pC_piX_Barton();ii++){
      
      sprintf(namepar,"ThinTargetBarton_pC_%s_%d","pip",ii);
      double data_err = univPars.getParameterValue(std::string(namepar));
      bart_vbin_data_pip.push_back(data_err);
      
      sprintf(namepar,"ThinTargetBarton_pC_%s_%d","pim",ii);
      data_err = univPars.getParameterValue(std::string(namepar));      
      bart_vbin_data_pim.push_back(data_err);
      
    } 
    
    aux_par = univPars.getParameterValue("aux_parameter");
    if(aux_par<1.e-15)aux_par = 1.0;

  }
  
   ThinTargetpCPionReweighter::~ThinTargetpCPionReweighter(){
    
  }
  bool ThinTargetpCPionReweighter::canReweight(const InteractionData& aa){
    std::string mode(getenv("MODE"));
    //checking:
    //std::cout<<"ThingTargetpcPionReweighter:: "<<aa.Inc_pdg<<" "<<aa.Vol<<" "<<aa.Prod_pdg<<" "<<std::endl;
    if(aa.Inc_pdg != 2212)return false;
    if(aa.Inc_P < 12.0)return false;
    //volume check: 
    bool is_wrong_volume = aa.Vol != "TGT1" && aa.Vol != "BudalMonitor" && aa.Vol != "Budal_HFVS" && aa.Vol != "Budal_VFHS";
    if( (mode=="REF") || (mode=="OPT") ){
      is_wrong_volume = aa.Vol != "TargetFinHorizontal" && aa.Vol != "TargetNoSplitSegment" && aa.Vol!="tCoreLog";
    }
    if(is_wrong_volume)return false;
    //
    if(aa.Prod_pdg != 211 && aa.Prod_pdg != -211)return false;
    
    ThinTargetBins*  Thinbins =  ThinTargetBins::getInstance();
    int bin      = Thinbins->BinID_pC_pi(aa.xF,aa.Pt,aa.Prod_pdg);
    int bart_bin = Thinbins->barton_BinID_pC_pi(aa.xF,aa.Pt,aa.Prod_pdg);
    
    if(bin<0 && bart_bin<0)return false;
    else return true;
  }
  
  double ThinTargetpCPionReweighter::calculateWeight(const InteractionData& aa){
    
    double wgt = 1.0;
    double low_value = 1.e-18; 
    ThinTargetBins*  Thinbins =  ThinTargetBins::getInstance();
    int bin = Thinbins->BinID_pC_pi(aa.xF,aa.Pt,aa.Prod_pdg);
    int bart_bin = Thinbins->barton_BinID_pC_pi(aa.xF,aa.Pt,aa.Prod_pdg);
    if(bin<0 && bart_bin<0)return aux_par;
    
    //Calculating the scale:
    double data_scale = calculateDataScale(aa.Inc_pdg,aa.Inc_P,aa.Prod_pdg,aa.xF,aa.Pt);
    double dataval = -1;
    if(aa.Prod_pdg == 211 && bin>=0)           dataval = vbin_data_pip[bin];
    else if(aa.Prod_pdg == 211 && bart_bin>=0) dataval = bart_vbin_data_pip[bart_bin];
    else if(aa.Prod_pdg == -211 && bin>=0)     dataval = vbin_data_pim[bin];
    else if(aa.Prod_pdg == -211 && bart_bin>=0)dataval = bart_vbin_data_pim[bart_bin];
    else{
      //  std::cout<<"Something is wrong, pdg_prod: "<< aa.Prod_pdg  <<std::endl;
      return aux_par;
    }

    //checking if this is the first interaction:
    if(aa.gen==0)dataval /= data_prod_xs;
    else if(aa.gen>0)dataval /= 1.0;
    else{
      std::cout<<"Something is wrong with gen "<<std::endl;
      return aux_par;
    }

    dataval *= data_scale;

    ThinTargetMC*  mc =  ThinTargetMC::getInstance();    
    double mc_cv = mc->getMCval_pC_X(aa.Inc_P,aa.xF,aa.Pt,aa.Prod_pdg); 
    double mc_prod = mc->getMCxs_pC_piK(aa.gen,aa.Inc_P);
    mc_cv /= mc_prod;
   // std::cout<<aa.Prod_pdg<<" "<<aa.Pt<<" "<<mc_cv<<" "<<dataval/mc_cv<<std::endl;
    if(mc_cv<1.e-12)return wgt;
    wgt = dataval/mc_cv;
    if(wgt<low_value){
    
      //std::cout<<"TTPCPI check wgt(<0) "<<iUniv<<" "<<aa.Inc_P<<" "<<aa.xF<<" "<<aa.Pt<<" "<<aa.Prod_pdg<<std::endl;
      return aux_par;
    }
    return wgt;
  }
  
  double ThinTargetpCPionReweighter::calculateDataScale(int inc_pdg, double inc_mom, int prod_pdg,double xf, double pt){
    double scaling_violation = 1.0;
    ThinTargetMC*  dtH =  ThinTargetMC::getInstance();
    //temporary:
    const int Nscl = 11;
    const int moms[Nscl] = {12,20,31,40,50,60,70,80,100,120,158};
    
    int idx_part = -1;
    if(prod_pdg == 211)idx_part = 0;
    if(prod_pdg ==-211)idx_part = 1;
    if(idx_part<0){
      std::cout<<"Error in the prod particle"<<std::endl;
      return 1.0;
    }
    
    int binid = dtH->hTTScl[idx_part][Nscl-1]->FindBin(xf,pt);
    double scl_ref158 = dtH->hTTScl[idx_part][Nscl-1]->GetBinContent(binid);    
    
    int idx_lowp = -1;
    int idx_hip  = -1;
    for(int i=0;i<Nscl-1;i++){
      if(inc_mom>=double(moms[i]) && inc_mom<double(moms[i+1])){
	idx_lowp=i;
	idx_hip =i+1;
      }
    }
    if(idx_lowp<0 || idx_hip<0){
      std::cout<<"Error calculating the scaling"<<std::endl;
      return 1.0;
    }
    double scl_low = dtH->hTTScl[idx_part][idx_lowp]->GetBinContent(binid);
    double scl_hi  = dtH->hTTScl[idx_part][idx_hip]->GetBinContent(binid);
    double scl_m   =  scl_low + (inc_mom-double(moms[idx_lowp]))*(scl_hi-scl_low)/(double(moms[idx_hip])-double(moms[idx_lowp]));
    if(scl_ref158<1.e-10){
      // std::cout<<"ref158 zero!!! "<<scl_ref158<<std::endl;
      return 1.0;
    }
    scaling_violation = scl_m/scl_ref158;
    return scaling_violation;
  }

}
