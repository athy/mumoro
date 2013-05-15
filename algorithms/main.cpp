#include <boost/foreach.hpp>

#include "debug/cwd_sys.h"
#include "debug/cwd_debug.h"

#include "MultipleParticipants/muparo.h"
#include "MultipleParticipants/run_configurations.h"
#include "utils/node_filter_utils.h"
#include "utils/Area.h"

#include "RegLC/AspectTarget.h"
#include "RegLC/AlgoTypedefs.h"
#include "RegLC/AspectTargetLandmark.h"
#include "RegLC/AspectTargetAreaLandmark.h"
#include "RegLC/LandmarkSet.h"

#include "MultipleParticipants/PotentialMeetingPoints.h"
#include "DataStructures/GraphFactory.h"

#include "MultipleParticipants/MPR_AspectConnectionRule.h"

#include "MultiObjectives/Martins.h"

using namespace RLC;

int main() 
{
    Debug( dc::notice.on() );             // Turn on the NOTICE Debug Channel.
    Debug( libcw_do.on() );               // Turn on the default Debug Object.
//     std::string file( "/home/arthur/LAAS/Data/Graphs/sud-ouest.dump" );
    std::string file( "/home/arthur/LAAS/Data/Graphs/toulouse.dump" );
    Transport::GraphFactory gf( file );
    
    const Transport::Graph * g = gf.get();
    
    RLC::Graph rlc(g, RLC::foot_dfa());
    RLC::BackwardGraph back_rlc( &rlc );
    
//     Area * toulouse = toulouse_area(g);
//     Area * bordeaux = bordeaux_area(g);
//     Area * small = small_area(g);

    int source = 218238; //580025;
    int target = 329194;
    int landmark = source;
    int landmark2 = 302296; //target;
    
    int src1 = 10;
    int src2 = 644;
    int dest = 235;//20; //693;
    
    int time = 50000;
    int day = 10;
    
    typedef AspectListConnections<AspectConnectionRule<Muparo<DRegLC>>> Algo;
    
    Algo::ParamType p(
        MuparoParams( g, 2),
        AspectConnectionRuleParams( SumPlusWaitCost, MaxArrival, &rlc, 0, 1, dest )
    );
    
    Algo algo( p );
    
    for(int i=0; i<algo.num_layers ; ++i)
    {
        algo.dfas.push_back(RLC::car_dfa());
        algo.graphs.push_back( new RLC::Graph(algo.transport, algo.dfas[i] ));
        Algo::Dijkstra::ParamType p( RLC::DRegLCParams( algo.graphs[i], day) );
        algo.dij.push_back( new Algo::Dijkstra( p ) );
    }
    
    algo.insert( StateFreeNode(0, src1), time, 0);
    algo.insert( StateFreeNode(1, src2), time, 0);
    algo.run();
    
    cout <<"Num connections : "<< algo.get_connection_points().size() <<endl;
    
    
    
    typedef RLC::AspectTarget<RLC::DRegLC> Dij;
    
    Dij::ParamType dij_params( 
            RLC::DRegLCParams(&rlc, day),
            RLC::AspectTargetParams( dest ) );
    Dij dij ( dij_params );

    std::list<Label> labels = algo.get_connection_points();
    
//     labels.push_back( Label(612, 50079, 158) );
//     labels.push_back( Label(620, 50134, 268) );
//     labels.push_back( Label(707, 50148, 296) );
    
    int best_cost = std::numeric_limits< int >::max();
    
    BOOST_FOREACH( Label l, labels ) {
        dij.clear();
        cout << l << endl;
        dij.insert_node(RLC::Vertice(l.node, 0), l.time, l.cost);
        dij.run();
        if( dij.get_path_cost() < best_cost )
            best_cost = dij.get_path_cost();
        cout << "Cost : " << dij.get_path_cost() << endl;
    }
    
    cout << "Best cost over all connection points : " << best_cost <<endl;
    
    martins( &rlc, labels, dest );
}