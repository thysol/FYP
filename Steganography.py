import subprocess
from tkinter import * 
from tkinter import filedialog

filenameHide =  filedialog.askopenfilename(initialdir = "E:/Images",title = "Choose file to hide",filetypes = ())
print (filenameHide)

filenameCarrier =  filedialog.askopenfilename(initialdir = "E:/Images",title = "Choose carrier image",filetypes = ())
print (filenameCarrier)

executable = ['Steganography.exe', filenameHide, filenameCarrier]
process = subprocess.Popen(executable)

