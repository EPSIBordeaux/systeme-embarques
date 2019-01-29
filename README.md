# Systeme Embarqués

# Pre-requisite

- vagrant >= 2.2.3 (support with VirtualBox 6)
- VirtualBox >= 6

# Installation 

- Clone https://gitlab.com/labasse/rt-esp
- vagrant up 
- take a coffee or two

- vagrant ssh or `ssh vagrant@localhost -p 2222` with password `vagrant`
- clone this repository
- plug USB
- in Virtualbox, set-up a shared folder mapped to this repository
- in your vagrant VM, go to the shared folder and run `make`

- in vagrant VM, run `lsusb` and `sudo esptool.py chip_id`. You should see your USB port.
- To flash, run `make flash`
- To run : `make monitor`

