// Définit les structures de données pour un graphe
// permettant un calcul d'itinéraire multiobjectif
//


#include <boost/pending/mutable_queue.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/array.hpp>
#include <boost/graph/adj_list_serialize.hpp>
#include <boost/serialization/list.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/foreach.hpp>
#include <boost/progress.hpp>

#include <iostream>
#include <fstream>

#ifndef GRAPH_H
#define GRAPH_H


using namespace boost;
using namespace boost::multi_index;

// La structure est templatée en fonction du nombre d'objectifs
template<int N>
struct Graph
{
    static const int objectives = N;

    bool dominates(boost::array<float, N> a, boost::array<float, N> b)
    {
        for(int i=0; i < N; i++)
        {
            if(a[i] > b[i])
                return true;
        }
        //Si on veut qu'il y ait au moins un i tel que a[i] < b[i] 
        return a != b;
    }


    struct Node
    {
        static const int undefined = -1;
        int order; // Définit l'ordre du nœud dans le graphe
        int id; // Identifiant orginal
        int priority; // Valeur de l'heuristique TODO: penser à le foutre à l'extérieur de la map...

        Node() : order(undefined) {} // Par défaut, l'ordre du nœud est indéfini

        //Permet de sérialiser dans un fichier le graphe
        template<class Archive>
            void serialize(Archive& ar, const unsigned int version)
            {
                ar & order & id & priority;
            }
    };

    struct Edge
    {
        float cost0;
        boost::array<float, N> cost; // Coût multiobjectif de l'arc
        std::list<int> shortcuted; // Nœuds court-circuités par cet arc. La liste est vide si c'est un arc original

        //Permet de sérialiser dans un fichier le graphe
        template<class Archive>
            void serialize(Archive& ar, const unsigned int version)
            {
                ar & cost & shortcuted;
            }
    };

    //NOTE: comme on modifie à tours de bras les arcs, il est important que les arcs soient
    //stockés dans une liste (problème de perfs et d'invalidation d'itérateurs)
    // (c'est le premier paramètre du template)
    typedef boost::adjacency_list<boost::listS, boost::vecS, boost::bidirectionalS, Node, Edge > Type;
    typedef typename boost::graph_traits<Type>::edge_descriptor edge_t;
    typedef typename boost::graph_traits<Type>::vertex_descriptor node_t;

    struct SimpleLabel
    {
        node_t node;
        boost::array<float, N> cost;

        bool operator==(const SimpleLabel & l)
        {
            return cost == l.cost;
        }

        bool operator<(const SimpleLabel & l)
        {
            return cost < l.cost;
        }
    };


    Type graph;

    Graph()
    {}

    Graph(std::string filename)
    {
        std::cout << "Loading graph from file " << filename << std::endl;
        std::ifstream ifile(filename.c_str());
        boost::archive::binary_iarchive iArchive(ifile);
        iArchive >> graph;   
        std::cout << "   " << boost::num_vertices(graph) << " nodes" << std::endl;
        std::cout << "   " << boost::num_edges(graph) << " edges" << std::endl;
    }

    node_t add_node()
    {
        return boost::add_vertex(graph);
    }

    node_t add_node(Node n)
    {
        return boost::add_vertex(n, graph);
    }

    std::pair<edge_t, bool> add_edge(node_t source, node_t target, Edge edge)
    {
        return boost::add_edge(source, target, edge, graph);
    }

    void save(std::string filename)
    {
        std::ofstream ofile(filename.c_str());
        boost::archive::binary_oarchive oArchive(ofile);
        oArchive << graph;
    }

    int num_vertices() const
    {
        return boost::num_vertices(graph);
    }

    // Calcule la priorité d'un nœud. Il s'agit d'une valeur purement heuristique pour
    // classer les nœuds
    // On utilise l'heuristique Edge-Diffirence qui calcule combien d'arcs seront rajoutés
    // par rapport au nombre d'arcs supprimés
    // On simule donc la suppression
    //
    // Un tout petit changement de cette heuristique, et le temps de contraction et d'execution varient d'un facteur 10 (et pas toujours dans le même sens..
    int node_priority(typename Graph::node_t node)
    {
        int added = 0;
        //Pour tous les prédecsseurs de node
        BOOST_FOREACH(edge_t in_edge, boost::in_edges(node, graph))
        {
            node_t pred = boost::source(in_edge, graph);
            if(graph[pred].order == Graph::Node::undefined)
            {
                //Pour tous les successeurs de node
                BOOST_FOREACH(edge_t out_edge, boost::out_edges(node, graph))
                {
                    node_t succ = boost::target(out_edge, graph);
                    if(graph[succ].order == Graph::Node::undefined)
                    {
                        Edge edge_prop(graph[in_edge]);
                        for(int i=0; i < Graph::objectives ; i++)
                        {
                            edge_prop.cost[i] += graph[out_edge].cost[i];
                        }

                        //Il n'existe pas de plus court chemin entre pred et succ
                        //                    if (!witness_martins<N>(pred, succ, graph, edge_prop.cost))
                        {
                            added++;
                        }
                    }
                }
            }
        }
        return added - (boost::out_degree(node, graph) + boost::in_degree(node, graph));
    }


    // Fonction qui effectivement élimine le nœud en le court-circuitant par des arcs
    // On étant un prédecesseur s et un successeur t, avec s > t
    // on crée l'arc s->t donc le cout est  c(s->u) + c(u->t)
    // s'il n'existe aucun "witness" (autre chemin de moindre coût)
    //
    // Retourne le nombre d'arcs rajoutés
    int suppress(node_t node)
    {
        int shortcuts = 0;
        //Pour tous les prédecsseurs de node
        BOOST_FOREACH(edge_t in_edge, boost::in_edges(node, graph))
        {
            node_t pred = boost::source(in_edge, graph);
            if(graph[pred].order == Graph::Node::undefined)
            {
                //Pour tous les successeurs de node
                BOOST_FOREACH(edge_t out_edge, boost::out_edges(node, graph))
                {
                    node_t succ = boost::target(out_edge, graph);
                    if(graph[succ].order == Graph::Node::undefined && pred != succ)
                    {
                        //Il n'existe pas de plus court chemin entre pred et succ
                        // Bon... bizarrement, ça ne change rien... un simple test d'existance
                        // d'un arc domminé pourrait ptet suffir... Ça contredit un peu le mémoire
                        // de Geisberger
                   //     if (!witness_martins(pred, succ, edge_prop.cost))
                        {
                            // On note le nœud court-circuité pour dérouler le chemin à la fin de la requète

                            // Note sur les itérateurs :
                            // Ce n'est faisable que parce que les arcs sont stoqués sous forme de listS
                            // Sinon ça merde sur les itérateurs dans la boucle
                            Edge edge_prop(graph[in_edge]);
                            for(int i=0; i < N; i++)
                            {
                                edge_prop.cost[i] += graph[out_edge].cost[i];
                            }
    bool exists;
                            edge_t existing_edge;
                            boost::tie(existing_edge, exists) = boost::edge(pred, succ, graph);
                            if(exists)
                            {
                                //Si le coût est dominé par l'existant, on ne fait rien
                                //if(dominates(graph[existing_edge].cost,  edge_prop.cost))
                                if(graph[existing_edge].cost[0] < edge_prop.cost[0])
                                {
                                }
                                //Si le nouveau cout domine, on modifie la valeur
//                                else if(dominates(edge_prop.cost, graph[existing_edge].cost))
                                else if(true)
                                {
                                    graph[existing_edge].cost = edge_prop.cost;
                                }
                                //Sinon on le rajoute en parallèle
                                else
                                {
                                    edge_prop.shortcuted.push_back(node);
                                    boost::add_edge(pred, succ, edge_prop, graph);
                                    shortcuts++;
                                }
                            }
                            else
                            {
                                //Propriétés du nouvel arc
                                edge_prop.shortcuted.push_back(node);
                                boost::add_edge(pred, succ, edge_prop, graph);
                                shortcuts++;
                            }
                        }
                    }
                }
            }
        }
        return shortcuts;
    }


    // Functor comparant la priorité d'un nœud
    // TODO : y'a vraiment besoin de passer le graph en réf ?
    struct priority_comp
    {
        const Type & g; 
        priority_comp(const Type & graph) : g(graph) {}

        bool operator()(node_t a, node_t b) const
        {
            return (g[a].priority < g[b].priority);
        }
    };

    //Fonction globale qui effectue la contraction
    //Phase 1 : trier les nœuds par leur priorité
    //Phase 2 : prendre récursivement le plus petit nœud non encore traité et l'éliminer
    //Phase 3 : on supprime tous 
    void contract()
    {
        int shortcuts = 0;

        boost::mutable_queue<node_t, std::vector<node_t>, priority_comp> queue(boost::num_vertices(graph), priority_comp(graph), boost::identity_property_map());
        std::cout << "Nombre d'arcs avant la contraction : " << boost::num_edges(graph) << std::endl;
        //Phase 1
        int positive = 0;
        int negative = 0;
        int zero = 0;
        std::cout << "Simulation de la contraction pour déterminer l'ordre de contraction" << std::endl;
        boost::progress_display show_progress( boost::num_vertices(graph) );
        BOOST_FOREACH(node_t node, boost::vertices(graph))
        {
            int i = node_priority(node);
            graph[node].priority = i;
            graph[node].order = Node::undefined;
            if(i < 0)
                negative++;
            else if(i == 0)
                zero++;
            else
                positive++;

            queue.push(node);
            ++show_progress;
        }
        std::cout << "Nombre de nœuds à edge diff positive : " << positive << std::endl;
        std::cout << "Nombre de nœuds à edge diff nulle : " << zero << std::endl;
        std::cout << "Nombre de nœuds à edge diff négative : " << negative << std::endl << std::endl;

        int current_order = 0;
        std::cout << "Effectue la contraction à proprement parler" << std::endl;
        boost::progress_display show_progress2( boost::num_vertices(graph) );
        while( !queue.empty() )
        {
            node_t node = queue.top();
            if(node_priority(node) != graph[node].priority)
            {
                //Bon gros hack car je ne vois pas comment itérer sur les élements de la queue
                //TODO: itérer que sur la queue
                BOOST_FOREACH(node_t n, boost::vertices(graph))
                {
                    if(graph[n].order == Graph::Node::undefined)
                    {
                        graph[n].priority = node_priority(n);
                        queue.update(n);
                    }
                }
            }

            node = queue.top();
            queue.pop();
            BOOST_ASSERT(graph[node].order == Node::undefined);

            graph[node].order = current_order++;
            shortcuts += suppress(node);

            //On met à jour la priorité des nœuds sortant (les entrants sont soit déjà traités,
            //soit l'arc a été supprimé car c'est forcément un nœud plus grand)
            BOOST_FOREACH(edge_t out_edge, boost::out_edges(node, graph))
            {
                node_t succ = boost::target(out_edge, graph);
                if(graph[succ].order == Node::undefined)
                {
                    graph[succ].priority = node_priority(succ); 
                    queue.update(succ);
                }
            }

            ++show_progress2;
        }

        std::cout << "Nombre de racourcis ajoutés : " << shortcuts << std::endl;
        std::cout << "Nombre d'arcs après la contraction : " << boost::num_edges(graph) << std::endl;
    }


};

#endif