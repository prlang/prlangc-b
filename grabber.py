import glob
from sys import argv

sources = glob.glob('*.c') + glob.glob('**/*.c')
for i in sources:
    print(i)