# Vaderstation v. a4.1



ABOUT:
  Personal DIY weatherstation project
    Currently the goal is to make this code have the same fuctionality as "logger03" but a lot better-written. By "better written"
    I mean easier to read and understand, easier to build upon (add or remove parts) and more robust code that also runs more 
    efficient and allows for future increased complexity. This is because the project goal is a fully flexed Wheatherstation that
    logs accurate enviromental data from both inside and outside. The plan is to later use an old smartphone to, instead of a 
    smaller caracter display, serve as GUI. Therefore the arduino or probably teensy will need an way to comunicate to the phone.
    The andriod code will likely not have much or any command/control duty so that it can be deactivated during power outtages;
    leaving the backup battery for the lower power consuming arduino/teensy. Thats at least the plan now.

VERSION LEGEND:
  Alpha = not up to the original functionality of "logger03".
  Beta = same or increased functionality of "logger03".
  Full version = Code incoperates all currently owned sensors relevant to the project, with wireless comunication to a outside 
  module and capability to communicate through usb (to eventual smartphone).
