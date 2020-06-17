/*
 *
 *    Copyright (c) 2015-2019
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */

#ifndef SRC_INCLUDE_SCATTERACTIONMULTI_H_
#define SRC_INCLUDE_SCATTERACTIONMULTI_H_

#include "action.h"

namespace smash {

/**
 * \ingroup action
 * ScatterActionMulti is a special action which takes any number of incoming particles
 * and performs a scattering with the use of the stochastic criterion,
 * producing one or more final-state particles.
 */
class ScatterActionMulti : public Action {
 public:
 /**
  * Construct a ScatterActionMulit object.
  *
  * \param[in] in_plist List of reaction partners
  * \param[in] time Time at which the action is supposed to take place
  */
  ScatterActionMulti(const ParticleList& in_plist, double time);

  /**
   * Generate the final-state of the multi-particle scattering process.
   * Assign position and momenta to outgoing particles.
   *
   * \throw InvalidScatterActionMulti
   */
  void generate_final_state() override;

  /**
   * Get the total probability for the reaction.
   *
   * \return total probability.
   */
  double get_total_weight() const override;

  /**
   * Get the partial probability for the chosen channel.
   *
   * \return partial probability.
   */
  double get_partial_weight() const override;

  /**
   * Add all possible multi-particle reactions for the given incoming particles.
   *
   * \param[in] dt timestep size
   * \param[in] gcell_vol gcell_vol grid cell volume
   * \param[in] three_to_one 3->1 reactions enabled?
   */
  void add_possible_reactions(double dt, const double gcell_vol, const bool three_to_one);

  /**
   * Get the total probability for the reaction. More expressive get function
   * than get_total_weight.
   *
   * \return total probability.
   */
  double probability() const { return total_probability_; }

  /**
   * \ingroup exception
   * Thrown when ScatterActionMulti is called to perform with unknown
   * combination of incoming and outgoing number of particles or unknown process
   * type.
   */
  class InvalidScatterActionMulti : public std::invalid_argument {
    using std::invalid_argument::invalid_argument;
  };

 protected:
  /*
   * \ingroup logging
   * Writes information about this action to the \p out stream.
   */
  void format_debug_output(std::ostream& out) const override;

 private:
 /**
  * Add a new reaction channel.
  *
  * \param[in] p Channel to be added.
  */
  void add_reaction(CollisionBranchPtr p);

  /**
   * Add several new reaction channels at once.
   *
   * \param[in] pv list of channels to be added.
   */
  void add_reactions(CollisionBranchList pv);

  /**
   * Perform a n->1 annihilation process.
   * \throw InvalidScatterActionMulti
   */
  void annihilation();

  /**
   * Calculate the probability for a 3pi-to-1 reaction according to the
   * stochastic collision criterion (TODO(stdnmr) Add ref here).
   *
   * TODO(stdnmr) Add formula
   *
   * \param[in] type_out type of outgoing particle
   * \param[in] dt timestep size
   * \param[in] gcell_vol grid cell volume
   * \return probabilty for 3pi-to-1 reaction
   */
  double probability_three_pi_to_one(const ParticleType& type_out, double dt,
                                       const double gcell_vol) const;

 /**
  * Check wether the three incoming particles are π⁺,π⁻,π⁰ in any order.
  * Wrapper for unwieldy if statment.
  *
  * \param[in] data_a data for first incoming particle
  * \param[in] data_b data for second incoming particle
  * \param[in] data_c data for third incoming particle
  * \return true if combination of π⁺,π⁻,π⁰
  */
  bool three_different_pions(const ParticleData& data_a,
                             const ParticleData& data_b,
                             const ParticleData& data_c) const;

  /// Total probability of reaction
  double total_probability_;

  /// Partial probability of the chosen outgoing channel
  double partial_probability_;

  /// List of possible collisions
  CollisionBranchList reaction_channels_;
};

}  // namespace smash

#endif  // SRC_INCLUDE_SCATTERACTIONMULTI_H_
