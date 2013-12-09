# Copyright (C) 2013 PSNC
#
# Authors:
#   Damian Parniewicz (PSNC) <damianp_at_man.poznan.pl>

import sys, os, logging, time, subprocess, thread
from optparse import OptionParser
from threading import Thread
import bottle
from deamon import Daemon

##############################################

MODULE_NAME = 'configService'
__version__ = '0.1'

##############################################

global_data = {}

#--------------------------------------------

def save_config():
    f = file(CONFIG_FILE, 'w')
    for queue_id, queue_conf in global_data.items():
        f.write("%s,%s,%s,%s\n" % (queue_id, queue_conf['port-id'], queue_conf['min-rate'], queue_conf['max-rate']))
    f.close()
        
def read_config():
    f = file(CONFIG_FILE, 'r')
    for line in f.readlines():
        line = line.replace('\n', '').split(',')
        global_data[line[0]] = {'port-id':line[1], 'min-rate':line[2], 'max-rate':line[3]}
    f.close()
    logger.info("Initial queues configuration is %s", global_data)
    
#--------------------------------------------
@bottle.put("/config/queue/<queue_id>")
def do_PUT(queue_id):
    logger.info('Creating or updating queue %s', queue_id)
    logger.info(bottle.request.json)
    for attr in [u'port-id', u'min-rate', u'max-rate']:
        if attr not in bottle.request.json:
            bottle.abort(400, "Missing attribute: %s" % attr)
    global_data[queue_id] = bottle.request.json
    save_config()

#--------------------------------------------
@bottle.delete("/config/queue/<queue_id>")
def do_DELETE(queue_id):
    logger.info('Removing queue %s', queue_id)
    if queue_id not in global_data:
        bottle.abort(404, "Queue not found")
    del global_data[queue_id]
    save_config()

#--------------------------------------------

class TriggeringServer(Thread):  
    def __init__(self, dataModels, config):
        '''contructor method required for access to common data model'''
        Thread.__init__(self)
        self.logger = logging.getLogger(self.__class__.__name__)
        self.config = config
        self.logger.debug('Triggering server initialized')
        
    def run(self):
        """Called when server is starting"""   
        logger.info('Running CherryPy HTTP server on port %s', self.config['port'])
        #bottle.run(host='0.0.0.0', port=self.config['port'], debug=True, server='cherrypy')
        bottle.run(host='0.0.0.0', port=self.config['port'], debug=True)

#--------------------------------------------------------

class ModuleDaemon(Daemon):
    def __init__(self, moduleName, options):
        self.logger = logging.getLogger(self.__class__.__name__)
        self.moduleName=moduleName
        self.options = options
        pidFile = "%s/%s.pid" % (self.options.pidDir, self.moduleName)
        self.initializeDataModel()
        Daemon.__init__(self, pidFile)
        self.logger.debug('deamon initialized')

    #---------------------
    def initializeDataModel(self):
        self.dataModels = {
            'config': {
                'port': 9090,
            }
        }
        read_config()

    #---------------------
    def run(self):
        """
        Method called when starting the daemon. 
        """
        try:
            trigServer = TriggeringServer(self.dataModels, self.dataModels['config'])
            trigServer.start()
            # run any more interface server as a Thread
        except:
            import traceback
            self.logger.error("Exception" + traceback.format_exc())

##############################################

if __name__ == "__main__":
    
    # optional command-line arguments processing
    usage="usage: %prog start|stop|restart [options]"
    parser = OptionParser(usage=usage, version="%prog " + __version__)
    parser.add_option("-p", "--pidDir", dest="pidDir", default='/tmp', help="directory for pid file")
    parser.add_option("-l", "--logDir", dest="logDir", default='.', help="directory for log file")
    parser.add_option("-c", "--confFile", dest="confFile", default='.',    help="path for config file")
    options, args = parser.parse_args()
    
    # I do a hack if configDir is default - './' could not point to local dir 
    if options.confFile == '.':
        options.confFile = '/tmp/queues.conf'
        #   options.confDir = sys.path[0]
    
    CONFIG_FILE  = options.confFile
    
    if len(args) == 0:
        print usage
        sys.exit(2)

    if 'start' in args[0]:
        # clear log file
        try:
            os.remove("%s/%s.log" % (options.logDir, MODULE_NAME))
        except: pass

    # creation of logging infrastructure
    logging.basicConfig(filename = "%s/%s.log" % (options.logDir, MODULE_NAME),
                        level    = logging.DEBUG,
                        format   = "%(levelname)s - %(asctime)s - %(name)s - %(message)s")
    logger = logging.getLogger(MODULE_NAME)

    # starting module's daemon
    daemon = ModuleDaemon(MODULE_NAME, options)
    
    # mandatory command-line arguments processing

    if 'start' == args[0]:
        logger.info('starting the module')
        daemon.start()
    elif 'stop' == args[0]:
        logger.info('stopping the module')
        daemon.stop()
    elif 'restart' == args[0]:
        logger.info('restarting the module')
        daemon.restart()
    else:
        print "Unknown command"
        print usage
        sys.exit(2)
    sys.exit(0)

