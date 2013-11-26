#!/usr/bin/env python
import sys
from xml.dom.minidom import getDOMImplementation
from subprocess import Popen, PIPE

impl = getDOMImplementation()
result = impl.createDocument(None, "testsuite", None)
testsuite = result.documentElement

for fn in sys.argv[1:]:
  process = Popen(["./" + fn], stdout=PIPE, stderr=PIPE)
  (stdoutdata, stderrdata) = process.communicate()

  testcase = result.createElement("testcase")
  testcase.setAttribute("classname", "vm")
  testcase.setAttribute("name", fn)

  systemout = result.createElement("system-out")
  systemout.appendChild(result.createTextNode(stdoutdata))
  testcase.appendChild(systemout)

  systemerr = result.createElement("system-err")
  systemerr.appendChild(result.createTextNode(stderrdata))
  testcase.appendChild(systemerr)

  if process.returncode != 0:
    failure = result.createElement("failure")
    failure.setAttribute("type", "returncode")
    failure.appendChild(result.createTextNode("code: " + str(process.returncode)))
    testcase.appendChild(failure)

  testsuite.appendChild(testcase)

print result.toxml()
