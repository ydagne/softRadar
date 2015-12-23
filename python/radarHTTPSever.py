#
#  Copyright 2015 Yihenew Beyene
#  
#  This file is part of SoftRadar.
#  
#      SoftRadar is free software: you can redistribute it and/or modify
#      it under the terms of the GNU General Public License as published by
#      the Free Software Foundation, either version 3 of the License, or
#      (at your option) any later version.
#  
#      SoftRadar is distributed in the hope that it will be useful,
#      but WITHOUT ANY WARRANTY; without even the implied warranty of
#      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#      GNU General Public License for more details.
#  
#      You should have received a copy of the GNU General Public License
#      along with SoftRadar.  If not, see <http://www.gnu.org/licenses/>.


"""Simple HTTP Server.

This module builds on BaseHTTPServer by implementing the standard GET
and HEAD requests in a fairly straightforward manner.

"""


__version__ = "0.6"

__all__ = ["SimpleHTTPRequestHandler"]

import os
import posixpath
import BaseHTTPServer
import urllib
import cgi
import shutil
import mimetypes
from StringIO import StringIO


import socket
import SocketServer
from optparse import OptionParser  # Command-line option parser
import string

from threading import Thread
import datetime
import time

import os
import sys
import cgi


global LOG_DIRECTORY

global PORT1 
global PORT2 
global STATE
global CONFIGURATION
global UPLOAD_STATE
global BURST_LEN

global EXIT_FLAG

BURST_LEN  = 1000

LOG_DIRECTORY = "log"  
  
EXIT_FLAG = False


def sendcommand(cmd):

      global PORT1
      writer=socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
      writer.sendto(cmd,("localhost",PORT1))
      
     
class deviceManager(Thread):

    
    def run(self):
   
       global LOG_DIRECTORY, STATE,UPLOAD_STATE, BURST_LEN
       
       os.system('../build/apps/deviceManager --srcport '+ \
                 str(PORT1)+' --destport ' + str(PORT2) + \
                 ' --dir ' + LOG_DIRECTORY + \
                 ' --burstlen ' + str(BURST_LEN) )
       
       print "Device manager is not running!"
       STATE = "DISCONNECTED"
       UPLOAD_STATE = "Error: no device found!"
    
class Updator(Thread):

    global EXIT_FLAG
    
    def run(self):
   
       while (1) :
       
           sendcommand("CMD GETCONFIG")
           sendcommand("CMD GETSTATE")
           
           time.sleep(4)
           
           if(EXIT_FLAG):
               exit();
           
              
       
class Monitor(Thread):

    global EXIT_FLAG
    
    def run(self):
    
      global PORT2,STATE,CONFIGURATION,UPLOAD_STATE
      
      reader=socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
      reader.bind(("localhost", PORT2))
      now = datetime.datetime.now()
      
      print "%s started at time: %s port: %s" % (self.getName(), now, PORT2)
      
      while(1):
      
         if(EXIT_FLAG):
               exit();
      
         data, addr = reader.recvfrom(1024) # buffer size is 1024 bytes
         
         if(len(data)<4):
             continue
             
         #print "rcvd response: "+data
         
         x=data.split(" ")
         
         if( x[0] != "RSP" or len(x)<3):
            continue      

         if(x[1] == "STATE"):
            STATE = x[2]
            
         elif(x[1] == "BURSTFLAG"):
            UPLOAD_STATE = " ".join(x[2:])
            
         elif(x[1] == "CONFIG"):
            data = data.split("*")
            if(len(data)>1):
                CONFIGURATION  = data[1].split("\n")
                


class SimpleHTTPRequestHandler(BaseHTTPServer.BaseHTTPRequestHandler):

    """Simple HTTP request handler with GET and HEAD commands.

    This serves files from the current directory and any of its
    subdirectories.  It assumes that all files are plain text files
    unless they have the extension ".html" in which case it assumes
    they are HTML files.

    The GET and HEAD requests are identical except that the HEAD
    request omits the actual contents of the file.

    """

    server_version = "SimpleHTTP/" + __version__
    
    
    def log_message(self, format, *args):
        return

    def do_GET(self):
    
        global PORT1,PORT2,STATE,CONFIGURATION,UPLOAD_STATE,LOG_DIRECTORY
        global EXIT_FLAG
        
        if self.path == "/log":
            self.path = '/'+LOG_DIRECTORY
            path = self.translate_path(LOG_DIRECTORY)
            f = self.list_directory2(path)
            
            if f:
                self.copyfile(f, self.wfile)
                f.close()
            else:
                self.send_error(404, "No log file created yet")
            return
            
        elif self.path == "/start":
            sendcommand("CMD START")
            self.send_response(200)
            self.send_header("Content-type", 'text/json')
            self.end_headers()
            return
            
        elif self.path == "/stop":
            sendcommand("CMD STOP")
            self.send_response(200)
            self.send_header("Content-type", 'text/json')
            self.end_headers()
            return

        elif self.path == "/terminate":
            sendcommand("CMD TERMINATE")
            self.send_response(200)
            self.send_header("Content-type", 'text/plain')
            self.end_headers()
            self.wfile.write("The radio has been terminated!")
            EXIT_FLAG = True
            return
            
        elif (self.path.split("?")[0] == "/upload"):
            self.send_response(200)
            self.send_header("Content-type", 'text/json')
            self.end_headers()

            tmp = self.path.split("?")
            if(len(tmp)<2):
               return
               
            fname = tmp[1]
            sendcommand("CMD LOAD " + fname)
            return
            
        elif self.path == "/config":
            self.send_response(200)
            self.send_header("Content-type", 'text/plain')
            self.end_headers()
            if(STATE == "DISCONNECTED"):
                self.wfile.write("Error:,no device found!")
            else:
                self.wfile.write(CONFIGURATION)
            return
            
        elif self.path == "/state":
            self.send_response(200)
            self.send_header("Content-type", 'text/plain')
            self.end_headers()
            self.wfile.write(STATE)
            return
            
        elif self.path == "/uploadstate":
            self.send_response(200)
            self.send_header("Content-type", 'text/plain')
            self.end_headers()
            self.wfile.write(UPLOAD_STATE)
            if(UPLOAD_STATE != "Error: no device found!"):
                 UPLOAD_STATE = ""  # Reset
            return
            
            
        """Serve a GET request."""
        f = self.send_head()
        if f:
            self.copyfile(f, self.wfile)
            f.close()
        else:
            self.send_error(404, "File not found")


    def do_POST(self):

        form = cgi.FieldStorage(
            fp=self.rfile,
            headers=self.headers,
            environ={'REQUEST_METHOD':'POST',
                     'CONTENT_TYPE':self.headers['Content-Type'],
                     })
        #filename = form['file'].filename
        filename = 'tmp.wm'
        data = form['file'].file.read()

        if(len(data)>0 and len(filename)>0):

            open(filename, "wb").write(data)        
            response = "The waveform is being analyzed..."
            self.send_response(200)
            self.send_header("Content-type", "text/html")
            self.send_header("Content-length", len(response))
            self.end_headers()
            self.wfile.write(response) 
            sendcommand("CMD LOAD "+ filename) 

        else:
            response = "please select file"
            self.send_response(201)
            self.send_header("Content-type", "text/html")
            self.send_header("Content-length", len(response))
            self.end_headers()
            self.wfile.write(response)  



    def do_HEAD(self):
        """Serve a HEAD request."""
        f = self.send_head()
        if f:
            f.close()

    def send_head(self):
        """Common code for GET and HEAD commands.

        This sends the response code and MIME headers.

        Return value is either a file object (which has to be copied
        to the outputfile by the caller unless the command was HEAD,
        and must be closed by the caller under all circumstances), or
        None, in which case the caller has nothing further to do.

        """
        path = self.translate_path(self.path)
        f = None
        if os.path.isdir(path):
            for index in "web/index.htm", "web/index.html":
                index = os.path.join(path, index)
                if os.path.exists(index):
                    path = index
                    break
            else:
                return self.list_directory(path)
        ctype = self.guess_type(path)
        if ctype.startswith('text/'):
            mode = 'r'
        else:
            mode = 'rb'
        try:
            f = open(path, mode)
        except IOError:
            return None
        self.send_response(200)
        self.send_header("Content-type", ctype)
        self.end_headers()
        return f

    def list_directory(self, path):
        """Helper to produce a directory listing (absent index.html).

        Return value is either a file object, or None (indicating an
        error).  In either case, the headers are sent, making the
        interface the same as for send_head().

        """
        try:
            list = os.listdir(path)
        except os.error:
            self.send_error(404, "No permission to list directory")
            return None
        list.sort(lambda a, b: cmp(a.lower(), b.lower()))
        f = StringIO()
        f.write("<title>Directory listing for %s</title>\n" % self.path)
        f.write("<h2>Directory listing for %s</h2>\n" % self.path)
        f.write("<hr>\n<ul>\n")
        for name in list:
            fullname = os.path.join(path, name)
            displayname = linkname = name = cgi.escape(name)
            # Append / for directories or @ for symbolic links
            if os.path.isdir(fullname):
                displayname = name + "/"
                linkname = name + "/"
            if os.path.islink(fullname):
                displayname = name + "@"
                # Note: a link to a directory displays with @ and links with /
            f.write('<li><a href="%s">%s</a>\n' % (linkname, displayname))
        f.write("</ul>\n<hr>\n")
        f.seek(0)
        self.send_response(200)
        self.send_header("Content-type", "text/html")
        self.end_headers()
        return f


    def list_directory2(self, path):
        """Helper to produce a directory listing (absent index.html).

        Return value is either a file object, or None (indicating an
        error).  In either case, the headers are sent, making the
        interface the same as for send_head().

        """
        try:
            list = os.listdir(path)
        except os.error:
            return None
        list.sort(lambda a, b: cmp(a.lower(), b.lower()))
        f = StringIO()
        f.write("<title>Directory listing for %s</title>\n" % self.path)
        f.write("<h2>Directory listing for %s</h2>\n" % self.path)
        f.write("<hr>\n<ul>\n")
        for name in list:
            fullname = os.path.join(path, name)
            displayname = linkname = name = cgi.escape(name)
            # Append / for directories or @ for symbolic links
            if os.path.isdir(fullname):
                displayname = name + "/"
                linkname = self.path+"/"+name + "/"
            if os.path.islink(fullname):
                displayname = name + "@"
                # Note: a link to a directory displays with @ and links with /
            f.write('<li><a href="%s">%s</a>\n' % (linkname, displayname))
        f.write("</ul>\n<hr>\n")
        f.seek(0)
        self.send_response(200)
        self.send_header("Content-type", "text/html")
        self.end_headers()
        return f
        
    def translate_path(self, path):
        """Translate a /-separated PATH to the local filename syntax.

        Components that mean special things to the local file system
        (e.g. drive or directory names) are ignored.  (XXX They should
        probably be diagnosed.)

        """
        path = posixpath.normpath(urllib.unquote(path))
        words = path.split('/')
        words = filter(None, words)
        path = os.getcwd()
        for word in words:
            drive, word = os.path.splitdrive(word)
            head, word = os.path.split(word)
            if word in (os.curdir, os.pardir): continue
            path = os.path.join(path, word)
        return path

    def copyfile(self, source, outputfile):
        """Copy all data between two file objects.

        The SOURCE argument is a file object open for reading
        (or anything with a read() method) and the DESTINATION
        argument is a file object open for writing (or
        anything with a write() method).

        The only reason for overriding this would be to change
        the block size or perhaps to replace newlines by CRLF
        -- note however that this the default server uses this
        to copy binary data as well.

        """
        shutil.copyfileobj(source, outputfile)

    def guess_type(self, path):
        """Guess the type of a file.

        Argument is a PATH (a filename).

        Return value is a string of the form type/subtype,
        usable for a MIME Content-type header.

        The default implementation looks the file's extension
        up in the table self.extensions_map, using text/plain
        as a default; however it would be permissible (if
        slow) to look inside the data to make a better guess.

        """

        base, ext = posixpath.splitext(path)
        if self.extensions_map.has_key(ext):
            return self.extensions_map[ext]
        ext = ext.lower()
        if self.extensions_map.has_key(ext):
            return self.extensions_map[ext]
        else:
            return self.extensions_map['']

    extensions_map = mimetypes.types_map.copy()
    extensions_map.update({
        '': 'application/octet-stream', # Default
        '.py': 'text/plain',
        '.c': 'text/plain',
        '.h': 'text/plain',
        })


def test(HandlerClass = SimpleHTTPRequestHandler,
         ServerClass = BaseHTTPServer.HTTPServer):
    BaseHTTPServer.test(HandlerClass, ServerClass)

      
     
class serverManager(Thread):

    global server
    def run(self):
   
       server.serve_forever()


def main():

    global PORT1,PORT2,STATE,CONFIGURATION,UPLOAD_STATE,BURST_LEN, EXIT_FLAG, server
    parser = OptionParser( conflict_handler="resolve") 
    parser.add_option("-p","--port", type = "int", default=8888,
    help="Choose TCP port to listen to, default = 8888") 
    parser.add_option("-b","--burstlen", type = "int", default=1000,
    help="Set waveform burst length, default = 1000") 

    

    (options, args) = parser.parse_args ()

    try:
          server = SocketServer.TCPServer(('', options.port), SimpleHTTPRequestHandler)
          print 'Serving on port: ' + str(options.port) + '....'
          print ' type ^C to stop the server'
          PORT1 = options.port+10
          PORT2 = options.port+20
          BURST_LEN = options.burstlen
          STATE = 'IDLE'
          UPLOAD_STATE = '  '
          CONFIGURATION = []
          
          # Start the radio
          
          d = deviceManager()
          m = Monitor()
          u = Updator()
          s = serverManager()
          
          d.start()
          m.start()
          u.start()
          s.start()
          
          while(1):
          
             if(EXIT_FLAG):
             
               d._Thread__stop()
               m._Thread__stop()
               u._Thread__stop()
               s._Thread__stop()
               return
               
             time.sleep(1)               
             
          
    except KeyboardInterrupt:
          d._Thread__stop()
          m._Thread__stop()
          u._Thread__stop()
          s._Thread__stop()

if __name__ == '__main__':

    main()

