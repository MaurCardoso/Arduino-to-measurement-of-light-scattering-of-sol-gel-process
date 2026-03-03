How to Generate the Executable (.exe) File from ABEL.py

This guide explains how to generate the Windows executable file (.exe) for the ABEL software using the provided setup.py script and PyInstaller.

The application is based on PyQt6 and communicates with an Arduino device for real-time light scattering measurements.

1) Install Python

Go to the official Python website:
https://www.python.org
Download Python 3.10 or newer.
During installation:
- Check “Add Python to PATH”
- Then click Install

After installation, verify it works:
Open Command Prompt (Using Windows Search type "cmd" and enter) and type:

python --version
You should see the installed version.

2) Install Required libraries

Go to the folder where your project files are located
(the folder containing ORLS.py, setup.py, ORLS.ico, and Main_Window.py).
Click on the address bar of the folder and type:

cmd

Press Enter.
Then install all required libraries, one at a time typing and Enter:
a) pip install pyinstaller 
b) pip install PyQt6
c) pip install numpy
d) pip install matplotlib
e) pip install pyserial

3) Build the Executable

In the same comand windows, run:

python setup.py

4) Locate the Executable

After the process finishes, two new folders will appear:

build
dist

Open the dist folder.
Inside you will find:

ORLS.exe


That is your final executable.
