import sys
from lib.core import mumoro
import osm4routing
from lib import bikestations
from lib import gtfs_reader
import os.path
from lib.datastructures import *
from sqlalchemy import *
from sqlalchemy.orm import mapper, sessionmaker, clear_mappers

street_data_array = []
municipal_data_array = []
bike_service_array = []


class Importer():
    def __init__(self,db_type,db_params):
        if not db_type or not db_params:
            raise NameError('Database connection parameters are empty')
        self.db_string = db_type + ":///" + db_params
        print "Connection string is : " + self.db_string        
        self.engine = create_engine(self.db_string)
        self.metadata = MetaData(bind = self.engine)
        self.mumoro_metadata = Table('metadata', self.metadata,
                Column('id', Integer, primary_key=True),
                Column('node_or_edge', String),
                Column('name', String),
                Column('origin', String)
                )
        self.metadata.create_all()
        mapper(Metadata, self.mumoro_metadata)
        Session = sessionmaker(bind=self.engine)
        self.session = Session()
        if not street_data_array and not municipal_data_array and not bike_service_array:
            raise NameError('No data is imported')
        if street_data_array:
            for f in street_data_array:
                self.import_osm( f )
        else:
            print "No street data is imported"
        if municipal_data_array:
            for m in municipal_data_array:
                self.import_gtfs( m['file'], m['sdate'], m['edate'], m['network'] )
        else:
            print "No municipal data is imported"
        if bike_service_array:
            for b in bike_service_array:
                self.import_bike( b['url_api'], b['service_name'] )
        else:
            print "No bike serivce is imported"

    def init_mappers(self):
        clear_mappers()
        mapper(Metadata, self.mumoro_metadata)

    def import_osm( self, filename ):
        print "Adding street data from " + filename
        nodes = Metadata("OSM nodes", "Nodes", filename)
        edges = Metadata("OSM edges", "Edges", filename)
        self.session.add(nodes)
        self.session.add(edges)
        self.session.commit()
        osm4routing.parse(filename, self.db_string, str(edges.id), str(nodes.id) ) 
        self.init_mappers()
        print "Done importing street data from " + filename

    def import_gtfs(self, filename, start_date, end_date, network_name = "GTFS"):
        print "Adding municipal data from " + filename
        print "From " + start_date + " to " + end_date + " for " + network_name + " network"
        nodes2 = Metadata(network_name, "Nodes", filename)
        self.session.add(nodes2)
        self.session.commit()
        mapper(PT_Node, create_pt_nodes_table(str(nodes2.id), self.metadata))
        edges2 = Metadata(network_name, "Edges", filename)
        self.session.add(edges2)
        self.session.commit()
        mapper(PT_Edge, create_pt_edges_table(str(edges2.id), self.metadata))
        self.session.commit()
        gtfs_reader.convert(filename, self.session, start_date, end_date)
        self.init_mappers()
        print "Done importing municipal data from " + filename

    def import_bike(self, url, name):
        print "Adding public bike service from " + url
        bike_service = Metadata(name, "bike_stations", url )
        self.session.add(bike_service)
        self.session.commit()
        i = bikestations.BikeStationImporter( url, str(bike_service.id), self.metadata, self.session )
        i.import_data()
        self.init_mappers()
        print "Done importing bike service from " + url

#Loads an osm (compressed of not) file and insert data into database
def import_street_data( filename ):
    if not os.path.exists( filename ):
        raise NameError('File does not exist')
    street_data_array.append( filename )

#Loads muncipal data file 
#( 3 cases : GTFS format (Call TransitFeed), Trident format (Call Chouette), Other : manual implementation ) and insert muncipal data into database.
#start_date & end_date in this format : 'YYYYMMDD'
def import_municipal_data( filename, start_date, end_date, network_name = "GTFS"):
    if not os.path.exists( filename ):
        raise NameError('File does not exist')
    if not start_date:
        raise NameError('Enter a starting date')
    if not end_date:
        raise NameError('Enter an end date')
    municipal_data_array.append( {'file': filename, 'sdate': start_date, 'edate': end_date, 'network': network_name } )

#Loads a bike service API ( from already formatted URL ). Insert bike stations in database and enables schedulded re-check.
def import_bike_service( url, name ):
    if not url:
        raise NameError('Enter an url')
    bike_service_array.append( {'url_api': url, 'service_name': name } )

#Loads data from previous inserted data and creates a layer used in multi-modal graph
def load_layer( data, layer_name, layer_mode = None):
    pass

#Creates a transit cost variable, including the duration in seconds of the transit and if the mode is changed
def cost( duration, mode_changed ):
    pass

#Connects 2 given layers on same nodes with the given cost(s)
def connect_layers_same_nodes( layer1, layer2, cost ):
    pass

#Connect 2 given layers on a node list (arg 3 which should be the returned data from import_municipal_data or import_bike_service) with the given cost(s)
def connect_layers_from_node_list( layer1, layer2, node_list, cost, cost2 = None ):
    pass

#Connect 2 given layers on nearest nodes
def connect_layers_on_nearest_nodes( layer1 , layer2, cost ):
    pass

if __name__ == "__main__":
    total = len( sys.argv )
    if total != 2:
        sys.exit("Usage: python data_import.py {config_file.py}")
    if not os.path.exists( os.getcwd() + "/" + sys.argv[1] ):
        raise NameError('Configuration file does not exist')
    exec( file( sys.argv[1] ) )
    Importer(db_type,db_params)


