/*
 *
 *    Copyright (c) 2015-2020
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */

#ifndef SRC_INCLUDE_SMASH_CROSSSECTIONS_H_
#define SRC_INCLUDE_SMASH_CROSSSECTIONS_H_

#include <memory>
#include <utility>

#include "forwarddeclarations.h"
#include "isoparticletype.h"
#include "particles.h"
#include "potential_globals.h"
#include "stringprocess.h"

namespace smash {

/// constants related to transition between low and high collision energies
namespace transit_high_energy {
/// transition range in N-pi collisions
const std::array<double, 2> sqrts_range_Npi = {1.9, 2.2};
/** transition range in N-N collisions:
 * Tuned to reproduce experimental exclusive cross section data, and at the same
 * produce excitation functions that are as smooth as possible. The default of
 * a 1 GeV range is preserved.
 */
const std::array<double, 2> sqrts_range_NN = {3.5, 4.5};
/**
 * constant for the lower end of transition region in the case of AQM
 * this is added to the sum of masses
 */
const double sqrts_add_lower = 0.9;
/**
 * constant for the range of transition region in the case of AQM
 * this is added to the sum of masses + sqrts_add_lower
 */
const double sqrts_range = 1.0;

/**
 * Constant offset as to where to turn on the strings and elastic processes
 * for pi pi reactions (this is an exception because the normal AQM behavior
 * destroys the cross-section at very low sqrt_s and around the f2 peak)
 */
const double pipi_offset = 1.12;

/**
 * Constant offset as to where to shift from 2to2 to string
 * processes (in GeV) in the case of KN reactions
 */
const double KN_offset = 15.15;
}  // namespace transit_high_energy

/**
 * The cross section class assembels everything that is needed to
 * calculate the cross section and returns a list of all possible reactions
 * for the incoming particles at the given energy with the calculated cross
 * sections.
 */
class CrossSections {
 public:
  /**
   * Construct CrossSections instance.
   *
   * \param[in] incoming_particles Particles that are reacting.
   * \param[in] sqrt_s Center-of-mass energy of the reaction.
   * \param[in] potentials Potentials at the interacting point. they are
   *            used to calculate the corrections on the thresholds.
   */
  CrossSections(const ParticleList& incoming_particles, const double sqrt_s,
                const std::pair<FourVector, FourVector> potentials);

  /**
   * Generate a list of all possible collisions between the incoming particles
   * with the given c.m. energy and the calculated cross sections.
   * The string processes are not added at this step if it's not triggerd
   * according to the probability. It will then be added in
   * add_all_scatterings in scatteraction.cc
   *
   * \param[in] elastic_parameter Value of the constant global elastic cross
   *            section, if it is non-zero.
   *            The parametrized elastic cross section is used otherwise.
   * \param[in] two_to_one_switch 2->1 reactions enabled?
   * \param[in] included_2to2 Which 2->2 ractions are enabled?
   * \param[in] included_multi Which multi-particle reactions are enabled?
   * \param[in] low_snn_cut Elastic collisions with CME below are forbidden.
   * \param[in] strings_switch Are string processes enabled?
   * \param[in] use_AQM Is the Additive Quark Model enabled?
   * \param[in] strings_with_probability Are string processes triggered
   *            according to a probability?
   * \param[in] nnbar_treatment NNbar treatment through resonance, strings or
   *                                                        none
   * \param[in] string_process a pointer to the StringProcess object,
   *            which is used for string excitation and fragmentation.
   * \param[in] scale_xs Factor by which all (partial) cross sections are scaled
   * \param[in] additional_el_xs Additional constant elastic cross section
   * \return List of all possible collisions.
   */
  CollisionBranchList generate_collision_list(
      double elastic_parameter, bool two_to_one_switch,
      ReactionsBitSet included_2to2,
      MultiParticleReactionsBitSet included_multi, double low_snn_cut,
      bool strings_switch, bool use_AQM, bool strings_with_probability,
      NNbarTreatment nnbar_treatment, StringProcess* string_process,
      double scale_xs, double additional_el_xs) const;

  /**
   * Helper function:
   * Sum all cross sections of the given process list.
   */
  static double sum_xs_of(const CollisionBranchList& list) {
    double xs_sum = 0.0;
    for (auto& proc : list) {
      xs_sum += proc->weight();
    }
    return xs_sum;
  }

  /**
   * Determine the elastic cross section for this collision. If elastic_par is
   * given (and positive), we just use a constant cross section of that size,
   * otherwise a parametrization of the elastic cross section is used
   * (if available). Optional a constant additional elastic cross section is
   * added
   *
   * \param[in] elast_par Elastic cross section parameter from the input file.
   * \param[in] use_AQM Whether to extend elastic cross-sections with AQM.
   * \param[in] add_el_xs Additional constant elastic cross section
   * \param[in] scale_xs Factor by which all (partial) cross sections are scaled
   *
   * \note The additional constant elastic cross section contribution is added
   * after the scaling of the cross section.
   *
   * \return A ProcessBranch object containing the cross section and
   * final-state IDs.
   */
  CollisionBranchPtr elastic(double elast_par, bool use_AQM, double add_el_xs,
                             double scale_xs) const;

  /**
   * Find all resonances that can be produced in a 2->1 collision of the two
   * input particles and the production cross sections of these resonances.
   *
   * Given the data and type information of two colliding particles,
   * create a list of possible resonance production processes
   * and their cross sections.
   *
   * \param[in] prevent_dprime_form In the case of using direct 3-to-2 deuteron
   * reactions, prevent the d' from forming via the decay back reaction.
   *
   * \return A list of processes with resonance in the final state.
   * Each element in the list contains the type of the final-state particle
   * and the cross section for that particular process.
   */
  CollisionBranchList two_to_one(const bool prevent_dprime_form) const;

  /**
   * Return the 2-to-1 resonance production cross section for a given resonance.
   *
   * \param[in] type_resonance Type information for the resonance to be
   * produced.
   * \param[in] cm_momentum_sqr Square of the center-of-mass momentum of the
   * two initial particles.
   *
   * \return The cross section for the process
   * [initial particle a] + [initial particle b] -> resonance.
   */
  double formation(const ParticleType& type_resonance,
                   double cm_momentum_sqr) const;

  /**
   * Find all 2->2 processes which are suppressed at high energies when
   * strings are turned on with probabilites, but important for the
   * production of rare species such as strange particles.
   *
   * This function should call the different, more specific functions for
   * the different scatterings. But so far, only Nucleon-Pion to Hyperon-
   * Kaon scattering is implemented.
   *
   * \return List of all possibe rare 2->2 processes.
   */
  CollisionBranchList rare_two_to_two() const;

  /**
   * Find all inelastic 2->2 processes for the given scattering.
   *
   * This function calls the different, more specific functions for
   * the different scatterings.
   *
   * \param[in] included_2to2 Which 2->2 reactions are enabled?
   * \return List of all possibe inelastic 2->2 processes.
   */
  CollisionBranchList two_to_two(ReactionsBitSet included_2to2) const;

  /**
   * Find all 2->3 processes for the given scattering.
   *
   * This function calls the different, more specific functions for
   * the different scatterings.
   *
   * \return List of all possibe 2->3 processes.
   */
  CollisionBranchList two_to_three() const;

  /**
   * Determine the cross section for string excitations, which is given by the
   * difference between the parametrized total cross section and all the
   * explicitly implemented channels at low energy (elastic, resonance
   * excitation, etc).
   *
   * \param[in] total_string_xs Total cross section for the string process [mb].
   * \param[in] string_process a pointer to the StringProcess object,
   *            which is used for string excitation and fragmentation.
   * \param[in] use_AQM whether to extend string cross-sections with AQM
   * \return List of subprocesses (single-diffractive,
   *        double-diffractive and non-diffractive) with their cross sections.
   *
   * \throw std::runtime_error
   *        if string_process is a null pointer.
   *
   * This method has to be called after all other processes
   * have been determined.
   * \todo Same assumption made by NNbar_annihilation. Resolve.
   */
  CollisionBranchList string_excitation(double total_string_xs,
                                        StringProcess* string_process,
                                        bool use_AQM) const;

  /**
   * Determine the cross section for NNbar annihilation, which is given by the
   * difference between the parametrized total cross section and all the
   * explicitly implemented channels at low energy (in this case only elastic).
   * \param[in] current_xs Sum of all cross sections of already determined
   *                                                     processes
   * \param[in] scale_xs Factor by which all (partial) cross sections are scaled
   * \return Collision Branch with NNbar annihilation process and its cross
   *   section
   *
   * This method has to be called after all other processes
   * have been determined.
   * \todo Same assumption made by string_excitation. Resolve.
   */
  CollisionBranchPtr NNbar_annihilation(const double current_xs,
                                        const double scale_xs) const;

  /**
   * Determine the cross section for NNbar creation, which is given by
   * detailed balance from the reverse reaction. See
   * NNbar_annihilation.
   * \return Collision Branch with NNbar creation process and its cross
   * section
   */
  CollisionBranchList NNbar_creation() const;

  /**
   * Determine 2->3 cross section for the scattering of the given particle
   * types.
   *
   * That the function only depends on the types of particles (plus sqrt(s)) and
   * not on the specific particles, is an assumption needed in order to treat
   * the 3->2 back-reaction with the stochastic criterion, where this function
   * also needs to be called for 3-to-2 collision probability with only types
   * and sqrt(s) known at this point. Therefore the function is also made
   * static.
   *
   * \param[in] type_in1 first scatterning particle type
   * \param[in] type_in2 second scatterning particle type
   * \param[in] sqrts center-of-mass energy of scattering
   * \return cross section for 2->3 process
   */
  static double two_to_three_xs(const ParticleType& type_in1,
                                const ParticleType& type_in2, double sqrts);

  /**
   * Determine the parametrized total cross section at high energies
   * for the given collision, which is non-zero for Baryon-Baryon and
   * Nucleon-Pion scatterings currently.
   *
   * This is rescaled by AQM factors.
   */
  double high_energy() const;

  /**
   * \return the probability whether the scattering between the incoming
   * particles is via string fragmentation or not.
   *
   * If use_transition_probability is true:
   * The string fragmentation is implemented in the same way in GiBUU (Physics
   * Reports 512(2012), 1-124, pg. 33). If the center of mass energy is low, two
   * particles scatter through the resonance channels. If high, the outgoing
   * particles are generated by string fragmentation. If in between, the out-
   * going particles are generated either through the resonance channels or
   * string fragmentation by chance. In detail, the low energy region is from
   * the threshold to (mix_scatter_type_energy - mix_scatter_type_window_width),
   * while the high energy region is from (mix_scatter_type_energy +
   * mix_scatter_type_window_width) to infinity. In between, the probability for
   * string fragmentation increases smoothly from 0 to 1 as the c.m. energy.
   *
   * If use_transition_probability is false:
   * The string fragmentation is implemented similarly to what is in UrQMD
   * (\iref{Bass:1998ca}). If sqrts is lower than some cutoff value, there are
   * no strings. If higher, strings are allowed, with the cross-section being
   * the difference between some parametrized total cross-section and the sum
   * of all other channels, if this parametrization is larger than the sum of
   * the channels. If not, strings are not allowed (this cross-section check
   * is performed directly after the function is called, for technical reasons).
   *
   * Both of these methods are initially implemented for NN and Npi cross-
   * sections, and extended using the AQM to all BB, BM and MM interactions.
   *
   * Baryon-antibaryon annihilation also uses this function to decide whether to
   * produce strings or not.
   * Since there are no other contributions for this process, there are no
   * cutoffs or gradual increase in the probability of this process happening or
   * not, it just requires the proper combination of incoming particles and
   * config parameters.
   *
   * \param[in] strings_switch Is string fragmentation enabled?
   * \param[in] use_transition_probability which algorithm to use for string
   *                         treatment (see Switch_on_String_with_Probability)
   * \param[in] use_AQM whether AQM is activated
   * \param[in] treat_nnbar_with_strings use strings for nnbar treatment?
   */
  double string_probability(bool strings_switch,
                            bool use_transition_probability, bool use_AQM,
                            bool treat_nnbar_with_strings) const;

  /**
   * \param[in] region_lower the lowest sqrts in the transition region [GeV]
   * \param[in] region_upper the highest sqrts in the transition region [GeV]
   * \return probability to have the high energy interaction (via string)
   */
  double probability_transit_high(const double region_lower,
                                  const double region_upper) const;

 private:
  /**
   * Choose the appropriate parametrizations for given incoming particles and
   * return the (parametrized) elastic cross section.
   *
   * \param[in] use_AQM whether AQM is activated
   * \return Elastic cross section
   */
  double elastic_parametrization(bool use_AQM) const;

  /**
   * Determine the (parametrized) elastic cross section for a
   * nucleon-nucleon (NN) collision.
   * \return Elastic cross section for NN
   *
   * \throw std::runtime_error
   *        if positive cross section cannot be specified.
   */
  double nn_el() const;

  /**
   * Determine the elastic cross section for a nucleon-pion (Npi) collision.
   * It is given by a parametrization of experimental data.
   * \return Elastic cross section for Npi
   *
   * \throw std::runtime_error
   *        if incoming particles are not nucleon+pion.
   * \throw std::runtime_error
   *        if positive cross section cannot be specified.
   */
  double npi_el() const;

  /**
   * Determine the elastic cross section for a nucleon-kaon (NK) collision.
   * It is given by a parametrization of experimental data.
   * \return Elastic cross section for NK
   *
   * \throw std::runtime_error
   *        if incoming particles are not nucleon+kaon.
   * \throw std::runtime_error
   *        if positive cross section cannot be specified.
   */
  double nk_el() const;

  /**
   * Find all processes for Nucleon-Pion to Hyperon-Kaon Scattering.
   * These scatterings are suppressed at high energies when strings are
   * turned on with probabilities, so they need to be added back manually.
   *
   * \return List of all possible Npi -> YK reactions
   *          with their cross sections
   */
  CollisionBranchList npi_yk() const;

  /**
   * Find all inelastic 2->2 processes for Baryon-Baryon (BB) Scattering
   * except the more specific Nucleon-Nucleon Scattering.
   * \param[in] included_2to2 Which 2->2 reactions are enabled?
   * \return List of all possible BB reactions with their cross sections
   */
  CollisionBranchList bb_xx_except_nn(ReactionsBitSet included_2to2) const;

  /**
   * Find all inelastic 2->2 processes for Nucelon-Nucelon Scattering.
   * Calculate cross sections for resonance production from
   * nucleon-nucleon collisions (i.e. N N -> N R, N N -> Delta R).
   *
   * Checks are processed in the following order:
   * 1. Charge conservation
   * 2. Isospin factors (Clebsch-Gordan)
   * 3. Enough energy for all decay channels to be available for the resonance
   *
   * \param[in] included_2to2 Which 2->2 reactions are enabled?
   *
   * \return List of resonance production processes possible in the collision
   * of the two nucleons. Each element in the list contains the type(s) of the
   * final state particle(s) and the cross section for that particular process.
   */
  CollisionBranchList nn_xx(ReactionsBitSet included_2to2) const;

  /**
   * Find all inelastic 2->2 background processes for Nucleon-Kaon (NK)
   * Scattering.
   * \param[in] included_2to2 Which 2->2 reactions are enabled?
   * \return List of all possible NK reactions with their cross sections
   */
  CollisionBranchList nk_xx(ReactionsBitSet included_2to2) const;

  /**
   * Find all inelastic 2->2 processes for Delta-Kaon (DeltaK) Scattering.
   * \param[in] included_2to2 Which 2->2 reactions are enabled?
   * \return List of all possible DeltaK reactions with their cross sections
   * */
  CollisionBranchList deltak_xx(ReactionsBitSet included_2to2) const;

  /**
   * Find all inelastic 2->2 processes for Hyperon-Pion (Ypi) Scattering.
   * \param[in] included_2to2 Which 2->2 reactions are enabled?
   * \return List of all possible Ypi reactions with their cross sections
   */
  CollisionBranchList ypi_xx(ReactionsBitSet included_2to2) const;

  /**
   * Find all inelastic 2->2 processes involving Pion and (anti-) Deuteron
   * (dpi), specifically dπ→ NN, d̅π→ N̅N̅; πd→ πd' (mockup for πd→ πnp), πd̅→ πd̅'
   * and reverse.
   * \param[in] included_2to2 Which 2->2 reactions are enabled?
   * \return List of all possible dpi reactions with their cross sections
   */
  CollisionBranchList dpi_xx(ReactionsBitSet included_2to2) const;

  /**
   * Find all inelastic 2->2 processes involving Nucleon and (anti-) Deuteron
   * (dN), specifically Nd → Nd', N̅d →  N̅d', N̅d̅→ N̅d̅', Nd̅→ Nd̅' and reverse (e.g.
   * Nd'→ Nd).
   * \param[in] included_2to2 Which 2->2 reactions are enabled?
   * \return List of all possible dN reactions with their cross sections
   */
  CollisionBranchList dn_xx(ReactionsBitSet included_2to2) const;

  /**
   * Parametrized cross section for πd→ πd' (mockup for πd→ πnp), πd̅→ πd̅' and
   * reverse, see \iref{Oliinychenko:2018ugs} for details.
   * \param[in] sqrts square-root of mandelstam s
   * \param[in] cm_mom center of mass momentum of incoming particles
   * \param[in] produced_nucleus type of outgoing deuteron or d-prime
   * \param[in] type_pi type of scattering pion
   * \return cross section for given scattering
   */
  static double xs_dpi_dprimepi(const double sqrts, const double cm_mom,
                                ParticleTypePtr produced_nucleus,
                                const ParticleType& type_pi);

  /**
   * Parametrized cross section for Nd → Nd', N̅d →  N̅d', N̅d̅→ N̅d̅', Nd̅→ Nd̅' and
   * reverse (e.g. Nd'→ Nd), see \iref{Oliinychenko:2018ugs} for details.
   * \param[in] sqrts square-root of mandelstam s
   * \param[in] cm_mom center of mass momentum of incoming particles
   * \param[in] produced_nucleus type of outgoing deuteron or d-prime
   * \param[in] type_nucleus type of scattering (incoming) deuteron or d-prime
   * \param[in] type_N type of scattering nucleon
   * \return cross section for given scattering
   */
  static double xs_dn_dprimen(const double sqrts, const double cm_mom,
                              ParticleTypePtr produced_nucleus,
                              const ParticleType& type_nucleus,
                              const ParticleType& type_N);

  /**
   * Determine the (parametrized) hard non-diffractive string cross section
   * for this collision.
   *
   * \return Parametrized cross section (without AQM scaling).
   */
  double string_hard_cross_section() const;

  /**
   * Calculate cross sections for resonance absorption
   * (i.e. NR->NN and ΔR->NN).
   *
   * \param[in] is_anti_particles Whether the colliding particles are
   * antiparticles
   *
   * \return List of possible resonance absorption processes. Each element of
   * the list contains the types of the final-state particles and the cross
   * section for that particular process.
   */
  CollisionBranchList bar_bar_to_nuc_nuc(const bool is_anti_particles) const;

  /**
   * Scattering matrix amplitude squared (divided by 16π) for resonance
   * production processes like NN → NR and NN → ΔR, where R is a baryon
   * resonance (Δ, N*, Δ*). Includes no spin or isospin factors.
   *
   * \param[in] sqrts sqrt(Mandelstam-s), i.e. collision CMS energy.
   * \param[in] type_a Type information for the first final-state particle.
   * \param[in] type_b Type information for the second final-state particle.
   * \param[in] twoI Twice the total isospin of the involved state.
   *
   * \return Matrix amplitude squared \f$ |\mathcal{M}(\sqrt{s})|^2/16\pi \f$.
   */
  static double nn_to_resonance_matrix_element(double sqrts,
                                               const ParticleType& type_a,
                                               const ParticleType& type_b,
                                               const int twoI);

  /**
   * Utility function to avoid code replication in nn_xx().
   * \param[in] type_res_1 List of possible first final resonance types
   * \param[in] type_res_2 List of possible second final resonance types
   * \param[in] integrator Used to integrate over the kinematically allowed
   * mass range of the Breit-Wigner distribution
   * \return List of all possible NN reactions with their cross sections
   * with different final states
   */
  template <class IntegrationMethod>
  CollisionBranchList find_nn_xsection_from_type(
      const ParticleTypePtrList& type_res_1,
      const ParticleTypePtrList& type_res_2,
      const IntegrationMethod integrator) const;

  /**
   * Determine the momenta of the incoming particles in the
   * center-of-mass system.
   * \return Center-of-mass momentum
   */
  double cm_momentum() const {
    const double m1 = incoming_particles_[0].effective_mass();
    const double m2 = incoming_particles_[1].effective_mass();
    return pCM(sqrt_s_, m1, m2);
  }

  /// List with data of scattering particles.
  const ParticleList incoming_particles_;

  /// Total energy in the center-of-mass frame.
  const double sqrt_s_;

  /**
   * Potentials at the interacting point.
   * They are used to calculate the corrections on the threshold energies.
   */
  const std::pair<FourVector, FourVector> potentials_;

  /// Whether incoming particles are a baryon-antibaryon pair
  const bool is_BBbar_pair_;

  /**
   * Helper function:
   * Add a 2-to-2 channel to a collision branch list given a cross section.
   *
   * The cross section is only calculated if there is enough energy
   * for the process. If the cross section is small, the branch is not added.
   */
  template <typename F>
  void add_channel(CollisionBranchList& process_list, F&& get_xsection,
                   double sqrts, const ParticleType& type_a,
                   const ParticleType& type_b) const {
    const double sqrt_s_min =
        type_a.min_mass_spectral() + type_b.min_mass_spectral();
    /* Determine wether the process is below the threshold. */
    double scale_B = 0.0;
    double scale_I3 = 0.0;
    bool is_below_threshold;
    FourVector incoming_momentum = FourVector();
    if (pot_pointer != nullptr) {
      for (const auto p : incoming_particles_) {
        incoming_momentum += p.momentum();
        scale_B += pot_pointer->force_scale(p.type()).first;
        scale_I3 +=
            pot_pointer->force_scale(p.type()).second * p.type().isospin3_rel();
      }
      scale_B -= pot_pointer->force_scale(type_a).first;
      scale_I3 -=
          pot_pointer->force_scale(type_a).second * type_a.isospin3_rel();
      scale_B -= pot_pointer->force_scale(type_b).first;
      scale_I3 -=
          pot_pointer->force_scale(type_b).second * type_b.isospin3_rel();
      is_below_threshold = (incoming_momentum + potentials_.first * scale_B +
                            potentials_.second * scale_I3)
                               .abs() <= sqrt_s_min;
    } else {
      is_below_threshold = (sqrts <= sqrt_s_min);
    }
    if (is_below_threshold) {
      return;
    }
    const auto xsection = get_xsection();
    if (xsection > really_small) {
      process_list.push_back(make_unique<CollisionBranch>(
          type_a, type_b, xsection, ProcessType::TwoToTwo));
    }
  }
};

}  // namespace smash

#endif  // SRC_INCLUDE_SMASH_CROSSSECTIONS_H_
