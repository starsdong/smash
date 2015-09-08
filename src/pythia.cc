/*
 *
 *    Copyright (c) 2015-
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *    If Pythia cite 
 *    T. Sjöstrand, S. Mrenna and P. Skands, JHEP05 (2006) 026,
 *                          Comput. Phys. Comm. 178 (2008) 852.
 *    
 */

#include "include/pythia.h"

#include "include/action.h"
#include "include/fpenvironment.h"
#include "include/forwarddeclarations.h"
#include "include/logging.h"
#include "include/particledata.h"
#include "include/random.h"

#ifdef PYTHIA_FOUND 
#include "Pythia8/Pythia.h"
/* #include "Pythia8/LHAPDFInterface.h" */
#endif 

namespace Smash {  
  /* This function will generate outgoing particles in CM frame from a hard process */
  ParticleList string_excitation(const ParticleList &incoming_particles_) {
      const auto &log = logger<LogArea::Pythia>();  
    /// Disable floating point exception trap for Pythia 
    {
    DisableFloatTraps guard(FE_DIVBYZERO | FE_INVALID);

    #ifdef PYTHIA_FOUND 
	  /* set all necessary parameters for Pythia 
	   * Create Pythia object */
      std::string xmlpath = PYTHIA_XML_DIR;
      log.debug ("Creating Pythia object."); 
      Pythia8::Pythia pythia( xmlpath, false ); 
      /* select only inelastic events: */
      pythia.readString("SoftQCD:inelastic = on");
      /* suppress unnecessary output */
      pythia.readString("Print:quiet = on");
      /* No resonance decays, since the resonances will be handled by SMASH */
      pythia.readString("HadronLevel:Decay = off");
      /* Set the random seed of the Pythia Random Number Generator.
       * Please note: Here we use a random number generated by the
       * SMASH, since every call of pythia.init should produce
       * different events. */
      pythia.readString("Random:setSeed = on");
      std::stringstream buffer1;
      buffer1 << "Random:seed = " << Random::canonical() ;
      pythia.readString(buffer1.str());
      /* set the incoming particles */
      std::stringstream buffer2;
      buffer2 << "Beams:idA = " << incoming_particles_[0].type().pdgcode().get_decimal();
      pythia.readString(buffer2.str());
      log.info("First particle in string excitation: ", incoming_particles_[0].type().pdgcode().get_decimal());    
      std::stringstream buffer3; 
      buffer3 << "Beams:idB = " << incoming_particles_[1].type().pdgcode().get_decimal();
      log.info("Second particle in string excitation: ", incoming_particles_[1].type().pdgcode().get_decimal());
      pythia.readString(buffer3.str());
      /* Calculate the center-of-mass energy of this collision */
      double sqrts = (incoming_particles_[0].momentum() +
             incoming_particles_[1].momentum()).abs();
      std::stringstream buffer4;
      buffer4 << "Beams:eCM = " << sqrts;
      pythia.readString(buffer4.str());
      log.info("Pythia call with eCM = ", buffer4.str());
      /* Initialize. */
      pythia.init();
      /* Short notation for Pythia event */
      Pythia8::Event& event = pythia.event;
      pythia.next();
      ParticleList outgoing_particles_;      
      for (int i = 0; i< event.size(); i++) {
	if (event[i].isFinal()) {
	  if (event[i].isHadron()) {
             const int pythia_id = event[i].id();
             log.debug("PDG ID from Pythia:", pythia_id);
	     std::string s = std::to_string(pythia_id);
             PdgCode pythia_code(s); 
             ParticleData new_particle_(ParticleType::pythiafind());    
             FourVector momentum;
             momentum.set_x0(event[i].e());
             momentum.set_x1(event[i].px());
             momentum.set_x2(event[i].py());
             momentum.set_x3(event[i].pz());
             new_particle_.set_4momentum(momentum);
             log.debug("4-momentum from Pythia: ", momentum);                                
             outgoing_particles_.push_back(new_particle_);   
          }
        }
      }  
    #else
      std::string errMsg = "Pythia 8 not available for string excitation";
      throw std::runtime_error( errMsg );
      ParticleList outgoing_particles_; 
    #endif         
    return outgoing_particles_; 
    }
  }	
}
