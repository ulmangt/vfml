#!/uns/bin/python

import fcntl, time, string, os

gVerbose = 0;
gExecute = 1;

def MySystem(command):
   if gVerbose:
      print command
   if gExecute:
      os.system(command);

class Job:
   def _IsDeliminator(self, s):
      return s == "---"
   
   def __init__(self, file):
      # defaults
      self.name        = ""
      self.commandList = []
      
      foundOpen = 0
      done = 0
      
      line = file.readline()
      while line != '' and not done:
         line = string.strip(line)
         
         if self._IsDeliminator(line):
            if foundOpen:
               done = 1
            else:
               foundOpen = 1
         elif len(line) > 0:
            if self.name == "":
               self.name = line
            else:
               self.commandList.append(line)

         if not done:
            line = file.readline()

      if not done:
         raise IOError()
         
   def Run(self):
      os.environ['jobname'] = self.name
      for command in self.commandList:
         MySystem(command)

   def __str__(self):
      s = "---\n"
      s = s + self.name + "\n"
      for command in self.commandList:
         s = s + command + "\n"
      s = s + "---\n"
      return s
      

class JobQueue:
   def _GetLock(self):
      # Make my lock file
      if self.localLock == None:
         self.localLock = open(self.localLockFileName, "w")

         # loop till we get the lock.  The only portable atomic get & set in
         #  NFS file systems is the link command, so we make a local file
         #  and try to make a link to it via a shared name until the link
         #  succedes
         haveLock = 0
         while not haveLock:
            try:
               os.link(self.localLockFileName, self.lockFileName)

               if os.stat(self.localLockFileName)[3] == 2:
                  haveLock = 1
            except:
               pass

   def _ReleaseLock(self):
      # relese the files
      if self.localLock != None:
         os.remove(self.lockFileName)
         self.localLock.close()
         os.remove(self.localLockFileName)
         self.localLock = None
      
   def __init__(self, queuefile, lockdir):
      self.lockDir = lockdir
      self.localLockFileName = lockdir + "/%s%d" % (string.split(os.uname()[1], '.')[0], os.getpid())
      self.lockFileName = lockdir + "/lock"
      self.queueFile = queuefile
      self.localLock = None

   def AddJob(self, job):
      self._GetLock()

      queueFile = open(self.queueFile, 'a')
      queueFile.write("%s" % (job))
      queueFile.flush()
      queueFile.close()

      self._ReleaseLock()
      
   def GetAndRemoveNextJob(self):
      self._GetLock()

      # read the queue
      queueFile = open(self.queueFile, 'r')

      jobs = []
      done = 0

      while not done:
         try:
            j = Job(queueFile)
            jobs.append(j)
         except IOError:
            done = 1

      queueFile.close()

      # remove the next job (maybe by priority)
      if len(jobs) == 0:
         haveWork = 0
         job = None
      else:
         haveWork = 1
         job = jobs[0]
         jobs = jobs[1:]

      # rewrite the new queue
      queueFile = open(self.queueFile, 'w')
      for j in jobs:
         queueFile.write("%s" % (j))
      queueFile.flush()
      queueFile.close()

      self._ReleaseLock()

      return job
