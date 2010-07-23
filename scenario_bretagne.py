#MUMORO CONFIGURATION BEFORE LAUNCHING SERVER

#Database type, choose among : 'sqlite', 'mysql', 'postgres', 'oracle', 'mssql', and 'firebird'
db_type = 'sqlite'

#Database connexion URL
#For user oriented databases : 'username:password@host:port/database'
#Port can be excluded (default one depending on db_type will be used) : 'username:password@host/database'
#For SQLiTE : 'file_name.db' for relative path or absolute : '/data/guidage/file_name.db'
db_params = '/home/ody/mumoro.scenario.bretagne.db'

#Load street data from (compressed or not) osm file(s)
#-----------------------------------------------------
osm_bretagne = import_street_data('/home/ody/Developement/mumoro/bretagne.osm.bz2')

#Load bike service from an API URL (Don't forget to add http://) with required valid params (depending on each API)
#------------------------------------------------------------------------------------------------------------------
bike_rennes = import_bike_service('http://data.keolis-rennes.com/xml/?version=1.0&key=UAYPAP0MHD482NR&cmd=getstation&param[request]=all', 'VeloStar')

#Loads muncipal data file and inserts it into database.
#starting_date & end_date in this format : 'YYYYMMDD' Y for year's digists, M for month's and D for day's
#starting_date and end_date MUST be defined if municipal data is imported
#------------------------------------------------------------------------------------------------------------------
starting_date = ''
end_data = ''

#Create relevant layers from previously imported data (origin paramater) with a name, a color and the mode.
#Color in the html format : '#RRGGBB' with R, G and B respetcly reg, green and blue values in hex
#Mode choose among: mumoro.Foot, mumoro.Bike and mumoro.Car 
#For GTFS Municipal layer dont mention layer mode
#--------------------------------------------------------------------------------------------------------------------
foot_bretagne = load_layer( origin = osm_bretagne, name = 'Foot', color = '#7D0541', mode = mumoro.Foot )
car_bretagne = load_layer( origin = osm_bretagne, name = 'Car', color = '#306EFF', mode = mumoro.Car )
bike_bretagne = load_layer( origin = osm_bretagne, name = 'Velo START', color = '#437C17', mode = mumoro.Bike )


#Starting layer is the layer on wich the route begins
#Destination layer is the layer on wich the route finishes
#Starting & destination layers MUST be selected, otherwise the server could not start
#If by mistake you select more than one starting/destination layers, the affectation will go on the last one
set_starting_layer( foot_bretagne )
set_destination_layer( foot_bretagne )

#Creates a transit cost variable, including the duration in seconds of the transit and if the mode is changed (True or False)
#------------------------------------------------------------------------------------------------------------
#cost( duration, mode_changed ):
cost1 = cost( duration = 120, mode_changed = True )
cost2 = cost( duration = 60, mode_changed = False )

#Connect 2 given layers on same nodes with the given cost(s)
#-----------------------------------------------------------
connect_layers_same_nodes( foot_bretagne , car_bretagne , cost1 )

#Connect 2 given layers on nodes imported from a list (Returned value from import_bike_service or import_municipal_data) with the given cost(s)
#----------------------------------------------------------------------------------------------------------------------------------------------
connect_layers_from_node_list( foot_bretagne, bike_bretagne, bike_rennes, cost1, cost2 )

#Connect 2 given layers on nearest nodes
#----------------------------------------

#Administrator valid email
#REQUIRED for geocoding services, if empty the service will NOT work
#-------------------------
admin_email = 'odysseas.gabrielides@gmail.com'

#Website valid URL : 'http://url/' example 'http://mumoro.openstreetmap.fr/'
#REQUIRED for urls generating (allowing you to send the url to a friend and to find the same route)
#---------------------------------------------------------------------------
web_url = 'http://localhost:3000/' 

#Listening port
#Check that it is free and port-fordwarded if behind a router
#---------------
listening_port = 3000
