# Dockerfile to SH

This is a bit of a weird little tool. It was initially based on the conversion script here:

https://github.com/thatkevin/dockerfile-to-shell-script

But using the convertor using bash scripting and sed is a little messy and has limited control.

I changed to using a luajit based script and this allowed for some nice features:

- Ability to set sections of the Dockerfile on/off
- More complicated replacement procedures
- Easier to manage changes to Dockerfile (ie when Dockerfile changes, the lua script is very easy to update).

This is only in test mode at the moment, and should not be used to generate an extender server. 

Once complete the intent is to be able to build an Defold extender server and create images for portable use in restricted environments.
