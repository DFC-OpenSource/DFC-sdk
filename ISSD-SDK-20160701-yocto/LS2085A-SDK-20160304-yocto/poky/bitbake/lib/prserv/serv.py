import os,sys,logging
import signal, time, atexit, threading
from SimpleXMLRPCServer import SimpleXMLRPCServer, SimpleXMLRPCRequestHandler
import xmlrpclib
import threading
import Queue

try:
    import sqlite3
except ImportError:
    from pysqlite2 import dbapi2 as sqlite3

import bb.server.xmlrpc
import prserv
import prserv.db
import errno

logger = logging.getLogger("BitBake.PRserv")

if sys.hexversion < 0x020600F0:
    print("Sorry, python 2.6 or later is required.")
    sys.exit(1)

class Handler(SimpleXMLRPCRequestHandler):
    def _dispatch(self,method,params):
        try:
            value=self.server.funcs[method](*params)
        except:
            import traceback
            traceback.print_exc()
            raise
        return value

PIDPREFIX = "/tmp/PRServer_%s_%s.pid"
singleton = None


class PRServer(SimpleXMLRPCServer):
    def __init__(self, dbfile, logfile, interface, daemon=True):
        ''' constructor '''
        import socket
        try:
            SimpleXMLRPCServer.__init__(self, interface,
                                        logRequests=False, allow_none=True)
        except socket.error:
            ip=socket.gethostbyname(interface[0])
            port=interface[1]
            msg="PR Server unable to bind to %s:%s\n" % (ip, port)
            sys.stderr.write(msg)
            raise PRServiceConfigError

        self.dbfile=dbfile
        self.daemon=daemon
        self.logfile=logfile
        self.working_thread=None
        self.host, self.port = self.socket.getsockname()
        self.pidfile=PIDPREFIX % (self.host, self.port)

        self.register_function(self.getPR, "getPR")
        self.register_function(self.quit, "quit")
        self.register_function(self.ping, "ping")
        self.register_function(self.export, "export")
        self.register_function(self.importone, "importone")
        self.register_introspection_functions()

        self.db = prserv.db.PRData(self.dbfile)
        self.table = self.db["PRMAIN"]

        self.requestqueue = Queue.Queue()
        self.handlerthread = threading.Thread(target = self.process_request_thread)
        self.handlerthread.daemon = False

    def process_request_thread(self):
        """Same as in BaseServer but as a thread.

        In addition, exception handling is done here.

        """
        iter_count = 1
        # 60 iterations between syncs or sync if dirty every ~30 seconds
        iterations_between_sync = 60

        while not self.quit:
            try:
                (request, client_address) = self.requestqueue.get(True, 30)
            except Queue.Empty:
                self.table.sync_if_dirty()
                continue
            try:
                self.finish_request(request, client_address)
                self.shutdown_request(request)
                iter_count = (iter_count + 1) % iterations_between_sync
                if iter_count == 0:
                    self.table.sync_if_dirty()
            except:
                self.handle_error(request, client_address)
                self.shutdown_request(request)
                self.table.sync()
            self.table.sync_if_dirty()

    def process_request(self, request, client_address):
        self.requestqueue.put((request, client_address))

    def export(self, version=None, pkgarch=None, checksum=None, colinfo=True):
        try:
            return self.table.export(version, pkgarch, checksum, colinfo)
        except sqlite3.Error as exc:
            logger.error(str(exc))
            return None

    def importone(self, version, pkgarch, checksum, value):
        return self.table.importone(version, pkgarch, checksum, value)

    def ping(self):
        return not self.quit

    def getinfo(self):
        return (self.host, self.port)

    def getPR(self, version, pkgarch, checksum):
        try:
            return self.table.getValue(version, pkgarch, checksum)
        except prserv.NotFoundError:
            logger.error("can not find value for (%s, %s)",version, checksum)
            return None
        except sqlite3.Error as exc:
            logger.error(str(exc))
            return None

    def quit(self):
        self.quit=True
        return

    def work_forever(self,):
        self.quit = False
        self.timeout = 0.5

        logger.info("Started PRServer with DBfile: %s, IP: %s, PORT: %s, PID: %s" %
                     (self.dbfile, self.host, self.port, str(os.getpid())))

        self.handlerthread.start()
        while not self.quit:
            self.handle_request()
        self.handlerthread.join()
        self.table.sync()
        logger.info("PRServer: stopping...")
        self.server_close()
        return

    def start(self):
        pid = self.daemonize()
        # Ensure both the parent sees this and the child from the work_forever log entry above
        logger.info("Started PRServer with DBfile: %s, IP: %s, PORT: %s, PID: %s" %
                     (self.dbfile, self.host, self.port, str(pid)))

    def delpid(self):
        os.remove(self.pidfile)

    def daemonize(self):
        """
        See Advanced Programming in the UNIX, Sec 13.3
        """
        try:
            pid = os.fork()
            if pid > 0:
                os.waitpid(pid, 0)
                #parent return instead of exit to give control 
                return pid
        except OSError as e:
            raise Exception("%s [%d]" % (e.strerror, e.errno))

        os.setsid()
        """
        fork again to make sure the daemon is not session leader, 
        which prevents it from acquiring controlling terminal
        """
        try:
            pid = os.fork()
            if pid > 0: #parent
                os._exit(0)
        except OSError as e:
            raise Exception("%s [%d]" % (e.strerror, e.errno))

        os.umask(0)
        os.chdir("/")

        sys.stdout.flush()
        sys.stderr.flush()
        si = file('/dev/null', 'r')
        so = file(self.logfile, 'a+')
        se = so
        os.dup2(si.fileno(),sys.stdin.fileno())
        os.dup2(so.fileno(),sys.stdout.fileno())
        os.dup2(se.fileno(),sys.stderr.fileno())

        # Clear out all log handlers prior to the fork() to avoid calling
        # event handlers not part of the PRserver
        for logger_iter in logging.Logger.manager.loggerDict.keys():
            logging.getLogger(logger_iter).handlers = []

        # Ensure logging makes it to the logfile
        streamhandler = logging.StreamHandler()
        streamhandler.setLevel(logging.DEBUG)
        formatter = bb.msg.BBLogFormatter("%(levelname)s: %(message)s")
        streamhandler.setFormatter(formatter)
        logger.addHandler(streamhandler)

        # write pidfile
        pid = str(os.getpid()) 
        pf = file(self.pidfile, 'w')
        pf.write("%s\n" % pid)
        pf.close()

        self.work_forever()
        self.delpid()
        os._exit(0)

class PRServSingleton(object):
    def __init__(self, dbfile, logfile, interface):
        self.dbfile = dbfile
        self.logfile = logfile
        self.interface = interface
        self.host = None
        self.port = None

    def start(self):
        self.prserv = PRServer(self.dbfile, self.logfile, self.interface)
        self.prserv.start()
        self.host, self.port = self.prserv.getinfo()

    def getinfo(self):
        return (self.host, self.port)

class PRServerConnection(object):
    def __init__(self, host, port):
        if is_local_special(host, port):
            host, port = singleton.getinfo()
        self.host = host
        self.port = port
        self.connection, self.transport = bb.server.xmlrpc._create_server(self.host, self.port)

    def terminate(self):
        try:
            logger.info("Terminating PRServer...")
            self.connection.quit()
        except Exception as exc:
            sys.stderr.write("%s\n" % str(exc))

    def getPR(self, version, pkgarch, checksum):
        return self.connection.getPR(version, pkgarch, checksum)

    def ping(self):
        return self.connection.ping()

    def export(self,version=None, pkgarch=None, checksum=None, colinfo=True):
        return self.connection.export(version, pkgarch, checksum, colinfo)

    def importone(self, version, pkgarch, checksum, value):
        return self.connection.importone(version, pkgarch, checksum, value)

    def getinfo(self):
        return self.host, self.port

def start_daemon(dbfile, host, port, logfile):
    pidfile = PIDPREFIX % (host, port)
    try:
        pf = file(pidfile,'r')
        pid = int(pf.readline().strip())
        pf.close()
    except IOError:
        pid = None

    if pid:
        sys.stderr.write("pidfile %s already exist. Daemon already running?\n"
                            % pidfile)
        return 1

    server = PRServer(os.path.abspath(dbfile), os.path.abspath(logfile), (host,port))
    server.start()
    return 0

def stop_daemon(host, port):
    pidfile = PIDPREFIX % (host, port)
    try:
        pf = file(pidfile,'r')
        pid = int(pf.readline().strip())
        pf.close()
    except IOError:
        pid = None

    if not pid:
        sys.stderr.write("pidfile %s does not exist. Daemon not running?\n"
                        % pidfile)

    try:
        PRServerConnection(host, port).terminate()
    except:
        logger.critical("Stop PRService %s:%d failed" % (host,port))

    try:
        if pid:
            wait_timeout = 0
            print("Waiting for pr-server to exit.")
            while is_running(pid) and wait_timeout < 50:
                time.sleep(0.1)
                wait_timeout += 1

            if is_running(pid):
                print("Sending SIGTERM to pr-server.")
                os.kill(pid,signal.SIGTERM)
                time.sleep(0.1)

            if os.path.exists(pidfile):
                os.remove(pidfile)

    except OSError as e:
        err = str(e)
        if err.find("No such process") <= 0:
            raise e

    return 0

def is_running(pid):
    try:
        os.kill(pid, 0)
    except OSError as err:
        if err.errno == errno.ESRCH:
            return False
    return True

def is_local_special(host, port):
    if host.strip().upper() == 'localhost'.upper() and (not port):
        return True
    else:
        return False

class PRServiceConfigError(Exception):
    pass

def auto_start(d):
    global singleton

    host_params = filter(None, (d.getVar('PRSERV_HOST', True) or '').split(':'))
    if not host_params:
        return None

    if len(host_params) != 2:
        logger.critical('\n'.join(['PRSERV_HOST: incorrect format',
                'Usage: PRSERV_HOST = "<hostname>:<port>"']))
        raise PRServiceConfigError

    if is_local_special(host_params[0], int(host_params[1])) and not singleton:
        import bb.utils
        cachedir = (d.getVar("PERSISTENT_DIR", True) or d.getVar("CACHE", True))
        if not cachedir:
            logger.critical("Please set the 'PERSISTENT_DIR' or 'CACHE' variable")
            raise PRServiceConfigError
        bb.utils.mkdirhier(cachedir)
        dbfile = os.path.join(cachedir, "prserv.sqlite3")
        logfile = os.path.join(cachedir, "prserv.log")
        singleton = PRServSingleton(os.path.abspath(dbfile), os.path.abspath(logfile), ("localhost",0))
        singleton.start()
    if singleton:
        host, port = singleton.getinfo()
    else:
        host = host_params[0]
        port = int(host_params[1])

    try:
        connection = PRServerConnection(host,port)
        connection.ping()
        realhost, realport = connection.getinfo()
        return str(realhost) + ":" + str(realport)
        
    except Exception:
        logger.critical("PRservice %s:%d not available" % (host, port))
        raise PRServiceConfigError

def auto_shutdown(d=None):
    global singleton
    if singleton:
        host, port = singleton.getinfo()
        try:
            PRServerConnection(host, port).terminate()
        except:
            logger.critical("Stop PRService %s:%d failed" % (host,port))
        singleton = None

def ping(host, port):
    conn=PRServerConnection(host, port)
    return conn.ping()
