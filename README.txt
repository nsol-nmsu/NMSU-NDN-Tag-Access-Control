Hi Reza:
Usage is pretty straightforward, I'm in a hurry since I'm leaving
for Deming at 10:00pm ( in an hour or so ) so I'm gonna be brief.

The simulation consists of two main directories: ns-3 and scenario.
The third directory pybinden is for visualization and can be ignored
since I haven't been able to get it to work with the current setup.
The ns-3 directory contains the modified ns-3/ndnSIM library, you'll
need to install it on any machine that you wanna run the simulation
on; you can either install to the default directory or to a local
one, but on some systems ( my openSUSE ) is doesn't work so well
in local directories.  Instructions for installing ns-3/ndnSIM are
here http://ndnsim.net/2.1/getting-started.html.  The one difference
is that you'll also need to make ns-3/BRITE and copy libbrite.so
from that directory to a library path ( maybe /usr/lib64 or /usr/local/lib64 )
and use the '--with-brite=BRITE' option when configuring.

To configure ns-3 for optimized execution you'll need to include the
'-d optimized' option:

./waf configure --with-brite=BRITE -d optimized

The scenario directory is where most customizations are.  You can build it with:

./waf configure
./waf

but there may be some library paths and include paths that need to be configured
namely PKG_CONFIG_PATH and LD_LIBRARY_PATH.  These should point to the .../lib/pkgconfig
and .../lib libraries to which ns-3 was installed; these may be lib64 directories also
depending on the system.

The scenario directory has four important directories, the rest can be ignored. These
are: extensions, scenarios, config, and results.

extensions:
This is where most of the code is; it's pretty nasty right now so you may not
wanna explore it unless it's really necessary.  But you may need to look through
it in case some results don't look right; they actually looked a bit odd to me
for the small simulation I ran.

scenarios:
This directory only has one file 'auth-tag-simulation.cc' which is the driver file,
it also implements the topology genereation.  It can also mostly be ignored if you
don't need to go through the code; but if you're gonna try to get one of the ISP
topologies to work this is the file you'll wanna change.  There's one subtlty that
needs to be taken into account when generating the topology:  edge network devices
need to be marked as such by aggregating with the IsEdgeFlag class.  An example of
this can be found on lines 627 and 643 of the 'auth-tag-simulation.cc' file.

config:
Files in here are used to configure the simulation.  These are .jx9 scripts
and .brite config files.  The 'producer_config.jx9' tells the simulation how
to configure any producer app given its instance ID, and 'consumer_config.jx9'
does the same for consumers.  The 'simulation_config.jx9' configures global
settings, but for now it can't enable or disable traces, all traces are hardcoded
to enabled.  The simulation_config file can control how long to run the simulation
and how many consumer, producers, and edges to create.  It also specified what config
files to use for other configurations: network, consumer, and producer.  Router
and edge configuration parsers aren't implemented yet.

results:
The trace reults go here.  You'll need to look through the extensions/tracers.hpp
and extensions/tracers.cpp comments to get descriptions of the tracers.  Trace log
files are hardcoded in 'auth-tag-simulation.cc' so if you wanna change them for some
reason that's where you'll go; but for the most part they are named according to their
trace names.

Well, that's it for now.  If you need something my number is (575) 404-8259.  I likely
won't have a good enough connection to ssh, so my help'll be limited.

# I actually didn't have time to document the trace formats, so you'll likely need to
look through the extensions/tracers.cpp file to figure out the fields.  I'll document
these later.

Ray Stubbs
