/*
 *
 *    Copyright (c) 2013-2019
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */
#include "./include/smash/Agnieszka_distribution.h"

//#include <gsl/gsl_sf_bessel.h>

//#include <cstdio>

#include <cmath>
#include <cstdlib>
#include <iostream>

//#include "smash/constants.h"
//#include "smash/logging.h"
//#include "smash/random.h"



// agnieszka included the following libraries:

// needed for solving for the distribution maximum
// and the chemical potential
#include <gsl/gsl_multiroots.h>
#include <gsl/gsl_integration.h>
// needed to handle GSL errors
//#include <gsl/gsl_vector.h>

namespace agnieszka {  

  /* 
   * ****************************************************************************
   *
   * This block is for:
   * double juttner_distribution_func
   *
   * The name of this function is the same as in distributions.cc;
   * however, the distribution defined here is easier to handle numerically
   * than the one defined originally in SMASH. This is because 
   * exp(-x)/[ 1 + exp(-x) ] is numerically more stable than 1/[ exp(x) + 1 ] .
   *
   * ****************************************************************************
   */

  double juttner_distribution_func
  (double momentum_radial,
   double mass,
   double temperature,
   double effective_chemical_potential,
   double statistics)
  {
    return ( std::exp( -( std::sqrt(momentum_radial * momentum_radial
				    + mass * mass) -
			  effective_chemical_potential ) /
		       temperature) )/
      ( 1 + statistics * std::exp( -( std::sqrt(momentum_radial * momentum_radial
						+ mass * mass) -
				      effective_chemical_potential ) /
				   temperature) );
  }

  /* 
   * ****************************************************************************
   *
   * This block is for:
   * Root equations and GSL procedure for finding the momentum for which the
   * maximum of a given Juttner distribution occurs. This is needed for a method 
   * of sampling the distribution function in which one samples uniformly below
   * the maximum of the distribution).
   *
   * ****************************************************************************
   */
  
  double p_max_root_equation
  (double p,
   double mass,			        
   double temperature,
   double effective_chemical_potential, 
   double statistics)
  {
    const double Ekin = std::sqrt( p * p + mass * mass );
    const double term1 = 2 * (1 +
			      statistics *
			      exp( -( Ekin - effective_chemical_potential )/
				   temperature ) );
    const double term2 = (p * p)/( temperature * Ekin );

    return term1 - term2;
  }

  int p_max_root_equation_for_GSL
  (const gsl_vector * roots_array,
   void * parameters,
   gsl_vector * function) 
  {
    struct ParametersForMaximumMomentumRootFinder * par =
      static_cast<struct ParametersForMaximumMomentumRootFinder*> (parameters);

    const double mass =
      (par->mass);
    const double temperature =
      (par->temperature);
    const double effective_chemical_potential =
      (par->effective_chemical_potential);
    const double statistics =
      (par->statistics);

    const double p_radial = gsl_vector_get(roots_array,0);

    gsl_vector_set (function, 0,
		    p_max_root_equation
		    (p_radial,
		     mass,
		     temperature,
		     effective_chemical_potential, 
		     statistics) );

    return GSL_SUCCESS;
  }

  void cout_state_p_max
  (unsigned int iter,
   gsl_multiroot_fsolver * solver)
  {
    printf ("\n***\nfind_maximum_of_the_distribution(): iter = %3u \t"
	    "x = % .3f \t"
	    "f(x) = % .3e \n",
	    iter,
	    gsl_vector_get (solver->x, 0),
	    gsl_vector_get (solver->f, 0));

  }

  int find_maximum_of_the_distribution
  (double mass,
   double temperature,
   double effective_chemical_potential,
   double statistics,
   double p_max_initial_guess,
   double solution_precision,
   double* p_max)
  {
    const gsl_multiroot_fsolver_type *Solver_name;
    gsl_multiroot_fsolver *Root_finder;

    int status;
    size_t iter = 0;
    size_t initial_guess_update = 0;

    const size_t problem_dimension = 1;

    struct ParametersForMaximumMomentumRootFinder parameters =
      {mass,
       temperature,
       effective_chemical_potential,
       statistics};

    gsl_multiroot_function MaximumOfDistribution =
      {&p_max_root_equation_for_GSL,
       problem_dimension,
       &parameters};

    double roots_array_initial[1] = {p_max_initial_guess};  

    gsl_vector *roots_array = gsl_vector_alloc (problem_dimension);
    gsl_vector_set (roots_array, 0,
		    roots_array_initial[0]);

  
    Solver_name = gsl_multiroot_fsolver_hybrids;
    Root_finder = gsl_multiroot_fsolver_alloc (Solver_name, problem_dimension);
    gsl_multiroot_fsolver_set (Root_finder,
			       &MaximumOfDistribution,
			       roots_array);

    //cout_state_p_max (iter, Root_finder);

    do
      {
	iter++;

	status = gsl_multiroot_fsolver_iterate (Root_finder);

	//cout_state_p_max (iter, Root_finder);

	/*
	 * Check if the solver is stuck
	 */
	if (status)
	  {
	    if (initial_guess_update < 100)
	      {
	        p_max_initial_guess = p_max_initial_guess + 0.05;
	      }
	    else
	      {
		std::cout << "\n\nThe GSL solver"
			  << "\nfind_maximum_of_the_distribution"
			  << "\nis stuck!"
			  << "\n\nInput parameters:"
			  << "\n                  mass [GeV] = "
			  << mass
			  << "\n           temperature [GeV] = "
			  << temperature
			  << "\neffective_chemical_potential = "
			  << effective_chemical_potential
			  << "\n                  statistics = "
			  << statistics
			  << "\n          solution_precision = "
			  << solution_precision
			  << "\n\n" << std::endl;
		std::cout << "The program cannot sample the momenta without "
			  << "calculating the distribution maximum."
			  << "\nTry adjusting the initial guess (which is "
			  << "looped over in the GSL procedure) or the "
			  << "solution precision.\n\n\n"
			  << std::endl;
		throw std::runtime_error
		  ("Distribution maximum calculation returned no result.\n\n");
		continue;
	      }	   
	  }

	status = gsl_multiroot_test_residual (Root_finder->f,
					      solution_precision);

	if (status == GSL_SUCCESS)
	  {
	    p_max[0] = gsl_vector_get (Root_finder->x, 0);
	  }


      }
    while (status == GSL_CONTINUE && iter < 100000);

    gsl_multiroot_fsolver_free (Root_finder);
    gsl_vector_free (roots_array);   
    
    return 0;
  }

  double maximum_of_the_distribution
  (double mass,
   double temperature,
   double effective_chemical_potential,
   double statistics,
   double solution_precision)
  {
    /*
     * Momentum at which the distribution function has its maximum.
     */
    double p_max[1];
    p_max[0] = 0.0;

    /*
     * Initial guess for the value of p_max, in GeV. This value is 
     * looped over within the GSL solver, so that many initial guesses
     * are probed if the solution is not found.
     */
    double initial_guess_p_max = 0.050;
    
    /*
     * Calling the GSL distribution maximum finder
     */
    agnieszka::find_maximum_of_the_distribution
      (mass,
       temperature,
       effective_chemical_potential,
       statistics,
       initial_guess_p_max,
       solution_precision,
       p_max);
	      
    double distribution_function_maximum = p_max[0] * p_max[0] *
      agnieszka::juttner_distribution_func
      (p_max[0],
       mass,
       temperature,
       effective_chemical_potential,
       statistics);

    return distribution_function_maximum;    
  }


}  // namespace agnieszka
