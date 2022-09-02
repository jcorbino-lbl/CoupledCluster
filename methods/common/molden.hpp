
#pragma once

#include <iostream>
#include "json_data.hpp"

string read_option(string line){
  std::istringstream oss(line);
  std::vector<std::string> option_string{
    std::istream_iterator<std::string>{oss},
    std::istream_iterator<std::string>{}};
  // assert(option_string.size() == 2);
  
  return option_string[1];
}

bool is_comment(const std::string line) {
  auto found = false;
  if(line.find("//") != std::string::npos){
    // found = true;
    auto fpos = line.find_first_not_of(' ');
    auto str = line.substr(fpos,2);
    if (str == "//") found = true;
  }
  return found;
}

bool is_in_line(const std::string str, const std::string line){
  auto found = true;
  std::string str_u = str, str_l = str;
  to_upper(str_u); to_lower(str_l);

  if (is_comment(line)) found = false;
  else {
    std::istringstream oss(line);
    std::vector<std::string> option_string{
    std::istream_iterator<std::string>{oss},
    std::istream_iterator<std::string>{}};
    for (auto &x: option_string) 
      x.erase(std::remove(x.begin(),x.end(),' '),x.end());
    
    if (std::find(option_string.begin(),option_string.end(), str_u) == option_string.end()
     && std::find(option_string.begin(),option_string.end(), str_l) == option_string.end() )
     found = false;
  }

  return found;
}

template<typename T>
void reorder_molden_orbitals(const bool is_spherical, std::vector<AtomInfo>& atominfo, Matrix& smat, Matrix& dmat, const bool reorder_cols = true, const bool reorder_rows = true) {
    auto dim1 = dmat.rows();
    auto dim2 = dmat.cols();

    const T sqrt_3 = std::sqrt(static_cast<T>(3.0));
    const T sqrt_5 = std::sqrt(static_cast<T>(5.0));
    const T sqrt_7 = std::sqrt(static_cast<T>(7.0));
    const T sqrt_753 = sqrt_7*sqrt_5/sqrt_3;

    auto col_copy = [&](int tc, int oc, const T scale=1.0) {
      for(auto i=0;i<dim1;i++) {
        dmat(i,tc) = scale * smat(i,oc);
      }
    };
  
    if(reorder_rows) {
      for(auto i=0;i<dim2;i++) {
        size_t j = 0;
        for(size_t x = 0; x < atominfo.size(); x++) { //loop over atoms
          for(auto s: atominfo[x].shells) { //loop over each shell for given atom
            for(const auto& c: s.contr) { //loop over contractions. 
              // FIXME: assumes only 1 contraction for now
              if(c.l == 0) {
                //S functions
                dmat(j,i) = smat(j,i); j++; 
              }
              else if(c.l == 1) {
                //P functions
                //libint set_pure to solid forces y,z,x ordering for l=1
                if(is_spherical) {
                  dmat(j,i) = smat(j+1,i); j++;
                  dmat(j,i) = smat(j+1,i); j++;
                  dmat(j,i) = smat(j-2,i); j++;
                }
                else {
                  dmat(j,i) = smat(j,i); j++;
                  dmat(j,i) = smat(j,i); j++;
                  dmat(j,i) = smat(j,i); j++;
                }
              }
              else if(c.l == 2) {
                //D functions
                if(is_spherical) {
                  dmat(j,i) = smat(j+4,i); j++;
                  dmat(j,i) = smat(j+1,i); j++;
                  dmat(j,i) = smat(j-2,i); j++;
                  dmat(j,i) = smat(j-2,i); j++;
                  dmat(j,i) = smat(j-1,i); j++;
                }
                else {
                  dmat(j,i) = smat(j,i);          j++;
                  dmat(j,i) = smat(j+2,i)*sqrt_3; j++;
                  dmat(j,i) = smat(j+2,i)*sqrt_3; j++;
                  dmat(j,i) = smat(j-2,i);        j++;
                  dmat(j,i) = smat(j+1,i)*sqrt_3; j++;
                  dmat(j,i) = smat(j-3,i);        j++;
                }
              }
              else if(c.l == 3) {
                //F functions
                if(is_spherical) {
                  dmat(j,i) = 1.0*smat(j+6,i);  j++;
                  dmat(j,i) = smat(j+3,i);      j++;
                  dmat(j,i) = smat(j,i);        j++;
                  dmat(j,i) = 1.0*smat(j-3,i);  j++;
                  dmat(j,i) = smat(j-3,i);      j++;
                  dmat(j,i) = smat(j-2,i);      j++;
                  dmat(j,i) = smat(j-1,i);      j++;
                }
                else {
                  dmat(j,i) = smat(j,i);                 j++;
                  dmat(j,i) = smat(j+3,i)*sqrt_5;        j++;
                  dmat(j,i) = smat(j+3,i)*sqrt_5;        j++;
                  dmat(j,i) = smat(j,i)  *sqrt_5;        j++;
                  dmat(j,i) = smat(j+5,i)*sqrt_5*sqrt_3; j++;
                  dmat(j,i) = smat(j+1,i)*sqrt_5;        j++;
                  dmat(j,i) = smat(j-5,i);               j++;
                  dmat(j,i) = smat(j+1,i)*sqrt_5;        j++; 
                  dmat(j,i) = smat(j-1,i)*sqrt_5;        j++;                  
                  dmat(j,i) = smat(j-7,i);               j++; 
                }
              }
              else if(c.l == 4) {
                //G functions
                if(is_spherical) {
                  dmat(j,i) = 1.0*smat(j+8,i);  j++;
                  dmat(j,i) = smat(j+5,i);      j++;
                  dmat(j,i) = smat(j+2,i);      j++;
                  dmat(j,i) = smat(j-1,i);      j++;
                  dmat(j,i) = 1.0*smat(j-4,i);  j++;
                  dmat(j,i) = 1.0*smat(j-4,i);  j++;
                  dmat(j,i) = smat(j-3,i);      j++;
                  dmat(j,i) = smat(j-2,i);      j++;
                  dmat(j,i) = 1.0*smat(j-1,i);  j++;
                }
                else {
                  dmat(j,i) = smat(j,i);                 j++;
                  dmat(j,i) = smat(j+2,i)*sqrt_7;        j++;
                  dmat(j,i) = smat(j+2,i)*sqrt_7;        j++;
                  dmat(j,i) = smat(j+6,i)*sqrt_753;      j++;
                  dmat(j,i) = smat(j+8,i)*sqrt_7*sqrt_5; j++;
                  dmat(j,i) = smat(j+5,i)*sqrt_753;      j++;
                  dmat(j,i) = smat(j-1,i)*sqrt_7;        j++;
                  dmat(j,i) = smat(j+6,i)*sqrt_7*sqrt_5; j++;
                  dmat(j,i) = smat(j+6,i)*sqrt_7*sqrt_5; j++;
                  dmat(j,i) = smat(j-2,i)*sqrt_7;        j++;
                  dmat(j,i) = smat(j-9,i);               j++;
                  dmat(j,i) = smat(j-5,i)*sqrt_7;        j++;
                  dmat(j,i) = smat(j-1,i)*sqrt_753;      j++;
                  dmat(j,i) = smat(j-5,i)*sqrt_7;        j++;
                  dmat(j,i) = smat(j-12,i);              j++;                           
                }
              }
              else if(c.l == 5) {
                //H functions
                if(is_spherical) {
                  dmat(j,i) = 1.0*smat(j+10,i); j++;
                  dmat(j,i) = -1.0*smat(j+7,i); j++;
                  dmat(j,i) = smat(j+4,i);      j++;
                  dmat(j,i) = smat(j+1,i);      j++;
                  dmat(j,i) = -1.0*smat(j-2,i); j++;
                  dmat(j,i) = 1.0*smat(j-5,i);  j++;
                  dmat(j,i) = smat(j-5,i);      j++;
                  dmat(j,i) = smat(j-4,i);      j++;
                  dmat(j,i) = 1.0*smat(j-3,i);  j++;   
                  dmat(j,i) = 1.0*smat(j-2,i);  j++;   
                  dmat(j,i) = -1.0*smat(j-1,i); j++;  
                }
                else NOT_IMPLEMENTED();       
              }                
            } //contr
          } //shells
        } //atom
      }
      smat = dmat;
    }
    if(reorder_cols) {
      dmat.setZero();
      size_t j = 0;
      for(size_t x = 0; x < atominfo.size(); x++) { //loop over atoms
        for(auto s: atominfo[x].shells) { //loop over each shell for given atom
          for(const auto& c: s.contr) { //loop over contractions. 
            // FIXME: assumes only 1 contraction for now
            if(c.l == 0) {
              //S functions
              col_copy(j,j); j++; 
            }
            else if(c.l == 1) {
              //P functions
              //libint set_pure to solid forces y,z,x ordering for l=1
              if(is_spherical) {
                col_copy(j,j+1); j++;
                col_copy(j,j+1); j++;
                col_copy(j,j-2); j++;
              }
              else { 
                col_copy(j,j); j++;
                col_copy(j,j); j++;
                col_copy(j,j); j++;
              }
            }
            else if(c.l == 2) {
              //D functions
              if(is_spherical) {
                col_copy(j,j+4); j++; 
                col_copy(j,j+1); j++; 
                col_copy(j,j-2); j++; 
                col_copy(j,j-2); j++; 
                col_copy(j,j-1); j++; 
              }
              else {
                col_copy(j,j);          j++;
                col_copy(j,j+2,sqrt_3); j++;
                col_copy(j,j+2,sqrt_3); j++;
                col_copy(j,j-2);        j++;
                col_copy(j,j+1,sqrt_3); j++;
                col_copy(j,j-3);        j++;
              }
            }
            else if(c.l == 3) {
              //F functions
              if(is_spherical) {
                col_copy(j,j+6); j++; //-1.0
                col_copy(j,j+3); j++; 
                col_copy(j,j);   j++; 
                col_copy(j,j-3); j++; //-1.0
                col_copy(j,j-3); j++; 
                col_copy(j,j-2); j++; 
                col_copy(j,j-1); j++; 
              }         
              else {
                col_copy(j,j);                 j++;
                col_copy(j,j+3,sqrt_5);        j++;
                col_copy(j,j+3,sqrt_5);        j++;
                col_copy(j,j,  sqrt_5);        j++;
                col_copy(j,j+5,sqrt_5*sqrt_3); j++;
                col_copy(j,j+1,sqrt_5);        j++;
                col_copy(j,j-5);               j++;
                col_copy(j,j+1,sqrt_5);        j++;
                col_copy(j,j-1,sqrt_5);        j++;
                col_copy(j,j-7);               j++; 
              }                       
            }
            else if(c.l == 4) {
              //G functions
              if(is_spherical) {
                col_copy(j,j+8); j++; //-1.0
                col_copy(j,j+5); j++; 
                col_copy(j,j+2); j++; 
                col_copy(j,j-1); j++; 
                col_copy(j,j-4); j++; //-1.0
                col_copy(j,j-4); j++; //-1.0
                col_copy(j,j-3); j++; 
                col_copy(j,j-2); j++; 
                col_copy(j,j-1); j++; //-1.0 
              }   
              else {
                col_copy(j,j);                 j++;
                col_copy(j,j+2,sqrt_7);        j++;
                col_copy(j,j+2,sqrt_7);        j++;
                col_copy(j,j+6,sqrt_753);      j++;
                col_copy(j,j+8,sqrt_7*sqrt_5); j++;
                col_copy(j,j+5,sqrt_753);      j++;
                col_copy(j,j-1,sqrt_7);        j++;
                col_copy(j,j+6,sqrt_7*sqrt_5); j++;
                col_copy(j,j+6,sqrt_7*sqrt_5); j++;
                col_copy(j,j-2,sqrt_7);        j++;
                col_copy(j,j-9);               j++;
                col_copy(j,j-5,sqrt_7);        j++;
                col_copy(j,j-1,sqrt_753);      j++;
                col_copy(j,j-5,sqrt_7);        j++;
                col_copy(j,j-12);              j++; 
              }
            }
            else if(c.l == 5) {
              //H functions
              if(is_spherical) {
                col_copy(j,j+10); j++; //-1.0
                col_copy(j,j+7);  j++; 
                col_copy(j,j+4);  j++; 
                col_copy(j,j+1);  j++; 
                col_copy(j,j-2);  j++; //-1.0
                col_copy(j,j-5);  j++; //-1.0
                col_copy(j,j-5);  j++; 
                col_copy(j,j-4);  j++; 
                col_copy(j,j-3);  j++; //-1.0  
                col_copy(j,j-2);  j++; //-1.0 
                col_copy(j,j-1);  j++; //-1.0
              }
              else NOT_IMPLEMENTED();               
            }
          } //contr
        } //shells
      } //atom
      smat = dmat;
    } //reorder cols

}

void renormalize_libint_shells(const SystemData& sys_data, libint2::BasisSet& shells) {
  using libint2::math::df_Kminus1;
  using std::pow;
  const auto sqrt_Pi_cubed = double{5.56832799683170784528481798212};
  #if 0 // TODO: Fix immutable basisset objects
  for (auto &s: shells) {
    const auto np = s.nprim();
    for(auto& c: s.contr) {
      EXPECTS(c.l <= 15); 
      for(auto p=0ul; p!=np; ++p) {
        EXPECTS(s.alpha[p] >= 0);
        if (s.alpha[p] != 0) {
          const auto two_alpha = 2 * s.alpha[p];
          const auto two_alpha_to_am32 = pow(two_alpha,c.l+1) * sqrt(two_alpha);
          const auto normalization_factor = sqrt(pow(2,c.l) * two_alpha_to_am32/(sqrt_Pi_cubed * df_Kminus1[2*c.l] ));

          c.coeff[p] *= normalization_factor;
        }
      }

      // need to force normalization to unity?
      if (s.do_enforce_unit_normalization()) {
        // compute the self-overlap of the , scale coefficients by its inverse square root
        double norm{0};
        for(auto p=0ul; p!=np; ++p) {
          for(decltype(p) q=0ul; q<=p; ++q) {
            auto gamma = s.alpha[p] + s.alpha[q];
            norm += (p==q ? 1 : 2) * df_Kminus1[2*c.l] * sqrt_Pi_cubed * c.coeff[p] * c.coeff[q] /
                    (pow(2,c.l) * pow(gamma,c.l+1) * sqrt(gamma));
          }
        }
        auto normalization_factor = 1 / sqrt(norm);
        for(auto p=0ul; p!=np; ++p) {
          c.coeff[p] *= normalization_factor;
        }
      }

    }

    // update max log coefficients
    s.max_ln_coeff.resize(np);
    for(auto p=0ul; p!=np; ++p) {
      double max_ln_c = - std::numeric_limits<double>::max();
      for(auto& c: s.contr) {
        max_ln_c = std::max(max_ln_c, std::log(std::abs(c.coeff[p])));
      }
      s.max_ln_coeff[p] = max_ln_c;
    }

  } //shells
  #endif
}

void read_geom_molden(const SystemData& sys_data, std::vector<libint2::Atom> &atoms) {
  std::string line;
  auto is = std::ifstream(sys_data.options_map.scf_options.moldenfile);

  while(line.find("[Atoms]") == std::string::npos)
    std::getline(is, line);

  //line at [Atoms]
  for (size_t ai=0;ai<atoms.size();ai++) {
    std::getline(is, line);
    std::istringstream iss(line);
    std::vector<std::string> geom{std::istream_iterator<std::string>{iss},
                                    std::istream_iterator<std::string>{}};
    atoms[ai].x = std::stod(geom[3]);
    atoms[ai].y = std::stod(geom[4]);
    atoms[ai].z = std::stod(geom[5]);
  }
}

void read_basis_molden(const SystemData& sys_data, libint2::BasisSet& shells) {

  //s_type = 0, p_type = 1, d_type = 2, 
  //f_type = 3, g_type = 4
  /*For spherical
  n = 2*type + 1
  s=1,p=3,d=5,f=7,g=9 
  For cartesian
  n = (type+1)*(type+2)/2
  s=1,p=3,d=6,f=10,g=15
  */

  std::string line;
  auto is = std::ifstream(sys_data.options_map.scf_options.moldenfile);

  while(line.find("GTO") == std::string::npos) {
    std::getline(is, line);
  } //end basis section

  bool basis_parse=true;
  int atom_i = 0, shell_i=0;
  while(basis_parse) {
    std::getline(is, line);
    std::istringstream iss(line);
    std::vector<std::string> expc{std::istream_iterator<std::string>{iss},
                                    std::istream_iterator<std::string>{}};
    if(expc.size()==0) continue;
    if(expc.size()==2) {
      //read shell nprim 0. TODO, should expc[1]==1 ?
      if(std::stoi(expc[0])==atom_i+1 && std::stoi(expc[1])==0) { atom_i++; } //shell_i=0;
    }

    // TODO: Fix immutable basisset objects
    // else if (expc[0]=="s" || expc[0]=="p" || expc[0]=="d" || expc[0]=="f" || expc[0]=="g") { 
    //   for(auto np=0;np<std::stoi(expc[1]);np++){
    //     std::getline(is, line);
    //     std::istringstream iss(line);
    //     std::vector<std::string> expc_val{std::istream_iterator<std::string>{iss},
    //                                   std::istream_iterator<std::string>{}};
    //     shells[shell_i].alpha[np] = std::stod(expc_val[0]);
    //     shells[shell_i].contr[0].coeff[np] = std::stod(expc_val[1]);
    //   } //nprims for shell_i
    //   shell_i++;
    // }
    else if (line.find("[5D]")   != std::string::npos) basis_parse=false;
    else if (line.find("[9G]")   != std::string::npos) basis_parse=false;
    else if (line.find("[MO]")   != std::string::npos) basis_parse=false;

  } //end basis parse
      
}

template<typename T>
void read_molden(const SystemData& sys_data, libint2::BasisSet& shells,
        Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>& C_alpha,
        Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>& C_beta) {

  auto scf_options = sys_data.options_map.scf_options;
  auto is = std::ifstream(scf_options.moldenfile);
  std::string line;
  size_t n_occ_alpha=0, n_occ_beta=0, n_vir_alpha=0, n_vir_beta=0;

  size_t N = C_alpha.rows();
  size_t Northo = C_alpha.cols();
  Matrix eigenvecs(N,Northo);
  eigenvecs.setZero();

  const bool is_spherical = (scf_options.gaussian_type == "spherical");
  const bool is_uhf = (sys_data.is_unrestricted);

  auto atoms = sys_data.options_map.options.atoms;
  const size_t natoms = atoms.size();

  auto a2s_map = shells.atom2shell(atoms);
  std::vector<AtomInfo> atominfo(natoms);

  for (size_t ai = 0; ai < natoms; ai++) {
    auto nshells = a2s_map[ai].size();
    auto first = a2s_map[ai][0];
    auto last = a2s_map[ai][nshells - 1];
    std::vector<libint2::Shell> atom_shells(nshells);
    int as_index = 0;
    for (auto si = first; si <= last; si++) {
      atom_shells[as_index] = shells[si];
      as_index++;
    }
    atominfo[ai].shells = atom_shells;
  }

  while(line.find("[MO]") == std::string::npos) {
    std::getline(is, line);
  } //end basis section
  
  bool mo_end = false;
  size_t i = 0;
  // size_t kb = 0;
  while(!mo_end) { 
    std::getline(is, line);
    if (line.find("Ene=") != std::string::npos) {
      /*evl_sorted[i] =*/ (std::stod(read_option(line)));
    }
    else if (line.find("Spin=") != std::string::npos){
      std::string spinstr = read_option(line);
      bool is_spin_alpha = spinstr.find("Alpha") != std::string::npos;
      bool is_spin_beta = spinstr.find("Beta") != std::string::npos;
      
      // if(is_spin_alpha) n_alpha++;
      // else if(is_spin_beta) n_beta++;
      std::getline(is, line);

      if (line.find("Occup=") != std::string::npos){
        int occup = stoi(read_option(line));
        if(is_spin_alpha) {
          if(occup==0) n_vir_alpha++;
          if(occup==1) n_occ_alpha++;
          if(occup==2) { n_occ_alpha++; n_occ_beta++; }
        }
        else if(is_spin_beta) {
           if(occup==1) n_occ_beta++;
           if(occup==0) n_vir_beta++; 
        }
        mo_end=true;
      }
    }

    if(mo_end){
      for(size_t j=0;j<N;j++) {
        std::getline(is, line);
        eigenvecs(j,i) = std::stod(read_option(line));
      }
      mo_end=false;
      i++;
    }

    if(i==Northo) mo_end=true;
  }

  const bool is_rhf = sys_data.is_restricted;
  reorder_molden_orbitals<T>(is_spherical, atominfo, eigenvecs, C_alpha, false);
  //TODO: WIP
  // if(is_uhf) reorder_molden_orbitals<T>(is_spherical, atominfo, eigenvecs, C_beta);

  if(is_rhf) { 
      n_occ_beta = n_occ_alpha;
      n_vir_beta = n_vir_alpha;
  }
  // else if(scf_options.scf_type == "rohf") { 
  //     n_vir_beta = N - n_occ_beta;
  // }

  cout << "finished reading molden: n_occ_alpha, n_vir_alpha, n_occ_beta, n_vir_beta = " 
         << n_occ_alpha << "," << n_vir_alpha << "," << n_occ_beta << "," << n_vir_beta << endl;

  EXPECTS(n_occ_alpha == sys_data.nelectrons_alpha);
  EXPECTS(n_occ_beta  == sys_data.nelectrons_beta);
  EXPECTS(n_vir_alpha == Northo - n_occ_alpha);
  EXPECTS(n_vir_beta  == Northo - n_occ_beta);

}
