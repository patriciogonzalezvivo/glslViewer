import OSC
c = OSC.OSCClient()
c.connect(('127.0.0.1', 8000))   # connect to SuperCollider

oscmsg = OSC.OSCMessage()
oscmsg.setAddress("/multifaderM_1")
oscmsg.append(0.5)

# oscmsg.setAddress("/debug")
# oscmsg.append('on')

# oscmsg.setAddress("/help")

c.send(oscmsg)