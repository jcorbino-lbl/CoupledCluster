#pragma once

#include "common/json_data.hpp"
#include "scf/scf_main.hpp"
#include "tamm/eigen_utils.hpp"

#define CD_USE_PGAS_API

using namespace tamm;
using TAMM_GA_SIZE = int64_t;

std::tuple<TiledIndexSpace, TAMM_SIZE> setup_mo_red(SystemData sys_data, bool triples = false) {
  // TAMM_SIZE nao = sys_data.nbf;
  TAMM_SIZE n_occ_alpha = sys_data.n_occ_alpha;
  TAMM_SIZE n_vir_alpha = sys_data.n_vir_alpha;

  Tile tce_tile = sys_data.options_map.ccsd_options.tilesize;
  if(!triples) {
    if((tce_tile < static_cast<Tile>(sys_data.nbf / 10) || tce_tile < 50) &&
       !sys_data.options_map.ccsd_options.force_tilesize) {
      tce_tile = static_cast<Tile>(sys_data.nbf / 10);
      if(tce_tile < 50) tce_tile = 50; // 50 is the default tilesize for CCSD.
      if(ProcGroup::world_rank() == 0)
        std::cout << std::endl << "Resetting CCSD tilesize to: " << tce_tile << std::endl;
    }
  }
  else tce_tile = sys_data.options_map.ccsd_options.ccsdt_tilesize;

  const TAMM_SIZE total_orbitals = sys_data.nbf;

  // Construction of tiled index space MO
  IndexSpace MO_IS{
    range(0, total_orbitals),
    {{"occ", {range(0, n_occ_alpha)}}, {"virt", {range(n_occ_alpha, total_orbitals)}}}};

  std::vector<Tile> mo_tiles;

  tamm::Tile est_nt = static_cast<tamm::Tile>(std::ceil(1.0 * n_occ_alpha / tce_tile));
  for(tamm::Tile x = 0; x < est_nt; x++)
    mo_tiles.push_back(n_occ_alpha / est_nt + (x < (n_occ_alpha % est_nt)));

  est_nt = static_cast<tamm::Tile>(std::ceil(1.0 * n_vir_alpha / tce_tile));
  for(tamm::Tile x = 0; x < est_nt; x++)
    mo_tiles.push_back(n_vir_alpha / est_nt + (x < (n_vir_alpha % est_nt)));

  TiledIndexSpace MO{MO_IS, mo_tiles};

  return std::make_tuple(MO, total_orbitals);
}

std::tuple<TiledIndexSpace, TAMM_SIZE> setupMOIS(SystemData sys_data, bool triples = false,
                                                 int nactv = 0) {
  TAMM_SIZE n_occ_alpha = sys_data.n_occ_alpha;
  TAMM_SIZE n_occ_beta  = sys_data.n_occ_beta;

  Tile tce_tile      = sys_data.options_map.ccsd_options.tilesize;
  bool balance_tiles = sys_data.options_map.ccsd_options.balance_tiles;
  if(!triples) {
    if((tce_tile < static_cast<Tile>(sys_data.nbf / 10) || tce_tile < 50 || tce_tile > 100) &&
       !sys_data.options_map.ccsd_options.force_tilesize) {
      tce_tile = static_cast<Tile>(sys_data.nbf / 10);
      if(tce_tile < 50) tce_tile = 50;   // 50 is the default tilesize for CCSD.
      if(tce_tile > 100) tce_tile = 100; // 100 is the max tilesize for CCSD.
      if(ProcGroup::world_rank() == 0)
        std::cout << std::endl << "Resetting CCSD tilesize to: " << tce_tile << std::endl;
    }
  }
  else {
    balance_tiles = false;
    tce_tile      = sys_data.options_map.ccsd_options.ccsdt_tilesize;
  }

  TAMM_SIZE nmo         = sys_data.nmo;
  TAMM_SIZE n_vir_alpha = sys_data.n_vir_alpha;
  TAMM_SIZE n_vir_beta  = sys_data.n_vir_beta;
  TAMM_SIZE nocc        = sys_data.nocc;

  const TAMM_SIZE total_orbitals = nmo;

  // Construction of tiled index space MO
  TAMM_SIZE  virt_alpha_int = nactv;
  TAMM_SIZE  virt_beta_int  = virt_alpha_int;
  TAMM_SIZE  virt_alpha_ext = n_vir_alpha - nactv;
  TAMM_SIZE  virt_beta_ext  = total_orbitals - (nocc + nactv + n_vir_alpha);
  IndexSpace MO_IS{
    range(0, total_orbitals),
    {
      {"occ", {range(0, nocc)}},
      {"occ_alpha", {range(0, n_occ_alpha)}},
      {"occ_beta", {range(n_occ_alpha, nocc)}},
      {"virt", {range(nocc, total_orbitals)}},
      {"virt_alpha", {range(nocc, nocc + n_vir_alpha)}},
      {"virt_beta", {range(nocc + n_vir_alpha, total_orbitals)}},
      {"virt_alpha_int", {range(nocc, nocc + nactv)}},
      {"virt_beta_int", {range(nocc + n_vir_alpha, nocc + nactv + n_vir_alpha)}},
      {"virt_int",
       {range(nocc, nocc + nactv), range(nocc + n_vir_alpha, nocc + nactv + n_vir_alpha)}},
      {"virt_alpha_ext", {range(nocc + nactv, nocc + n_vir_alpha)}},
      {"virt_beta_ext", {range(nocc + nactv + n_vir_alpha, total_orbitals)}},
      {"virt_ext",
       {range(nocc + nactv, nocc + n_vir_alpha),
        range(nocc + nactv + n_vir_alpha, total_orbitals)}},
    },
    {{Spin{1}, {range(0, n_occ_alpha), range(nocc, nocc + n_vir_alpha)}},
     {Spin{2}, {range(n_occ_alpha, nocc), range(nocc + n_vir_alpha, total_orbitals)}}}};

  std::vector<Tile> mo_tiles;

  if(!balance_tiles) {
    tamm::Tile est_nt    = n_occ_alpha / tce_tile;
    tamm::Tile last_tile = n_occ_alpha % tce_tile;
    for(tamm::Tile x = 0; x < est_nt; x++) mo_tiles.push_back(tce_tile);
    if(last_tile > 0) mo_tiles.push_back(last_tile);
    est_nt    = n_occ_beta / tce_tile;
    last_tile = n_occ_beta % tce_tile;
    for(tamm::Tile x = 0; x < est_nt; x++) mo_tiles.push_back(tce_tile);
    if(last_tile > 0) mo_tiles.push_back(last_tile);
    // est_nt = n_vir_alpha/tce_tile;
    // last_tile = n_vir_alpha%tce_tile;
    // for (tamm::Tile x=0;x<est_nt;x++) mo_tiles.push_back(tce_tile);
    // if(last_tile>0) mo_tiles.push_back(last_tile);
    est_nt    = virt_alpha_int / tce_tile;
    last_tile = virt_alpha_int % tce_tile;
    for(tamm::Tile x = 0; x < est_nt; x++) mo_tiles.push_back(tce_tile);
    if(last_tile > 0) mo_tiles.push_back(last_tile);
    est_nt    = virt_alpha_ext / tce_tile;
    last_tile = virt_alpha_ext % tce_tile;
    for(tamm::Tile x = 0; x < est_nt; x++) mo_tiles.push_back(tce_tile);
    if(last_tile > 0) mo_tiles.push_back(last_tile);
    // est_nt = n_vir_beta/tce_tile;
    // last_tile = n_vir_beta%tce_tile;
    // for (tamm::Tile x=0;x<est_nt;x++) mo_tiles.push_back(tce_tile);
    // if(last_tile>0) mo_tiles.push_back(last_tile);
    est_nt    = virt_beta_int / tce_tile;
    last_tile = virt_beta_int % tce_tile;
    for(tamm::Tile x = 0; x < est_nt; x++) mo_tiles.push_back(tce_tile);
    if(last_tile > 0) mo_tiles.push_back(last_tile);
    est_nt    = virt_beta_ext / tce_tile;
    last_tile = virt_beta_ext % tce_tile;
    for(tamm::Tile x = 0; x < est_nt; x++) mo_tiles.push_back(tce_tile);
    if(last_tile > 0) mo_tiles.push_back(last_tile);
  }
  else {
    tamm::Tile est_nt = static_cast<tamm::Tile>(std::ceil(1.0 * n_occ_alpha / tce_tile));
    for(tamm::Tile x = 0; x < est_nt; x++)
      mo_tiles.push_back(n_occ_alpha / est_nt + (x < (n_occ_alpha % est_nt)));

    est_nt = static_cast<tamm::Tile>(std::ceil(1.0 * n_occ_beta / tce_tile));
    for(tamm::Tile x = 0; x < est_nt; x++)
      mo_tiles.push_back(n_occ_beta / est_nt + (x < (n_occ_beta % est_nt)));

    // est_nt = static_cast<tamm::Tile>(std::ceil(1.0 * n_vir_alpha / tce_tile));
    // for (tamm::Tile x=0;x<est_nt;x++) mo_tiles.push_back(n_vir_alpha / est_nt + (x<(n_vir_alpha %
    // est_nt)));

    est_nt = static_cast<tamm::Tile>(std::ceil(1.0 * virt_alpha_int / tce_tile));
    for(tamm::Tile x = 0; x < est_nt; x++)
      mo_tiles.push_back(virt_alpha_int / est_nt + (x < (virt_alpha_int % est_nt)));

    est_nt = static_cast<tamm::Tile>(std::ceil(1.0 * virt_alpha_ext / tce_tile));
    for(tamm::Tile x = 0; x < est_nt; x++)
      mo_tiles.push_back(virt_alpha_ext / est_nt + (x < (virt_alpha_ext % est_nt)));

    // est_nt = static_cast<tamm::Tile>(std::ceil(1.0 * n_vir_beta / tce_tile));
    // for (tamm::Tile x=0;x<est_nt;x++) mo_tiles.push_back(n_vir_beta / est_nt + (x<(n_vir_beta %
    // est_nt)));

    est_nt = static_cast<tamm::Tile>(std::ceil(1.0 * virt_beta_int / tce_tile));
    for(tamm::Tile x = 0; x < est_nt; x++)
      mo_tiles.push_back(virt_beta_int / est_nt + (x < (virt_beta_int % est_nt)));

    est_nt = static_cast<tamm::Tile>(std::ceil(1.0 * virt_beta_ext / tce_tile));
    for(tamm::Tile x = 0; x < est_nt; x++)
      mo_tiles.push_back(virt_beta_ext / est_nt + (x < (virt_beta_ext % est_nt)));
  }

  TiledIndexSpace MO{MO_IS, mo_tiles}; //{ova,ova,ovb,ovb}};

  return std::make_tuple(MO, total_orbitals);
}

void update_sysdata(SystemData& sys_data, TiledIndexSpace& MO, bool is_mso = true) {
  const bool do_freeze      = sys_data.n_frozen_core > 0 || sys_data.n_frozen_virtual > 0;
  TAMM_SIZE  total_orbitals = sys_data.nmo;
  if(do_freeze) {
    sys_data.nbf -= (sys_data.n_frozen_core + sys_data.n_frozen_virtual);
    sys_data.n_occ_alpha -= sys_data.n_frozen_core;
    sys_data.n_vir_alpha -= sys_data.n_frozen_virtual;
    if(is_mso) {
      sys_data.n_occ_beta -= sys_data.n_frozen_core;
      sys_data.n_vir_beta -= sys_data.n_frozen_virtual;
    }
    sys_data.update();
    if(!is_mso) std::tie(MO, total_orbitals) = setup_mo_red(sys_data);
    else std::tie(MO, total_orbitals) = setupMOIS(sys_data);
  }
}

// Reshape F/lcao after freezing
Matrix reshape_mo_matrix(SystemData sys_data, Matrix& emat, bool is_lcao = false) {
  const int noa   = sys_data.n_occ_alpha;
  const int nob   = sys_data.n_occ_beta;
  const int nva   = sys_data.n_vir_alpha;
  const int nvb   = sys_data.n_vir_beta;
  const int nocc  = sys_data.nocc;
  const int N_eff = sys_data.nmo;
  const int nbf   = sys_data.nbf_orig;

  const int n_frozen_core    = sys_data.n_frozen_core;
  const int n_frozen_virtual = sys_data.n_frozen_virtual;

  Matrix cvec;
  if(!is_lcao) cvec.resize(N_eff, N_eff); // MOxMO
  else cvec.resize(nbf, N_eff);           // AOxMO

  const int block2_off     = 2 * n_frozen_core + noa;
  const int block3_off     = 2 * n_frozen_core + nocc;
  const int last_block_off = block3_off + n_frozen_virtual + nva;

  if(is_lcao) {
    cvec.block(0, 0, nbf, noa)          = emat.block(0, n_frozen_core, nbf, noa);
    cvec.block(0, noa, nbf, nob)        = emat.block(0, block2_off, nbf, nob);
    cvec.block(0, nocc, nbf, nva)       = emat.block(0, block3_off, nbf, nva);
    cvec.block(0, nocc + nva, nbf, nvb) = emat.block(0, last_block_off, nbf, nvb);
  }
  else {
    cvec.block(0, 0, noa, noa)          = emat.block(n_frozen_core, n_frozen_core, noa, noa);
    cvec.block(0, noa, noa, nob)        = emat.block(n_frozen_core, block2_off, noa, nob);
    cvec.block(0, nocc, noa, nva)       = emat.block(n_frozen_core, block3_off, noa, nva);
    cvec.block(0, nocc + nva, noa, nvb) = emat.block(n_frozen_core, last_block_off, noa, nvb);

    cvec.block(noa, 0, nob, noa)          = emat.block(block2_off, n_frozen_core, nob, noa);
    cvec.block(noa, noa, nob, nob)        = emat.block(block2_off, block2_off, nob, nob);
    cvec.block(noa, nocc, nob, nva)       = emat.block(block2_off, block3_off, nob, nva);
    cvec.block(noa, nocc + nva, nob, nvb) = emat.block(block2_off, last_block_off, nob, nvb);

    cvec.block(nocc, 0, nva, noa)          = emat.block(block3_off, n_frozen_core, nva, noa);
    cvec.block(nocc, noa, nva, nob)        = emat.block(block3_off, block2_off, nva, nob);
    cvec.block(nocc, nocc, nva, nva)       = emat.block(block3_off, block3_off, nva, nva);
    cvec.block(nocc, nocc + nva, nva, nvb) = emat.block(block3_off, last_block_off, nva, nvb);

    cvec.block(nocc + nva, 0, nvb, noa)    = emat.block(last_block_off, n_frozen_core, nvb, noa);
    cvec.block(nocc + nva, noa, nvb, nob)  = emat.block(last_block_off, block2_off, nvb, nob);
    cvec.block(nocc + nva, nocc, nvb, nva) = emat.block(last_block_off, block3_off, nvb, nva);
    cvec.block(nocc + nva, nocc + nva, nvb, nvb) =
      emat.block(last_block_off, last_block_off, nvb, nvb);
  }

  emat.resize(0, 0);
  return cvec;
}

template<typename TensorType>
Tensor<TensorType> cd_svd(SystemData& sys_data, ExecutionContext& ec, TiledIndexSpace& tMO,
                          TiledIndexSpace& tAO, TAMM_SIZE& chol_count, const TAMM_GA_SIZE max_cvecs,
                          libint2::BasisSet& shells, Tensor<TensorType>& lcao, bool is_mso = true) {
  using libint2::Atom;
  using libint2::Engine;
  using libint2::Operator;
  using libint2::Shell;

  const auto       ccsd_options = sys_data.options_map.ccsd_options;
  const bool       readt        = ccsd_options.readt;
  const bool       writet       = ccsd_options.writet;
  const double     diagtol      = sys_data.options_map.cd_options.diagtol;
  const int        write_vcount = sys_data.options_map.cd_options.write_vcount;
  const tamm::Tile itile_size   = ccsd_options.itilesize;
  // const TAMM_GA_SIZE northo      = sys_data.nbf;
  const TAMM_GA_SIZE nao = sys_data.nbf_orig;

  SCFVars scf_vars; // init vars
  std::tie(scf_vars.shell_tile_map, scf_vars.AO_tiles, scf_vars.AO_opttiles) =
    compute_AO_tiles(ec, sys_data, shells);
  compute_shellpair_list(ec, shells, scf_vars);
  auto [obs_shellpair_list, obs_shellpair_data] = compute_shellpairs(shells);

  auto shell2bf = map_shell_to_basis_function(shells);
  auto bf2shell = map_basis_function_to_shell(shells);

  std::vector<size_t>     shell_tile_map = scf_vars.shell_tile_map;
  std::vector<tamm::Tile> AO_tiles       = scf_vars.AO_tiles;

  // TiledIndexSpace tAOt{tAO.index_space(), AO_tiles};
  ExecutionContext ec_dense{ec.pg(), DistributionKind::dense, MemoryManagerKind::ga};
  auto             rank = ec_dense.pg().rank().value();

  TAMM_GA_SIZE    N = tMO("all").max_num_indices();
  IndexSpace      CI{range(0, max_cvecs)};
  TiledIndexSpace tCI{CI, static_cast<Tile>(max_cvecs)};

  Matrix lcao_eig(nao, N);
  lcao_eig.setZero();
  tamm_to_eigen_tensor(lcao, lcao_eig);

  if(rank == 0) {
    cout << endl << "    Begin Cholesky Decomposition" << endl;
    cout << std::string(45, '-') << endl;
  }

  const auto nbf   = nao;
  int64_t    count = 0; // Initialize cholesky vector count

  const auto out_fp        = sys_data.output_file_prefix + "." + ccsd_options.basis;
  const auto files_dir     = out_fp + "_files/" + sys_data.options_map.scf_options.scf_type;
  const auto files_prefix  = /*out_fp;*/ files_dir + "/" + out_fp;
  const auto chol_ao_file  = files_prefix + ".chol_ao";
  const auto diag_ao_file  = files_prefix + ".diag_ao";
  const auto cv_count_file = files_prefix + ".cholcount";

  std::vector<int64_t> lo_x(4, -1); // The lower limits of blocks
  std::vector<int64_t> hi_x(4, -2); // The upper limits of blocks
  std::vector<int64_t> ld_x(4);     // The leading dims of blocks

  Tensor<TensorType> g_d_tamm{tAO, tAO};
  Tensor<TensorType> g_r_tamm{tAO, tAO};
  Tensor<TensorType> g_chol_tamm{tAO, tAO, tCI};

  double cd_mem_req       = sum_tensor_sizes(g_d_tamm, g_r_tamm, g_chol_tamm);
  auto   check_cd_mem_req = [&](const std::string mstep) {
    if(ec.print())
      std::cout << "- CPU memory required for " << mstep << ": " << std::fixed
                << std::setprecision(2) << cd_mem_req << " GiB" << std::endl;
    check_memory_requirements(ec, cd_mem_req);
  };
  check_cd_mem_req("computing cholesky vectors");

  g_d_tamm.set_dense();
  g_r_tamm.set_dense();
  g_chol_tamm.set_dense();

  Tensor<TensorType>::allocate(&ec_dense, g_d_tamm, g_r_tamm, g_chol_tamm);

#ifndef USE_UPCXX
  NGA_Zero(g_d_tamm.ga_handle());
  NGA_Zero(g_r_tamm.ga_handle());
  NGA_Zero(g_chol_tamm.ga_handle());

  auto write_chol_vectors = [&]() {
    write_to_disk(g_d_tamm, diag_ao_file);
    write_to_disk(g_chol_tamm, chol_ao_file);
    if(rank == 0) {
      std::ofstream out(cv_count_file, std::ios::out);
      if(!out) cerr << "Error opening file " << cv_count_file << endl;
      out << count << std::endl;
      out.close();
      if(rank == 0)
        cout << endl << "- Number of cholesky vectors written to disk = " << count << endl;
    }
  };

  const int g_d    = g_d_tamm.ga_handle();
  const int g_r    = g_r_tamm.ga_handle();
  const int g_chol = g_chol_tamm.ga_handle();

#ifdef CD_USE_PGAS_API
  std::vector<int64_t> lo_b(g_chol_tamm.num_modes(), -1); // The lower limits of blocks of B
  std::vector<int64_t> hi_b(g_chol_tamm.num_modes(), -2); // The upper limits of blocks of B
  std::vector<int64_t> ld_b(g_chol_tamm.num_modes());     // The leading dims of blocks of B

  std::vector<int64_t> lo_r(g_r_tamm.num_modes(), -1); // The lower limits of blocks of R
  std::vector<int64_t> hi_r(g_r_tamm.num_modes(), -2); // The upper limits of blocks of R
  std::vector<int64_t> ld_r(g_r_tamm.num_modes());     // The leading dims of blocks of R

  std::vector<int64_t> lo_d(g_d_tamm.num_modes(), -1); // The lower limits of blocks of D
  std::vector<int64_t> hi_d(g_d_tamm.num_modes(), -2); // The upper limits of blocks of D
  std::vector<int64_t> ld_d(g_d_tamm.num_modes());     // The leading dims of blocks of D

  // Distribution Check
  NGA_Distribution64(g_chol, rank, lo_b.data(), hi_b.data());
  NGA_Distribution64(g_d, rank, lo_d.data(), hi_d.data());
  NGA_Distribution64(g_r, rank, lo_r.data(), hi_r.data());

  bool has_gc_data = (lo_b[0] >= 0 && hi_b[0] >= 0);
  bool has_gd_data = (lo_d[0] >= 0 && hi_d[0] >= 0);
  bool has_gr_data = (lo_r[0] >= 0 && hi_r[0] >= 0);
#endif
#endif

  auto cd_t1 = std::chrono::high_resolution_clock::now();

  /*
    g_d_tamm stores the diagonal integrals, i.e. (uv|uv)'s
    ScrCol temporarily stores all (uv|rs)'s with fixed r and s
  */
  Engine      engine(Operator::coulomb, max_nprim(shells), max_l(shells), 0);
  const auto& buf = engine.results();
  bool cd_restart = (readt || writet) && fs::exists(diag_ao_file) && fs::exists(chol_ao_file) &&
                    fs::exists(cv_count_file);

  auto compute_diagonals = [&](const IndexVector& blockid) {
    auto bi0 = blockid[0];
    auto bi1 = blockid[1];

    const TAMM_SIZE         size       = g_d_tamm.block_size(blockid);
    auto                    block_dims = g_d_tamm.block_dims(blockid);
    std::vector<TensorType> dbuf(size);

    auto bd1 = block_dims[1];

    auto s1range_start = 0l;
    auto s1range_end   = shell_tile_map[bi0];
    if(bi0 > 0) s1range_start = shell_tile_map[bi0 - 1] + 1;

    for(auto s1 = s1range_start; s1 <= s1range_end; ++s1) {
      auto n1 = shells[s1].size();

      auto s2range_start = 0l;
      auto s2range_end   = shell_tile_map[bi1];
      if(bi1 > 0) s2range_start = shell_tile_map[bi1 - 1] + 1;

      for(size_t s2 = s2range_start; s2 <= s2range_end; ++s2) {
        if(s2 > s1) {
          auto s2spl = scf_vars.obs_shellpair_list[s2];
          if(std::find(s2spl.begin(), s2spl.end(), s1) == s2spl.end()) continue;
        }
        else {
          auto s2spl = scf_vars.obs_shellpair_list[s1];
          if(std::find(s2spl.begin(), s2spl.end(), s2) == s2spl.end()) continue;
        }

        auto n2 = shells[s2].size();

        // Compute shell pair; return is the pointer to the buffer
        engine.compute(shells[s1], shells[s2], shells[s1], shells[s2]);
        const auto* buf_1212 = buf[0];
        if(buf_1212 == nullptr) continue;

        auto curshelloffset_i = 0U;
        auto curshelloffset_j = 0U;
        for(auto x = s1range_start; x < s1; x++) curshelloffset_i += AO_tiles[x];
        for(auto x = s2range_start; x < s2; x++) curshelloffset_j += AO_tiles[x];

        auto dimi = curshelloffset_i + AO_tiles[s1];
        auto dimj = curshelloffset_j + AO_tiles[s2];

        for(size_t i = curshelloffset_i; i < dimi; i++) {
          for(size_t j = curshelloffset_j; j < dimj; j++) {
            auto f1           = i - curshelloffset_i;
            auto f2           = j - curshelloffset_j;
            auto f1212        = f1 * n2 * n1 * n2 + f2 * n1 * n2 + f1 * n2 + f2;
            dbuf[i * bd1 + j] = buf_1212[f1212];
          }
        }
      }
    }
    g_d_tamm.put(blockid, dbuf);
  };

  if(!cd_restart) block_for(ec_dense, g_d_tamm(), compute_diagonals);

  auto cd_t2   = std::chrono::high_resolution_clock::now();
  auto cd_time = std::chrono::duration_cast<std::chrono::duration<double>>((cd_t2 - cd_t1)).count();
  if(rank == 0 && !cd_restart) {
    std::cout << endl
              << "- Time for computing the diagonal: " << std::fixed << std::setprecision(2)
              << cd_time << " secs" << endl;
  }

#ifndef USE_UPCXX
  if(cd_restart) {
    cd_t1 = std::chrono::high_resolution_clock::now();

    read_from_disk(g_d_tamm, diag_ao_file);
    read_from_disk(g_chol_tamm, chol_ao_file);

    std::ifstream in(cv_count_file, std::ios::in);
    int           rstatus = 0;
    if(in.is_open()) rstatus = 1;
    if(rstatus == 1) in >> count;
    else tamm_terminate("Error reading " + cv_count_file);

    if(rank == 0)
      cout << endl << "- [CD restart] Number of cholesky vectors read = " << count << endl;

    cd_t2   = std::chrono::high_resolution_clock::now();
    cd_time = std::chrono::duration_cast<std::chrono::duration<double>>((cd_t2 - cd_t1)).count();
    if(rank == 0) {
      std::cout << "- [CD restart] Time for reading the diagonal and cholesky vectors: "
                << std::fixed << std::setprecision(2) << cd_time << " secs" << endl;
    }
  }
#endif

  auto cd_t3 = std::chrono::high_resolution_clock::now();

  auto [val_d0, blkid, eoff]  = tamm::max_element(g_d_tamm);
  auto                 blkoff = g_d_tamm.block_offsets(blkid);
  std::vector<int64_t> indx_d0(g_d_tamm.num_modes());
  indx_d0[0] = (int64_t) blkoff[0] + (int64_t) eoff[0];
  indx_d0[1] = (int64_t) blkoff[1] + (int64_t) eoff[1];

  while(val_d0 > diagtol && count < max_cvecs) {
    auto bfu   = indx_d0[0];
    auto bfv   = indx_d0[1];
    auto s1    = bf2shell[bfu];
    auto n1    = shells[s1].size();
    auto s2    = bf2shell[bfv];
    auto n2    = shells[s2].size();
    auto n12   = n1 * n2;
    auto f1    = bfu - shell2bf[s1];
    auto f2    = bfv - shell2bf[s2];
    auto ind12 = f1 * n2 + f2;

#ifndef USE_UPCXX
    NGA_Zero(g_r_tamm.ga_handle());
#endif

#ifndef CD_USE_PGAS_API
    auto compute_eri = [&](const IndexVector& blockid) {
      auto bi0 = blockid[0];
      auto bi1 = blockid[1];

      const TAMM_SIZE         size       = g_r_tamm.block_size(blockid);
      auto                    block_dims = g_r_tamm.block_dims(blockid);
      std::vector<TensorType> dbuf(size);

      auto bd1 = block_dims[1];

      auto s3range_start = 0l;
      auto s3range_end   = shell_tile_map[bi0];
      if(bi0 > 0) s3range_start = shell_tile_map[bi0 - 1] + 1;

      for(Index s3 = s3range_start; s3 <= s3range_end; ++s3) {
        auto n3 = shells[s3].size();

        auto s4range_start = 0l;
        auto s4range_end   = shell_tile_map[bi1];
        if(bi1 > 0) s4range_start = shell_tile_map[bi1 - 1] + 1;

        for(Index s4 = s4range_start; s4 <= s4range_end; ++s4) {
          if(s4 > s3) {
            auto s2spl = obs_shellpair_list[s4];
            if(std::find(s2spl.begin(), s2spl.end(), s3) == s2spl.end()) continue;
          }
          else {
            auto s2spl = obs_shellpair_list[s3];
            if(std::find(s2spl.begin(), s2spl.end(), s4) == s2spl.end()) continue;
          }

          auto n4 = shells[s4].size();

          // Compute shell pair; return is the pointer to the buffer
          engine.compute(shells[s3], shells[s4], shells[s1], shells[s2]);
          const auto* buf_3412 = buf[0];
          if(buf_3412 == nullptr) continue; // If all integrals screened out, skip to next quartet

          auto curshelloffset_i = 0U;
          auto curshelloffset_j = 0U;
          for(auto x = s3range_start; x < s3; x++) curshelloffset_i += AO_tiles[x];
          for(auto x = s4range_start; x < s4; x++) curshelloffset_j += AO_tiles[x];

          auto dimi = curshelloffset_i + AO_tiles[s3];
          auto dimj = curshelloffset_j + AO_tiles[s4];

          for(size_t i = curshelloffset_i; i < dimi; i++) {
            for(size_t j = curshelloffset_j; j < dimj; j++) {
              auto f3           = i - curshelloffset_i;
              auto f4           = j - curshelloffset_j;
              auto f3412        = f3 * n4 * n12 + f4 * n12 + ind12;
              auto x            = buf_3412[f3412];
              dbuf[i * bd1 + j] = x;
            }
          }
        }
      }
      g_r_tamm.put(blockid, dbuf);
    };

    block_for(ec_dense, g_r_tamm(), compute_eri);

#else
    for(size_t s3 = 0; s3 != shells.size(); ++s3) {
      auto bf3_first = shell2bf[s3]; // First basis function in this shell
      auto n3        = shells[s3].size();

      for(size_t s4 = 0; s4 != shells.size(); ++s4) {
        auto bf4_first = shell2bf[s4];
        auto n4        = shells[s4].size();

#ifdef USE_UPCXX
        if(g_r_tamm.is_local_element(0, 0, bf3_first, bf4_first)) {
#else
        if(lo_r[0] <= bf3_first && bf3_first <= hi_r[0] && lo_r[1] <= bf4_first &&
           bf4_first <= hi_r[1]) {
#endif
          engine.compute(shells[s3], shells[s4], shells[s1], shells[s2]);
          const auto* buf_3412 = buf[0];
          if(buf_3412 == nullptr) continue; // If all integrals screened out, skip to next quartet

          std::vector<TensorType> k_eri(n3 * n4);
          for(decltype(n3) f3 = 0; f3 != n3; ++f3)
            for(decltype(n4) f4 = 0; f4 != n4; ++f4) {
              auto f3412          = f3 * n4 * n12 + f4 * n12 + ind12;
              k_eri[f3 * n4 + f4] = buf_3412[f3412];
            }

          int64_t ibflo[4] = {0, 0, cd_ncast<size_t>(bf3_first), cd_ncast<size_t>(bf4_first)};
          int64_t ibfhi[4] = {0, 0, cd_ncast<size_t>(bf3_first + n3 - 1),
                              cd_ncast<size_t>(bf4_first + n4 - 1)};

#ifdef USE_UPCXX
          g_r_tamm.put_raw(ibflo, ibfhi, k_eri.data());
#else
          int64_t ld[1] = {cd_ncast<size_t>(n4)};
          NGA_Put64(g_r, &ibflo[2], &ibfhi[2], k_eri.data(), ld);
#endif
        }
      }
    }

    ec_dense.pg().barrier();
#endif

#ifndef USE_UPCXX
    lo_x[0] = indx_d0[0];
    lo_x[1] = indx_d0[1];
    lo_x[2] = 0;
    lo_x[3] = 0;
    hi_x[0] = indx_d0[0];
    hi_x[1] = indx_d0[1];
    hi_x[2] = count;
    hi_x[3] = 0;
    ld_x[0] = 1;
    ld_x[1] = hi_x[2] + 1;
#else
    lo_x[0] = 0;
    lo_x[1] = indx_d0[0];
    lo_x[2] = indx_d0[1];
    lo_x[3] = 0;
    hi_x[0] = 0;
    hi_x[1] = indx_d0[0];
    hi_x[2] = indx_d0[1];
    hi_x[3] = count;
#endif

    std::vector<TensorType> k_row(max_cvecs);

#ifndef CD_USE_PGAS_API
    auto update_diagonals = [&](const IndexVector& blockid) {
      auto bi0 = blockid[0];
      auto bi1 = blockid[1];

      IndexVector             rdblockid  = {bi0, bi1};
      const TAMM_SIZE         size       = g_chol_tamm.block_size(blockid);
      auto                    block_dims = g_chol_tamm.block_dims(blockid);
      const TAMM_SIZE         rdsize     = g_r_tamm.block_size(rdblockid);
      std::vector<TensorType> cbuf(size);
      std::vector<TensorType> rbuf(rdsize);
      std::vector<TensorType> dbuf(rdsize);

      g_r_tamm.get(rdblockid, rbuf);
      g_d_tamm.get(rdblockid, dbuf);
      g_chol_tamm.get(blockid, cbuf);

      auto bd0 = block_dims[1];
      auto bd1 = block_dims[2];

      auto s3range_start = 0l;
      auto s3range_end   = shell_tile_map[bi0];
      if(bi0 > 0) s3range_start = shell_tile_map[bi0 - 1] + 1;

      for(Index s3 = s3range_start; s3 <= s3range_end; ++s3) {
        auto n3 = shells[s3].size();

        auto s4range_start = 0l;
        auto s4range_end   = shell_tile_map[bi1];
        if(bi1 > 0) s4range_start = shell_tile_map[bi1 - 1] + 1;

        for(Index s4 = s4range_start; s4 <= s4range_end; ++s4) {
          if(s4 > s3) {
            auto s2spl = obs_shellpair_list[s4];
            if(std::find(s2spl.begin(), s2spl.end(), s3) == s2spl.end()) continue;
          }
          else {
            auto s2spl = obs_shellpair_list[s3];
            if(std::find(s2spl.begin(), s2spl.end(), s4) == s2spl.end()) continue;
          }

          auto n4 = shells[s4].size();

          auto curshelloffset_i = 0U;
          auto curshelloffset_j = 0U;
          for(auto x = s3range_start; x < s3; x++) curshelloffset_i += AO_tiles[x];
          for(auto x = s4range_start; x < s4; x++) curshelloffset_j += AO_tiles[x];

          auto dimi = curshelloffset_i + AO_tiles[s3];
          auto dimj = curshelloffset_j + AO_tiles[s4];

          for(size_t i = curshelloffset_i; i < dimi; i++) {
            for(size_t j = curshelloffset_j; j < dimj; j++) {
              for(decltype(count) icount = 0; icount < count; icount++) {
                rbuf[i * bd0 + j] -= cbuf[icount + j * bd1 + i * bd0 * bd1] * k_row[icount];
              }
              auto vtmp                             = rbuf[i * bd0 + j] / sqrt(val_d0);
              cbuf[count + j * bd1 + i * bd1 * bd0] = vtmp;
              dbuf[i * bd0 + j] -= vtmp * vtmp;
            }
          }
        }
      }

      g_r_tamm.put(rdblockid, rbuf);
      g_d_tamm.put(rdblockid, dbuf);
      g_chol_tamm.put(blockid, cbuf);
    };

    block_for(ec_dense, g_chol_tamm(), update_diagonals);

    count++;
#else
#ifdef USE_UPCXX

    g_chol_tamm.get_raw_contig(lo_x.data(), hi_x.data(), k_row.data());

    auto left  = g_r_tamm.access_local_buf();
    auto right = g_chol_tamm.access_local_buf();
    auto n     = g_r_tamm.local_buf_size();
    for(size_t icount = 0; icount < count; icount++)
      for(size_t i = 0, k = icount; i < n; i++, k += max_cvecs)
        *(left + i) -= *(right + k) * k_row[icount];

    for(size_t i = 0, k = count; i < n; i++, k += max_cvecs) {
      auto tmp     = *(left + i) / sqrt(val_d0);
      *(right + k) = tmp;
    }

    left = g_d_tamm.access_local_buf();
    n    = g_d_tamm.local_buf_size();
    for(size_t i = 0, k = count; i < n; i++, k += max_cvecs) {
      auto tmp = *(right + k);
      *(left + i) -= tmp * tmp;
    }

    count++;
#else
    TensorType *indx_b, *indx_d, *indx_r;

    {
      NGA_Get64(g_chol, lo_x.data(), hi_x.data(), k_row.data(), ld_x.data());

      if(has_gr_data) NGA_Access64(g_r, lo_r.data(), hi_r.data(), &indx_r, ld_r.data());
      if(has_gc_data) NGA_Access64(g_chol, lo_b.data(), hi_b.data(), &indx_b, ld_b.data());

      if(has_gr_data) {
        for(decltype(count) icount = 0; icount < count; icount++) {
          for(int64_t i = 0; i <= hi_r[0] - lo_r[0]; i++) {
            for(int64_t j = 0; j <= hi_r[1] - lo_r[1]; j++) {
              indx_r[i * ld_r[0] + j] -=
                indx_b[icount + j * ld_b[1] + i * ld_b[1] * ld_b[0]] * k_row[icount];
            }
          }
        }
      }

      if(has_gc_data) NGA_Release64(g_chol, lo_b.data(), hi_b.data());
      if(has_gr_data) NGA_Release_update64(g_r, lo_r.data(), hi_r.data());

      if(has_gr_data) NGA_Access64(g_r, lo_r.data(), hi_r.data(), &indx_r, ld_r.data());
      if(has_gc_data) NGA_Access64(g_chol, lo_b.data(), hi_b.data(), &indx_b, ld_b.data());

      if(has_gc_data) {
        for(auto i = 0; i <= hi_r[0] - lo_r[0]; i++) {
          for(auto j = 0; j <= hi_r[1] - lo_r[1]; j++) {
            auto tmp = indx_r[i * ld_r[0] + j] / sqrt(val_d0);
            indx_b[count + j * ld_b[1] + i * ld_b[1] * ld_b[0]] = tmp;
          }
        }
      }

      if(has_gc_data) NGA_Release_update64(g_chol, lo_b.data(), hi_b.data());
      if(has_gr_data) NGA_Release64(g_r, lo_r.data(), hi_r.data());
    }

    if(has_gd_data) NGA_Access64(g_d, lo_d.data(), hi_d.data(), &indx_d, ld_d.data());
    if(has_gc_data) NGA_Access64(g_chol, lo_b.data(), hi_b.data(), &indx_b, ld_b.data());

    if(has_gd_data) {
      for(auto i = 0; i <= hi_d[0] - lo_d[0]; i++) {
        for(auto j = 0; j <= hi_d[1] - lo_d[1]; j++) {
          auto tmp = indx_b[count + j * ld_b[1] + i * ld_b[1] * ld_b[0]];
          indx_d[i * ld_d[0] + j] -= tmp * tmp;
        }
      }
    }

    if(has_gc_data) NGA_Release64(g_chol, lo_b.data(), hi_b.data());
    if(has_gd_data) NGA_Release_update64(g_d, lo_d.data(), hi_d.data());

    count++;
#endif
#endif

    std::tie(val_d0, blkid, eoff) = tamm::max_element(g_d_tamm);
    blkoff                        = g_d_tamm.block_offsets(blkid);
    indx_d0[0]                    = (int64_t) blkoff[0] + (int64_t) eoff[0];
    indx_d0[1]                    = (int64_t) blkoff[1] + (int64_t) eoff[1];

#ifndef USE_UPCXX
    if(writet && count % write_vcount == 0 && nbf > 1000) write_chol_vectors();
#endif
  }

  if(rank == 0) std::cout << endl << "- Total number of cholesky vectors = " << count << std::endl;

  auto cd_t4 = std::chrono::high_resolution_clock::now();
  cd_time    = std::chrono::duration_cast<std::chrono::duration<double>>((cd_t4 - cd_t3)).count();
  if(rank == 0) {
    std::cout << endl
              << "- Time to compute cholesky vectors: " << std::fixed << std::setprecision(2)
              << cd_time << " secs" << endl
              << endl;
  }

#ifndef USE_UPCXX
  if(writet && nbf > 1000) write_chol_vectors();
#endif

  Tensor<TensorType>::deallocate(g_d_tamm, g_r_tamm);

  update_sysdata(sys_data, tMO, is_mso);

  const bool do_freeze = (sys_data.n_frozen_core > 0 || sys_data.n_frozen_virtual > 0);

  IndexSpace         CIp{range(0, count)};
  TiledIndexSpace    tCIp{CIp, static_cast<tamm::Tile>(itile_size)};
  Tensor<TensorType> g_chol_ao_tamm{tAO, tAO, tCIp};

  cd_mem_req -= sum_tensor_sizes(g_d_tamm, g_r_tamm);
  cd_mem_req += sum_tensor_sizes(g_chol_ao_tamm);
  check_cd_mem_req("resizing the ao cholesky tensor");

  Tensor<TensorType>::allocate(&ec, g_chol_ao_tamm);

  // Convert g_chol_tamm(nD with max_cvecs) to g_chol_ao_tamm(1D with chol_count)
  auto lambdacv = [&](const IndexVector& bid) {
    const IndexVector blockid = internal::translate_blockid(bid, g_chol_ao_tamm());

    auto block_dims   = g_chol_ao_tamm.block_dims(blockid);
    auto block_offset = g_chol_ao_tamm.block_offsets(blockid);

    const tamm::TAMM_SIZE dsize = g_chol_ao_tamm.block_size(blockid);

    int64_t lo[4] = {0, cd_ncast<size_t>(block_offset[0]), cd_ncast<size_t>(block_offset[1]),
                     cd_ncast<size_t>(block_offset[2])};
    int64_t hi[4] = {0, cd_ncast<size_t>(block_offset[0] + block_dims[0] - 1),
                     cd_ncast<size_t>(block_offset[1] + block_dims[1] - 1),
                     cd_ncast<size_t>(block_offset[2] + block_dims[2] - 1)};

    std::vector<TensorType> sbuf(dsize);
#ifdef USE_UPCXX
    g_chol_tamm.get_raw(lo, hi, sbuf.data());
#else
    int64_t ld[2] = {cd_ncast<size_t>(block_dims[1]), cd_ncast<size_t>(block_dims[2])};
    NGA_Get64(g_chol, &lo[1], &hi[1], sbuf.data(), ld);
#endif

    g_chol_ao_tamm.put(blockid, sbuf);
  };

  block_for(ec, g_chol_ao_tamm(), lambdacv);

  Scheduler sch{ec};

  if(do_freeze) {
    Matrix lcao_new;
    if(rank == 0) lcao_new = reshape_mo_matrix(sys_data, lcao_eig, true);
    sch.deallocate(lcao).execute();
    lcao = Tensor<TensorType>{tAO, tMO};
    sch.allocate(lcao).execute();
    if(rank == 0) eigen_to_tamm_tensor(lcao, lcao_new);
  }

  Tensor<TensorType>::deallocate(g_chol_tamm);

  Tensor<TensorType> CholVpr_tmp{tMO, tAO, tCIp};
  Tensor<TensorType> CholVpr_tamm{{tMO, tMO, tCIp},
                                  {SpinPosition::upper, SpinPosition::lower, SpinPosition::ignore}};
  Tensor<TensorType>::allocate(&ec, CholVpr_tmp);

  cd_mem_req -= sum_tensor_sizes(g_chol_tamm);
  cd_mem_req += sum_tensor_sizes(CholVpr_tmp);
  check_cd_mem_req("ao2mo transformation");

  auto [mu, nu]   = tAO.labels<2>("all");
  auto [pmo, rmo] = tMO.labels<2>("all");
  auto cindexp    = tCIp.label("all");

  cd_t1 = std::chrono::high_resolution_clock::now();

  // clang-format off
  // Contraction 1
  sch(CholVpr_tmp(pmo, mu, cindexp) = lcao(nu, pmo) * g_chol_ao_tamm(nu, mu, cindexp))
  .deallocate(g_chol_ao_tamm).execute(ec.exhw());

  cd_mem_req -= sum_tensor_sizes(g_chol_ao_tamm);
  cd_mem_req += sum_tensor_sizes(CholVpr_tamm);
  check_cd_mem_req("the 2-step contraction");

  // Contraction 2
  sch.allocate(CholVpr_tamm)
  (CholVpr_tamm(pmo, rmo, cindexp) = lcao(mu, rmo) * CholVpr_tmp(pmo, mu, cindexp))
  .deallocate(CholVpr_tmp)
  .execute(ec.exhw());
  // clang-format on

  cd_t2   = std::chrono::high_resolution_clock::now();
  cd_time = std::chrono::duration_cast<std::chrono::duration<double>>((cd_t2 - cd_t1)).count();
  if(rank == 0) {
    std::cout << endl
              << "- Time for ao to mo transform: " << std::fixed << std::setprecision(2) << cd_time
              << " secs" << endl;
  }

  if(rank == 0) {
    cout << endl << "    End Cholesky Decomposition" << endl;
    cout << std::string(45, '-') << endl;
  }

  chol_count = count;
  return CholVpr_tamm;
}
