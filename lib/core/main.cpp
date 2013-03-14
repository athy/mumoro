
#include "debug/cwd_sys.h"
#include "debug/cwd_debug.h"


#include "reglc_graph.h"
#include "path_algo.h"
#include "muparo.h"

int main() 
{
    Debug( dc::notice.on() );             // Turn on the NOTICE Debug Channel.
    Debug( libcw_do.on() );               // Turn on the default Debug Object.
  
//     Transport::Graph g("/home/arthur/LAAS/mumoro/976c9329c82da079f78be26dddcf1174.dump");
//     Transport::Graph g("/home/arthur/LAAS/Data/Graphs/toulouse-mixed.dump");
    Transport::Graph g("/home/arthur/LAAS/Data/Graphs/midi-pyrennees.dump");
    
    
//     RLC::Graph rlc(&g, RLC::foot_subway_dfa());
//     RLC::BackwardGraph back_rlc( &rlc );
    
//     MuPaRo::Muparo mpr(&g, 50, 600);
//     mpr.run();
//     MuPaRo::Muparo * mup = MuPaRo::bi_point_to_point(&g, 223, 3);
    
    MuPaRo::Muparo * mup = MuPaRo::covoiturage(&g, 277967, 284756, 323542, 303655, RLC::foot_dfa(), RLC::bike_dfa());
    mup->run();
}